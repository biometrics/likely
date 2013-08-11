/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright 2013 Joshua C. Klontz                                           *
 *                                                                           *
 * Licensed under the Apache License, Version 2.0 (the "License");           *
 * you may not use this file except in compliance with the License.          *
 * You may obtain a copy of the License at                                   *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 * Unless required by applicable law or agreed to in writing, software       *
 * distributed under the License is distributed on an "AS IS" BASIS,         *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
 * See the License for the specific language governing permissions and       *
 * limitations under the License.                                            *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <QtCore>
#include <QtWidgets>
#include "likely.h"

class MatrixViewer : public QLabel
  { Q_OBJECT
    QImage src;
    int zoomLevel = 0;
public:
    explicit MatrixViewer(QWidget *p = 0) : QLabel(p)
      { setAcceptDrops(true);
        setAlignment(Qt::AlignCenter);
        setMouseTracking(true);
        updatePixmap(); }
public slots:
    void setImage(const QImage &i)             { src =  i;      updatePixmap(); }
    void zoomIn()  { if (++zoomLevel >  4) zoomLevel =  4; else updatePixmap(); }
    void zoomOut() { if (--zoomLevel < -4) zoomLevel = -4; else updatePixmap(); }
private:
    void dragEnterEvent(QDragEnterEvent *e) { e->accept(); if (e->mimeData()->hasUrls() || e->mimeData()->hasImage()) e->acceptProposedAction(); }
    void dropEvent(QDropEvent *e)
      { e->accept(); e->acceptProposedAction();
        const QMimeData *md = e->mimeData();
        if (md->hasImage())
          { emit newMatrix(qvariant_cast<QImage>(md->imageData())); }
        else if (md->hasUrls())
          { foreach (const QUrl &url, md->urls())
              { if (!url.isValid()) continue;
                const QString file = url.toLocalFile();
                if (file.isNull()) continue;
                emit newMatrix(QImage(file));
                break; } } }
    void leaveEvent(QEvent *e) { e->accept(); queryPoint(QPoint(-1, -1)); }
    void mouseMoveEvent(QMouseEvent *e) { e->accept(); queryPoint(e->pos() / pow(2, zoomLevel)); }
    void updatePixmap()
      { if (src.isNull())
          { clear();
            setText("<b>Drag and Drop Image Here</b>");
            resize(512, 512);
            queryPoint(QPoint(-1, -1));
            setFrameShape(QFrame::StyledPanel); }
        else
          { const QSize newSize = src.size() * pow(2, zoomLevel);
            setPixmap(QPixmap::fromImage(src.scaled(newSize, Qt::KeepAspectRatio)));
            resize(newSize);
            queryPoint(mapFromGlobal(QCursor::pos()) / pow(2, zoomLevel));
            setFrameShape(QFrame::NoFrame); } }
    void queryPoint(const QPoint &p)
      { if (src.rect().contains(p))
          { const QRgb pixel = src.pixel(p);
            emit newPosition(QString("%1,%2")
                             .arg(QString::number(p.x()),
                                  QString::number(p.y())));
            emit newColor(QString("<font color=\"red\">%1</font>,<font color=\"green\">%2</font>,<font color=\"blue\">%3</font>")
                          .arg(QString::number(qRed(pixel)),
                               QString::number(qGreen(pixel)),
                               QString::number(qBlue(pixel)))); }
        else
          { emit newPosition("");
            emit newColor(""); } }
signals:
    void newMatrix(QImage);
    void newPosition(QString);
    void newColor(QString); };

class ShyLabel : public QLabel
  { Q_OBJECT
    QString name;
    int wheelRemainder = 0;

public:
    bool in = false;

    ShyLabel(const QString &name_, QWidget *parent = 0) : QLabel(parent), name(name_)
      { setCursor(Qt::IBeamCursor);
        setFocusPolicy(Qt::StrongFocus); }

    void setName(const QString &name_) { name = name_; }

public slots:
    void setText(const QString &text)
      { QLabel::setText(text);
        if (in) emit newParameter(name+" = "+text); }

private:
    void enterEvent(QEvent *event)
      { event->accept();
        emit newParameter(name+" = "+text());
        in = true; }

    void focusInEvent(QFocusEvent *focusEvent)
      { focusEvent->accept();
        emit focus();}

    void leaveEvent(QEvent *event)
      { event->accept();
        emit newParameter("");
        in = false; }

    void wheelEvent(QWheelEvent *wheelEvent)
      { wheelEvent->accept();
        wheelRemainder += wheelEvent->angleDelta().x() + wheelEvent->angleDelta().y();
        const int delta = wheelRemainder / 120;
        if (delta != 0)
          { wheelRemainder = wheelRemainder % 120;
            emit change(delta); } }

signals:
    void change(int delta);
    void focus();
    void newParameter(QString parameter); };

class ShyComboBox : public QComboBox
  { Q_OBJECT
    QString name;
    ShyLabel *shyLabel;

public:
    ShyComboBox(const QString &name_, QWidget *parent = 0) : QComboBox(parent), name(name_)
      { shyLabel = new ShyLabel(name_, this);
        setEditable(true);
        setInsertPolicy(QComboBox::NoInsert);
        connect(this, SIGNAL(currentIndexChanged(QString)), shyLabel, SLOT(setText(QString)));
        connect(shyLabel, SIGNAL(change(int)), this, SLOT(change(int)));
        connect(shyLabel, SIGNAL(focus()), this, SLOT(show()));
        connect(shyLabel, SIGNAL(newParameter(QString)), this, SIGNAL(newParameter(QString)));
        hide(); }

    ~ShyComboBox()         { delete shyLabel; }
    QWidget *proxy() const { return shyLabel; }

public slots:
    void change(int delta) { setCurrentIndex(qMin(qMax(currentIndex() - delta, 0), count()-1)); }

    void hide()
      { QComboBox::hide();
        shyLabel->show(); }

    void show()
      { QComboBox::show();
        shyLabel->hide();
        setFocus(); }

private:
    void enterEvent(QEvent *event)
      { event->accept();
        emit newParameter(name+" = "+currentText());
        shyLabel->in = true; }

    void focusOutEvent(QFocusEvent *focusEvent)
      { QComboBox::focusOutEvent(focusEvent);
        if ((focusEvent->reason() != Qt::PopupFocusReason) &&
            (focusEvent->reason() != Qt::OtherFocusReason))
          { focusEvent->accept();
            hide(); } }

    void leaveEvent(QEvent *event)
      { event->accept();
        emit newParameter("");
        shyLabel->in = false; }

signals:
    void newParameter(QString parameter); };

class ShyDoubleSpinBox : public QDoubleSpinBox
  { Q_OBJECT
    QString name;
    ShyLabel *shyLabel;

public:
    ShyDoubleSpinBox(const QString &name_, QWidget *parent = 0) : QDoubleSpinBox(parent), name(name_)
      { shyLabel = new ShyLabel(name_, this);
        connect(this, SIGNAL(valueChanged(QString)), shyLabel, SLOT(setText(QString)));
        connect(shyLabel, SIGNAL(change(int)), this, SLOT(change(int)));
        connect(shyLabel, SIGNAL(focus()), this, SLOT(show()));
        connect(shyLabel, SIGNAL(newParameter(QString)), this, SIGNAL(newParameter(QString)));
        shyLabel->setText(QString::number(value()));
        hide(); }

    ~ShyDoubleSpinBox()    { delete shyLabel; }
    QWidget *proxy() const { return shyLabel; }
    void setName(const QString &name_) { name = name_; shyLabel->setName(name_); }

public slots:
    void change(int delta) { setValue(value() + delta); }

    void hide()
      { QDoubleSpinBox::hide();
        shyLabel->show(); }

    void show()
      { QDoubleSpinBox::show();
        shyLabel->hide();
        setFocus(); }

private:
    void enterEvent(QEvent *event)
      { event->accept();
        emit newParameter(name+" = "+text());
        shyLabel->in = true; }

    void focusOutEvent(QFocusEvent *event)
      { event->accept();
        hide(); }

    void leaveEvent(QEvent *event)
      { event->accept();
        emit newParameter("");
        shyLabel->in = false; }

signals:
    void newParameter(QString parameter); };

class Function : public QWidget
  { Q_OBJECT
    static QStringListModel *functionNames;

    QHBoxLayout *layout;
    ShyComboBox *functionChooser;
    QList<ShyDoubleSpinBox*> parameterChoosers;

    likely_matrix input;
    likely_unary_function function = NULL;

public:
    Function(QWidget *parent = 0) : QWidget(parent)
      { if (functionNames == NULL)
          { const char **function_names;
            int num_functions;
            likely_functions(&function_names, &num_functions);
            QStringList strings; strings.reserve(num_functions);
            for (int i=0; i<num_functions; i++)
                strings.append(function_names[i]);
            functionNames = new QStringListModel(strings); }

        likely_matrix_initialize(&input);

        functionChooser = new ShyComboBox("function", this);
        layout = new QHBoxLayout(this);
        layout->addWidget(functionChooser->proxy());
        layout->addWidget(functionChooser);

        connect(functionChooser, SIGNAL(currentIndexChanged(QString)), this, SLOT(updateParameters(QString)));
        connect(functionChooser, SIGNAL(newParameter(QString)), this, SIGNAL(newParameter(QString)));
        functionChooser->setModel(functionNames); }

public slots:
    void setInput(const QImage &image)
      { likely_free(&input);
        if (!image.isNull())
          { likely_matrix_initialize(&input, likely_hash_u8, 3, image.width(), image.height(), 1);
            likely_allocate(&input);
            memcpy(input.data, image.constBits(), likely_bytes(&input)); }
        compute(); }

    void setInput(QAction *action)
      { QString file;
        if      (action->text() == "New...")  file = "";
        else if (action->text() == "Open...") file = QFileDialog::getOpenFileName(NULL, "Open File");
        else                                  file = action->data().toString();
        setInput(file.isEmpty() ? QImage() : QImage(file).convertToFormat(QImage::Format_RGB888)); }

private:
    void compute()
      { if (input.data == NULL)
          { emit newMatrixView(QImage());
            emit newHash(QString());
            emit newDimensions(QString());
            return; }

        likely_matrix output;
        likely_matrix_initialize(&output);
        function(&input, &output);
        QImage outputImage(output.data, output.columns, output.rows, QImage::Format_RGB888);
        emit newMatrixView(outputImage.copy());
        emit newHash(likely_hash_to_string(input.hash));
        emit newDimensions(QString("%1x%2x%3x%4")
                           .arg(QString::number(input.channels),
                                QString::number(input.rows),
                                QString::number(input.columns),
                                QString::number(input.frames))); }

private slots:
    void updateParameters(const QString &function)
      { const char **parameter_names, **defaults;
        int num_parameters;
        likely_parameters(qPrintable(function), &parameter_names, &num_parameters, &defaults);
        QStringList strings; strings.reserve(num_parameters);
        for (int i=0; i<num_parameters; i++)
            strings.append(parameter_names[i]);

        while (parameterChoosers.size() > num_parameters)
          { layout->removeWidget(parameterChoosers.last()->proxy());
            layout->removeWidget(parameterChoosers.last());
            delete parameterChoosers.takeLast(); }

        while (parameterChoosers.size() < num_parameters)
          { ShyDoubleSpinBox *chooser = new ShyDoubleSpinBox("", this);
            parameterChoosers.append(chooser);
            layout->addWidget(chooser->proxy());
            layout->addWidget(chooser);
            connect(chooser, SIGNAL(valueChanged(QString)), this, SLOT(compile()));
            connect(chooser, SIGNAL(newParameter(QString)), this, SIGNAL(newParameter(QString))); }

        for (int i=0; i<num_parameters; i++)
          { parameterChoosers[i]->setName(parameter_names[i]);
            parameterChoosers[i]->setValue(QString(defaults[i]).toDouble());
            if (i == 0) setTabOrder(functionChooser, parameterChoosers[i]);
            else        setTabOrder(parameterChoosers[i-1], parameterChoosers[i]);
            if (i == num_parameters-1) setTabOrder(parameterChoosers[i], functionChooser); }

        compile(); }

    void compile()
      { QStringList arguments; arguments.reserve(arguments.size());
        foreach (const ShyDoubleSpinBox *parameterChooser, parameterChoosers)
            arguments.append(QString::number(parameterChooser->value()));
        const QString description = functionChooser->currentText()+"("+arguments.join(',')+")";
        function = likely_make_unary_function(qPrintable(description));
        compute(); }

signals:
    void newMatrixView(QImage image);
    void newHash(QString hash);
    void newDimensions(QString dimensions);
    void newParameter(QString parameter); };

QStringListModel *Function::functionNames = NULL;

class StatusLabel : public QLabel
{
    Q_OBJECT
    QString description;

public:
    StatusLabel(const QString &description)
      { this->description = description;
        setToolTip(description);
        setDescription(); }

public slots:
    void setText(const QString &text)
      { if (text.isEmpty()) setDescription();
        else { setEnabled(true); QLabel::setText(text); } }

private:
    void setDescription()
      { setEnabled(false);
        QLabel::setText(description); }

    void resizeEvent(QResizeEvent *resizeEvent)
      { resizeEvent->accept();
        setMinimumWidth(width()); }
};

int main(int argc, char *argv[])
  { QApplication application(argc, argv);

    QMenu *fileMenu = new QMenu("File");
    QAction *newFile = new QAction("New...", fileMenu);
    QAction *openFile = new QAction("Open...", fileMenu);
    newFile->setShortcut(QKeySequence("Ctrl+N"));
    openFile->setShortcut(QKeySequence("Ctrl+O"));
    fileMenu->addAction(newFile);
    fileMenu->addAction(openFile);
    fileMenu->addSeparator();
    foreach (const QString &fileName, QDir(":/img").entryList(QDir::Files, QDir::Name))
      { const QString filePath = ":/img/"+fileName;
        QAction *potentialFile = new QAction(QIcon(filePath), QFileInfo(filePath).baseName(), fileMenu);
        potentialFile->setData(filePath);
        potentialFile->setShortcut(QKeySequence("Ctrl+"+fileName.mid(0, 1)));
        fileMenu->addAction(potentialFile); }

    Function *function = new Function();
    MatrixViewer *matrixViewer = new MatrixViewer();
    QObject::connect(function, SIGNAL(newMatrixView(QImage)), matrixViewer, SLOT(setImage(QImage)));
    QObject::connect(matrixViewer, SIGNAL(newMatrix(QImage)), function, SLOT(setInput(QImage)));
    QObject::connect(fileMenu, SIGNAL(triggered(QAction*)), function, SLOT(setInput(QAction*)));

    QMenu *viewMenu = new QMenu("View");
    QAction *zoomIn = new QAction("Zoom In", viewMenu);
    QAction *zoomOut = new QAction("Zoom Out", viewMenu);
    zoomIn->setShortcut(QKeySequence("Ctrl++"));
    zoomOut->setShortcut(QKeySequence("Ctrl+-"));
    viewMenu->addAction(zoomIn);
    viewMenu->addAction(zoomOut);
    QObject::connect(zoomIn, SIGNAL(triggered()), matrixViewer, SLOT(zoomIn()));
    QObject::connect(zoomOut, SIGNAL(triggered()), matrixViewer, SLOT(zoomOut()));

    QMenuBar *menuBar = new QMenuBar();
    menuBar->addMenu(fileMenu);
    menuBar->addMenu(viewMenu);

    QGridLayout *centralWidgetLayout = new QGridLayout();
    QScrollArea *matrixViewerScrollArea = new QScrollArea();
    matrixViewerScrollArea->setAlignment(Qt::AlignCenter);
    matrixViewerScrollArea->setFrameShape(QFrame::NoFrame);
    matrixViewerScrollArea->setWidget(matrixViewer);
    centralWidgetLayout->addWidget(function, 0, 0);
    centralWidgetLayout->addWidget(matrixViewerScrollArea, 0, 1, 2, 1);
    centralWidgetLayout->setRowStretch(1, 1);

    QWidget *centralWidget = new QWidget();
    centralWidget->setLayout(centralWidgetLayout);

    StatusLabel *parameter = new StatusLabel("parameter");
    StatusLabel *hash = new StatusLabel("hash");
    StatusLabel *dimensions = new StatusLabel("dimensions");
    StatusLabel *position = new StatusLabel("position");
    StatusLabel *color = new StatusLabel("color");
    QObject::connect(function, SIGNAL(newParameter(QString)), parameter, SLOT(setText(QString)));
    QObject::connect(function, SIGNAL(newHash(QString)), hash, SLOT(setText(QString)));
    QObject::connect(function, SIGNAL(newDimensions(QString)), dimensions, SLOT(setText(QString)));
    QObject::connect(matrixViewer, SIGNAL(newPosition(QString)), position, SLOT(setText(QString)));
    QObject::connect(matrixViewer, SIGNAL(newColor(QString)), color, SLOT(setText(QString)));
    QStatusBar *statusBar = new QStatusBar();
    statusBar->addPermanentWidget(parameter);
    statusBar->addPermanentWidget(new QWidget(), 1);
    statusBar->addPermanentWidget(hash);
    statusBar->addPermanentWidget(dimensions);
    statusBar->addPermanentWidget(position);
    statusBar->addPermanentWidget(color);

    QMainWindow mainWindow;
    mainWindow.setCentralWidget(centralWidget);
    mainWindow.setMenuBar(menuBar);
    mainWindow.setStatusBar(statusBar);
    mainWindow.setWindowTitle("Likely Dream");
    mainWindow.resize(800,600);
    mainWindow.show();

    return application.exec(); }

#include "dream.moc"
