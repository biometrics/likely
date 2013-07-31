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

using namespace likely;

class Dataset : public QObject
{
    Q_OBJECT
    Matrix matrix;

public slots:
    void setDataset(const QImage &image)
    {
        matrix = Matrix(3, image.width(), image.height(), 1, likely_hash_u8);
        memcpy(matrix.data, image.constBits(), matrix.bytes());
        emit newImage(image);
        emit newDatasetInfo(QString("%1 %2x%3x%4x%5").arg(likely_hash_to_string(matrix.hash),
                                                          QString::number(matrix.channels),
                                                          QString::number(matrix.rows),
                                                          QString::number(matrix.columns),
                                                          QString::number(matrix.frames)));
    }

    void setDataset(QAction *action)
    {
        setDataset(QImage(action->data().toString()).convertToFormat(QImage::Format_RGB888));
    }

signals:
    void newImage(QImage image);
    void newDatasetInfo(QString info);
};

class DatasetViewer : public QLabel
{
    Q_OBJECT
    QImage src;
    int zoomLevel;

public:
    explicit DatasetViewer(QWidget *parent = 0)
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
            emit newDataset(qvariant_cast<QImage>(mimeData->imageData()));
        } else if (mimeData->hasUrls()) {
            foreach (const QUrl &url, mimeData->urls()) {
                if (!url.isValid()) continue;
                const QString localFile = url.toLocalFile();
                if (localFile.isNull()) continue;
                emit newDataset(QImage(localFile));
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
    void newDataset(QImage image);
};

class Engine : public QObject
{
    Q_OBJECT
    QImage input;
    int param;
    likely_unary_function function = NULL;

public:
    Engine(QObject *parent = 0)
        : QObject(parent)
    {
        setParam(0);
    }

public slots:
    void setInput(const QImage &input)
    {
        this->input = input;
        // TODO: process image and emit result
    }

    void setParam(int param)
    {
        this->param = param;
        function = likely_make_unary_function(qPrintable(QString::number(param/10.0) + "*src"));
        qDebug() << param/10.0;
        setInput(input);
    }
};

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    Dataset *dataset = new Dataset();
    DatasetViewer *datasetViewer = new DatasetViewer();
    QObject::connect(dataset, SIGNAL(newImage(QImage)), datasetViewer, SLOT(setImage(QImage)));
    QObject::connect(datasetViewer, SIGNAL(newDataset(QImage)), dataset, SLOT(setDataset(QImage)));

    QMenu *datasetsMenu = new QMenu("Datasets");
    foreach (const QString &fileName, QDir(":/img").entryList(QDir::Files, QDir::Name)) {
        const QString filePath = ":/img/"+fileName;
        QAction *potentialDataset = new QAction(QIcon(filePath), QFileInfo(filePath).baseName(), datasetsMenu);
        potentialDataset->setData(filePath);
        potentialDataset->setShortcut(QKeySequence("Ctrl+"+fileName.mid(0, 1)));
        datasetsMenu->addAction(potentialDataset);
    }
    QObject::connect(datasetsMenu, SIGNAL(triggered(QAction*)), dataset, SLOT(setDataset(QAction*)));

    QMenu *viewMenu = new QMenu("View");
    QAction *zoomIn = new QAction("Zoom In", viewMenu);
    QAction *zoomOut = new QAction("Zoom Out", viewMenu);
    zoomIn->setShortcut(QKeySequence("Ctrl++"));
    zoomOut->setShortcut(QKeySequence("Ctrl+-"));
    viewMenu->addAction(zoomIn);
    viewMenu->addAction(zoomOut);
    QObject::connect(zoomIn, SIGNAL(triggered()), datasetViewer, SLOT(zoomIn()));
    QObject::connect(zoomOut, SIGNAL(triggered()), datasetViewer, SLOT(zoomOut()));

    QMenuBar *menuBar = new QMenuBar();
    menuBar->addMenu(datasetsMenu);
    menuBar->addMenu(viewMenu);

    QSlider *slider = new QSlider(Qt::Horizontal);
    slider->setMinimum(0);
    slider->setMaximum(20);

    QVBoxLayout *centralWidgetLayout = new QVBoxLayout();
    QScrollArea *datasetViewerScrollArea = new QScrollArea();
    datasetViewerScrollArea->setAlignment(Qt::AlignCenter);
    datasetViewerScrollArea->setFocusPolicy(Qt::WheelFocus);
    datasetViewerScrollArea->setFrameShape(QFrame::NoFrame);
    datasetViewerScrollArea->setWidget(datasetViewer);
    centralWidgetLayout->addWidget(datasetViewerScrollArea);
    centralWidgetLayout->addWidget(slider);

    QWidget *centralWidget = new QWidget();
    centralWidget->setLayout(centralWidgetLayout);

    QLabel *datasetInfo = new QLabel;
    datasetInfo->setAlignment(Qt::AlignRight);
    datasetInfo->setToolTip("Matrix hash and dimensions");
    QObject::connect(dataset, SIGNAL(newDatasetInfo(QString)), datasetInfo, SLOT(setText(QString)));
    QStatusBar *statusBar = new QStatusBar();
    statusBar->addPermanentWidget(datasetInfo, 1);

    Engine *engine = new Engine();
    QObject::connect(datasetViewer, SIGNAL(newDataset(QImage)), engine, SLOT(setInput(QImage)));
    QObject::connect(slider, SIGNAL(valueChanged(int)), engine, SLOT(setParam(int)));

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
