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
#include <lua.hpp>
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
    void dragEnterEvent(QDragEnterEvent *e) { e->accept(); if (e->mimeData()->hasUrls()) e->acceptProposedAction(); }
    void dropEvent(QDropEvent *e)
      { e->accept(); e->acceptProposedAction();
        const QMimeData *md = e->mimeData();
        if (md->hasUrls())
          { foreach (const QUrl &url, md->urls())
              { if (!url.isValid()) continue;
                const QString file = url.toLocalFile();
                if (file.isNull()) continue;
                QAction *action = new QAction(this);
                action->setData(file);
                emit newMatrix(action);
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
    void newMatrix(QAction*);
    void newPosition(QString);
    void newColor(QString); };

class ErrorHandler : public QObject
  { Q_OBJECT
    static ErrorHandler *errorHandler;
    ErrorHandler() : QObject(NULL) { likely_set_error_callback(likely_error_handler); }
    void setError(const QString &error) { emit newError("<font color=\"red\">"+error+"</font>"); }
    static void likely_error_handler(const char *error) { get()->setError(error); }
public:
    static ErrorHandler *get() { if (!errorHandler) errorHandler = new ErrorHandler(); return errorHandler; }
signals:
    void newError(QString); };
ErrorHandler *ErrorHandler::errorHandler = NULL;

class Editor : public QTextEdit
  { Q_OBJECT
    QString sourceFileName;
    QSettings settings;
    lua_State *L;
public:
    Editor(QWidget *p = 0) : QTextEdit(p), L(NULL)
      { connect(this, SIGNAL(textChanged()), this, SLOT(exec()));
        connect(ErrorHandler::get(), SIGNAL(newError(QString)), this, SIGNAL(newInfo(QString)));
        setText(settings.value("source").toString()); }
    static lua_State *exec(const QString &source, bool *error)
      { lua_State *L = luaL_newstate();
        luaL_openlibs(L);
        luaL_requiref(L, "likely", luaopen_likely, 1);
        lua_pop(L, 1);
        *error = luaL_dostring(L, likely_standard_library()) ||
                 luaL_dostring(L, qPrintable(source));
        return L; }
public slots:
    void setSource(QAction *a)
      { if (a->text() == "Open...") {
            sourceFileName = QFileDialog::getOpenFileName(NULL, "Open Source File");
            if (!sourceFileName.isEmpty())
              { QFile file(sourceFileName);
                file.open(QFile::ReadOnly | QFile::Text);
                setText(QString::fromLocal8Bit(file.readAll()));
                file.close(); } }
        else
          { if (sourceFileName.isEmpty() || (a->text() == "Save As..."))
                sourceFileName = QFileDialog::getSaveFileName(NULL, "Save Source File");
            if (!sourceFileName.isEmpty())
              { QFile file(sourceFileName);
                file.open(QFile::WriteOnly | QFile::Text);
                file.write(toPlainText().toLocal8Bit());
                file.close(); } } }
    void exec()
      { emit newInfo("");
        if (L) lua_close(L);
        const QString source = toPlainText();
        bool error;
        L = exec(source, &error);
        if (error) {
            emit newInfo(lua_tostring(L, -1));
            lua_pop(L, 1);
        }
        settings.setValue("source", source); }
signals:
    void newInfo(QString); };

class StatusLabel : public QLabel
  { Q_OBJECT
    QString description;
public:
    StatusLabel(const QString &description_) : description(description_)
      { setToolTip(description);
        setDescription(); }
public slots:
    void setText(const QString &text)
      { if (text.isEmpty()) setDescription();
        else { setEnabled(true); QLabel::setText(text); } }
private:
    void setDescription() { setEnabled(false); QLabel::setText(description); }
    void resizeEvent(QResizeEvent *e) { e->accept(); setMinimumWidth(width()); } };

int main(int argc, char *argv[])
  { for (int i=1; i<argc; i++)
      { QString source;
        if (QFileInfo(argv[i]).exists())
          { QFile file(argv[i]);
            file.open(QFile::ReadOnly | QFile::Text);
            source = file.readAll();
            file.close();
            if (source.startsWith("#!")) source = source.mid(source.indexOf('\n')+1); }
        else
            source = argv[i];
        bool error;
        Editor::exec(source, &error);
    }
    if (argc > 1) return 0;
    QApplication::setApplicationName("Dream");
    QApplication::setOrganizationName("Likely");
    QApplication::setOrganizationDomain("liblikely.org");
    QApplication application(argc, argv);
    QMenu *sourceMenu = new QMenu("Source");
    QAction *openSource = new QAction("Open...", sourceMenu);
    QAction *saveSource = new QAction("Save", sourceMenu);
    QAction *saveSourceAs = new QAction("Save As...", sourceMenu);
    openSource->setShortcut(QKeySequence("Ctrl+O"));
    saveSource->setShortcut(QKeySequence("Ctrl+S"));
    saveSourceAs->setShortcut(QKeySequence("Ctrl+Shift+S"));
    sourceMenu->addAction(openSource);
    sourceMenu->addAction(saveSource);
    sourceMenu->addAction(saveSourceAs);
    Editor *editor = new Editor();
    MatrixViewer *matrixViewer = new MatrixViewer();
    QObject::connect(sourceMenu, SIGNAL(triggered(QAction*)), editor, SLOT(setSource(QAction*)));
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
    menuBar->addMenu(sourceMenu);
    menuBar->addMenu(viewMenu);
    QGridLayout *centralWidgetLayout = new QGridLayout();
    QScrollArea *matrixViewerScrollArea = new QScrollArea();
    matrixViewerScrollArea->setAlignment(Qt::AlignCenter);
    matrixViewerScrollArea->setFrameShape(QFrame::NoFrame);
    matrixViewerScrollArea->setWidget(matrixViewer);
    centralWidgetLayout->addWidget(editor, 0, 0);
    centralWidgetLayout->addWidget(matrixViewerScrollArea, 0, 1, 2, 1);
    QWidget *centralWidget = new QWidget();
    centralWidget->setLayout(centralWidgetLayout);
    StatusLabel *info = new StatusLabel("info");
    StatusLabel *hash = new StatusLabel("hash");
    StatusLabel *dimensions = new StatusLabel("dimensions");
    StatusLabel *position = new StatusLabel("position");
    StatusLabel *color = new StatusLabel("color");
    QObject::connect(editor, SIGNAL(newInfo(QString)), info, SLOT(setText(QString)));
    QObject::connect(matrixViewer, SIGNAL(newPosition(QString)), position, SLOT(setText(QString)));
    QObject::connect(matrixViewer, SIGNAL(newColor(QString)), color, SLOT(setText(QString)));
    QStatusBar *statusBar = new QStatusBar();
    statusBar->addPermanentWidget(info);
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
