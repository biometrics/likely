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
    int zoomLevel;

public:
    explicit MatrixViewer(QWidget *parent = 0)
        : QLabel(parent)
    {
        zoomLevel = 0;
        setAcceptDrops(true);
        setAlignment(Qt::AlignCenter);
        setFocusPolicy(Qt::WheelFocus);
        setFrameShape(QFrame::StyledPanel);
        setText("<b>Drag and Drop Image Here</b>");
        resize(512, 512);
    }

public slots:
    void setImage(const QImage &image)
    {
        src = image;
        updatePixmap();
        setFrameShape(QFrame::NoFrame);
    }

    void zoomIn()
    {
        zoomLevel++;
        if (zoomLevel > 4) zoomLevel = 4;
        else               updatePixmap();
    }

    void zoomOut()
    {
        zoomLevel--;
        if (zoomLevel < -4) zoomLevel = -4;
        else                updatePixmap();
    }

private slots:
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

    void updatePixmap()
    {
        if (src.isNull()) return;
        const QSize newSize = src.size() * pow(2, zoomLevel);
        setPixmap(QPixmap::fromImage(src.scaled(newSize, Qt::KeepAspectRatio)));
        resize(newSize);
    }

signals:
    void newMatrix(QImage image);
};

class Engine : public QObject
{
    Q_OBJECT
    likely_matrix matrix;
    likely_unary_function function = NULL;

public:
    Engine(QObject *parent = 0)
        : QObject(parent)
    {
        likely_matrix_initialize(&matrix);
        setParam(0);
    }

public slots:
    void setMatrix(const QImage &image)
    {
        likely_free(&matrix);
        likely_matrix_initialize(&matrix, 3, image.width(), image.height(), 1, likely_hash_u8);
        likely_allocate(&matrix);
        memcpy(matrix.data, image.constBits(), likely_bytes(&matrix));
        emit newMatrixView(image);
        emit newMatrixInfo(QString("%1 %2x%3x%4x%5").arg(likely_hash_to_string(matrix.hash),
                                                         QString::number(matrix.channels),
                                                         QString::number(matrix.rows),
                                                         QString::number(matrix.columns),
                                                         QString::number(matrix.frames)));
    }

    void setMatrix(QAction *action)
    {
        setMatrix(QImage(action->data().toString()).convertToFormat(QImage::Format_RGB888));
    }

    void setParam(int param)
    {
        function = likely_make_unary_function(qPrintable(QString::number(param/10.0) + "*src"));
        qDebug() << param/10.0;
    }

signals:
    void newMatrixView(QImage image);
    void newMatrixInfo(QString info);
};

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    Engine *engine = new Engine();
    MatrixViewer *matrixViewer = new MatrixViewer();
    QObject::connect(engine, SIGNAL(newMatrixView(QImage)), matrixViewer, SLOT(setImage(QImage)));
    QObject::connect(matrixViewer, SIGNAL(newMatrix(QImage)), engine, SLOT(setMatrix(QImage)));

    QMenu *matricesMenu = new QMenu("Matrices");
    foreach (const QString &fileName, QDir(":/img").entryList(QDir::Files, QDir::Name)) {
        const QString filePath = ":/img/"+fileName;
        QAction *potentialMatrix = new QAction(QIcon(filePath), QFileInfo(filePath).baseName(), matricesMenu);
        potentialMatrix->setData(filePath);
        potentialMatrix->setShortcut(QKeySequence("Ctrl+"+fileName.mid(0, 1)));
        matricesMenu->addAction(potentialMatrix);
    }
    QObject::connect(matricesMenu, SIGNAL(triggered(QAction*)), engine, SLOT(setMatrix(QAction*)));

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
    menuBar->addMenu(matricesMenu);
    menuBar->addMenu(viewMenu);

    QSlider *slider = new QSlider(Qt::Horizontal);
    slider->setMinimum(0);
    slider->setMaximum(20);
    QObject::connect(slider, SIGNAL(valueChanged(int)), engine, SLOT(setParam(int)));

    QVBoxLayout *centralWidgetLayout = new QVBoxLayout();
    QScrollArea *matrixViewerScrollArea = new QScrollArea();
    matrixViewerScrollArea->setAlignment(Qt::AlignCenter);
    matrixViewerScrollArea->setFocusPolicy(Qt::WheelFocus);
    matrixViewerScrollArea->setFrameShape(QFrame::NoFrame);
    matrixViewerScrollArea->setWidget(matrixViewer);
    centralWidgetLayout->addWidget(matrixViewerScrollArea);
    centralWidgetLayout->addWidget(slider);

    QWidget *centralWidget = new QWidget();
    centralWidget->setLayout(centralWidgetLayout);

    QLabel *matrixInfo = new QLabel;
    matrixInfo->setAlignment(Qt::AlignRight);
    matrixInfo->setToolTip("Matrix hash and dimensions");
    QObject::connect(engine, SIGNAL(newMatrixInfo(QString)), matrixInfo, SLOT(setText(QString)));
    QStatusBar *statusBar = new QStatusBar();
    statusBar->addPermanentWidget(matrixInfo, 1);

    QMainWindow mainWindow;
    mainWindow.setCentralWidget(centralWidget);
    mainWindow.setMenuBar(menuBar);
    mainWindow.setStatusBar(statusBar);
    mainWindow.setWindowTitle("Likely Creator");
    mainWindow.resize(800,600);
    mainWindow.show();

    return application.exec();
}

#include "create.moc"
