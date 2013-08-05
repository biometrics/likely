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
{
    Q_OBJECT
    QImage src;
    int zoomLevel = 0;

public:
    explicit MatrixViewer(QWidget *parent = 0)
        : QLabel(parent)
    {
        setAcceptDrops(true);
        setAlignment(Qt::AlignCenter);
        setMouseTracking(true);
        updatePixmap();
    }

public slots:
    void setImage(const QImage &image) { src = image; updatePixmap(); }
    void zoomIn() { if (++zoomLevel > 4) zoomLevel = 4; else updatePixmap(); }
    void zoomOut() { if (--zoomLevel < -4) zoomLevel = -4; else updatePixmap(); }

private:
    void dragEnterEvent(QDragEnterEvent *event)
    {
        event->accept();
        if (event->mimeData()->hasUrls() || event->mimeData()->hasImage())
            event->acceptProposedAction();
    }

    void dropEvent(QDropEvent *event)
    {
        event->accept();
        event->acceptProposedAction();
        const QMimeData *mimeData = event->mimeData();
        if (mimeData->hasImage()) {
            emit newMatrix(qvariant_cast<QImage>(mimeData->imageData()));
        } else if (mimeData->hasUrls()) {
            foreach (const QUrl &url, mimeData->urls()) {
                if (!url.isValid()) continue;
                const QString localFile = url.toLocalFile();
                if (localFile.isNull()) continue;
                emit newMatrix(QImage(localFile));
                break;
            }
        }
    }

    void leaveEvent(QEvent *event) { event->accept(); queryPoint(QPoint(-1, -1)); }
    void mouseMoveEvent(QMouseEvent *mouseEvent) { mouseEvent->accept(); queryPoint(mouseEvent->pos() / pow(2, zoomLevel)); }

    void updatePixmap()
    {
        if (src.isNull()) {
            clear();
            setText("<b>Drag and Drop Image Here</b>");
            resize(512, 512);
            queryPoint(QPoint(-1, -1));
            setFrameShape(QFrame::StyledPanel);
        } else {
            const QSize newSize = src.size() * pow(2, zoomLevel);
            setPixmap(QPixmap::fromImage(src.scaled(newSize, Qt::KeepAspectRatio)));
            resize(newSize);
            queryPoint(mapFromGlobal(QCursor::pos()) / pow(2, zoomLevel));
            setFrameShape(QFrame::NoFrame);
        }
    }

    void queryPoint(const QPoint &point)
    {
        if (src.rect().contains(point)) {
            const QRgb pixel = src.pixel(point);
            emit newPosition(QString("%1,%2")
                             .arg(QString::number(point.x()),
                                  QString::number(point.y())));
            emit newColor(QString("<font color=\"red\">%1</font>,<font color=\"green\">%2</font>,<font color=\"blue\">%3</font>")
                          .arg(QString::number(qRed(pixel)),
                               QString::number(qGreen(pixel)),
                               QString::number(qBlue(pixel))));
        } else {
            emit newPosition("");
            emit newColor("");
        }
    }

signals:
    void newMatrix(QImage image);
    void newPosition(QString position);
    void newColor(QString color);
};

class Parameter : public QLabel
{
    Q_OBJECT
    QString name;
    double value = 0;
    int wheelRemainder = 0;

public:
    void reset(const QString &name, const QString &value)
    {
        this->name = name;
        this->value = value.toDouble();
        updateValue();
    }

    QString argument() const { return QString::number(value); }

private:
    void updateValue() { setText("<font color=\"blue\">"+QString::number(value)+"</font>"); }

    void enterEvent(QEvent *event)
    {
        static QPixmap cursor;
        if (cursor.isNull()) {
            cursor = QPixmap(32, 32);
            QPainter painter(&cursor);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setBrush(Qt::red);
            painter.setPen(Qt::red);
            painter.drawEllipse(-3, -3, 6, 6);
            painter.end();
            QBitmap mask(32, 32);
            mask.clear();
            painter.begin(&mask);
            painter.setRenderHint(QPainter::Antialiasing, false);
            painter.setBrush(Qt::color1);
            painter.setPen(Qt::color1);
            painter.drawEllipse(-3, -3, 6, 6);
            painter.end();
            cursor.setMask(mask);
        }

        event->accept();
        setCursor(QCursor(cursor, 0, 0));
        emit newParameter(name+"="+QString::number(value));
    }

    void leaveEvent(QEvent *event)
    {
        event->accept();
        unsetCursor();
        emit newParameter(QString());
    }

    void wheelEvent(QWheelEvent *wheelEvent)
    {
        wheelEvent->accept();
        wheelRemainder += wheelEvent->angleDelta().x() - wheelEvent->angleDelta().y();
        const int delta = wheelRemainder / 120;
        if (delta == 0) return;

        value += delta;
        wheelRemainder = wheelRemainder % 120;
        updateValue();
        emit newParameter(value);
        emit newParameter(name+"="+QString::number(value));
    }

signals:
    void newParameter(QString parameter);
    void newParameter(double value);
};

class Function : public QWidget
{
    Q_OBJECT
    static QStringListModel *functionNames;

    QHBoxLayout *layout;
    QComboBox *functionName;
    QList<Parameter*> parameters;

    likely_matrix input;
    likely_unary_function function = NULL;

public:
    Function(QWidget *parent = 0)
        : QWidget(parent)
    {
        if (functionNames == NULL) {
            const char **function_names;
            int num_functions;
            likely_functions(&function_names, &num_functions);
            QStringList strings; strings.reserve(num_functions);
            for (int i=0; i<num_functions; i++)
                strings.append(function_names[i]);
            functionNames = new QStringListModel(strings);
        }

        likely_matrix_initialize(&input);

        functionName = new QComboBox(this);
        functionName->setEditable(true);
        functionName->setFrame(false);
        functionName->setInsertPolicy(QComboBox::NoInsert);

        layout = new QHBoxLayout(this);
        layout->addWidget(functionName);

        connect(functionName, SIGNAL(currentIndexChanged(QString)), this, SLOT(updateParameters(QString)));
        functionName->setModel(functionNames);
    }

public slots:
    void setInput(const QImage &image)
    {
        likely_free(&input);
        if (!image.isNull()) {
            likely_matrix_initialize(&input, likely_hash_u8, 3, image.width(), image.height(), 1);
            likely_allocate(&input);
            memcpy(input.data, image.constBits(), likely_bytes(&input));
        }
        compute();
    }

    void setInput(QAction *action)
    {
        QString file;
        if      (action->text() == "New...")  file = "";
        else if (action->text() == "Open...") file = QFileDialog::getOpenFileName(NULL, "Open File");
        else                                  file = action->data().toString();
        setInput(file.isEmpty() ? QImage() : QImage(file).convertToFormat(QImage::Format_RGB888));
    }

private:
    void compute()
    {
        if (input.data == NULL) {
            emit newMatrixView(QImage());
            emit newHash(QString());
            emit newDimensions(QString());
            return;
        }

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
                                QString::number(input.frames)));
    }

private slots:
    void updateParameters(const QString &function)
    {
        const char **parameter_names, **defaults;
        int num_parameters;
        likely_parameters(qPrintable(function), &parameter_names, &num_parameters, &defaults);
        QStringList strings; strings.reserve(num_parameters);
        for (int i=0; i<num_parameters; i++)
            strings.append(parameter_names[i]);

        while (parameters.size() > num_parameters) {
            layout->removeWidget(parameters.last());
            delete parameters.takeLast();
        }
        while (parameters.size() < num_parameters) {
            parameters.append(new Parameter());
            layout->addWidget(parameters.last());
            connect(parameters.last(), SIGNAL(newParameter(QString)), this, SIGNAL(newParameter(QString)));
            connect(parameters.last(), SIGNAL(newParameter(double)), this, SLOT(compile()));
        }

        for (int i=0; i<num_parameters; i++)
            parameters[i]->reset(strings[i], defaults[i]);
        compile();
    }

    void compile()
    {
        QStringList parameterValues; parameterValues.reserve(parameters.size());
        foreach (const Parameter *parameter, parameters)
            parameterValues.append(parameter->argument());
        const QString description = functionName->currentText()+"("+parameterValues.join(',')+")";
        function = likely_make_unary_function(qPrintable(description));
        compute();
    }

signals:
    void newMatrixView(QImage image);
    void newHash(QString hash);
    void newDimensions(QString dimensions);
    void newParameter(QString parameter);
};

QStringListModel *Function::functionNames = NULL;

class StatusLabel : public QLabel
{
    Q_OBJECT
    QString description;

public:
    StatusLabel(const QString &description)
    {
        this->description = description;
        setToolTip(description);
        setDescription();
    }

public slots:
    void setText(const QString &text)
    {
        if (text.isEmpty()) setDescription();
        else { setEnabled(true); QLabel::setText(text); }
    }

private:
    void setDescription()
    {
        setEnabled(false);
        QLabel::setText(description);
    }

    void resizeEvent(QResizeEvent *resizeEvent)
    {
        resizeEvent->accept();
        setMinimumWidth(width());
    }
};

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    QMenu *fileMenu = new QMenu("File");
    QAction *newFile = new QAction("New...", fileMenu);
    QAction *openFile = new QAction("Open...", fileMenu);
    newFile->setShortcut(QKeySequence("Ctrl+N"));
    openFile->setShortcut(QKeySequence("Ctrl+O"));
    fileMenu->addAction(newFile);
    fileMenu->addAction(openFile);
    fileMenu->addSeparator();
    foreach (const QString &fileName, QDir(":/img").entryList(QDir::Files, QDir::Name)) {
        const QString filePath = ":/img/"+fileName;
        QAction *potentialFile = new QAction(QIcon(filePath), QFileInfo(filePath).baseName(), fileMenu);
        potentialFile->setData(filePath);
        potentialFile->setShortcut(QKeySequence("Ctrl+"+fileName.mid(0, 1)));
        fileMenu->addAction(potentialFile);
    }

    Function *engine = new Function();
    MatrixViewer *matrixViewer = new MatrixViewer();
    QObject::connect(engine, SIGNAL(newMatrixView(QImage)), matrixViewer, SLOT(setImage(QImage)));
    QObject::connect(matrixViewer, SIGNAL(newMatrix(QImage)), engine, SLOT(setInput(QImage)));
    QObject::connect(fileMenu, SIGNAL(triggered(QAction*)), engine, SLOT(setInput(QAction*)));

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
    matrixViewerScrollArea->setFocusPolicy(Qt::WheelFocus);
    matrixViewerScrollArea->setFrameShape(QFrame::NoFrame);
    matrixViewerScrollArea->setWidget(matrixViewer);
    centralWidgetLayout->addWidget(engine, 0, 0);
    centralWidgetLayout->addWidget(matrixViewerScrollArea, 0, 1, 2, 1);
    centralWidgetLayout->setRowStretch(1, 1);

    QWidget *centralWidget = new QWidget();
    centralWidget->setLayout(centralWidgetLayout);

    StatusLabel *parameter = new StatusLabel("parameter");
    StatusLabel *hash = new StatusLabel("hash");
    StatusLabel *dimensions = new StatusLabel("dimensions");
    StatusLabel *position = new StatusLabel("position");
    StatusLabel *color = new StatusLabel("color");
    QObject::connect(engine, SIGNAL(newParameter(QString)), parameter, SLOT(setText(QString)));
    QObject::connect(engine, SIGNAL(newHash(QString)), hash, SLOT(setText(QString)));
    QObject::connect(engine, SIGNAL(newDimensions(QString)), dimensions, SLOT(setText(QString)));
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

    return application.exec();
}

#include "dream.moc"
