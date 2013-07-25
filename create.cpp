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

class DatasetViewer : public QLabel
{
    Q_OBJECT
    QImage src;

public:
    explicit DatasetViewer(QWidget *parent = 0)
        : QLabel(parent)
    {
        setAcceptDrops(true);
        setAlignment(Qt::AlignCenter);
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        setText("<b>Drag and Drop Image Here</b>");
    }

public slots:
    void setDataset(QAction *action)
    {
        src = QImage(action->data().toString());
        updatePixmap();
        emit newDataset(src);
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
            src = qvariant_cast<QImage>(mimeData->imageData());
        } else if (event->mimeData()->hasUrls()) {
            foreach (const QUrl &url, mimeData->urls()) {
                if (!url.isValid()) continue;
                const QString localFile = url.toLocalFile();
                if (localFile.isNull()) continue;
                src = QImage(localFile);
                break;
            }
        }

        updatePixmap();
    }

    void resizeEvent(QResizeEvent *event)
    {
        event->accept();
        updatePixmap();
    }

    void updatePixmap()
    {
        if (src.isNull()) return;
        setPixmap(QPixmap::fromImage(src.scaled(size(), Qt::KeepAspectRatio)));
    }

signals:
    void newDataset(QImage dataset);
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

    DatasetViewer *datasetViewer = new DatasetViewer();

    QMenu *datasetsMenu = new QMenu("Datasets");
    foreach (const QString &fileName, QDir(":/img").entryList(QDir::Files, QDir::Name)) {
        const QString filePath = ":/img/"+fileName;
        QAction *potentialDataset = new QAction(QIcon(filePath), QFileInfo(filePath).baseName(), datasetsMenu);
        potentialDataset->setData(filePath);
        datasetsMenu->addAction(potentialDataset);
    }
    QObject::connect(datasetsMenu, SIGNAL(triggered(QAction*)), datasetViewer, SLOT(setDataset(QAction*)));

    QMenuBar *menuBar = new QMenuBar();
    menuBar->addMenu(datasetsMenu);

    QSlider *slider = new QSlider(Qt::Horizontal);
    slider->setMinimum(0);
    slider->setMaximum(20);

    QVBoxLayout *centralWidgetLayout = new QVBoxLayout();
    centralWidgetLayout->addWidget(datasetViewer);
    centralWidgetLayout->addWidget(slider);

    QWidget *centralWidget = new QWidget();
    centralWidget->setLayout(centralWidgetLayout);

    Engine *engine = new Engine();
    QObject::connect(datasetViewer, SIGNAL(newDataset(QImage)), engine, SLOT(setInput(QImage)));
    QObject::connect(slider, SIGNAL(valueChanged(int)), engine, SLOT(setParam(int)));

    QMainWindow mainWindow;
    mainWindow.setCentralWidget(centralWidget);
    mainWindow.setMenuBar(menuBar);
    mainWindow.setWindowTitle("Likely Creator");
    mainWindow.resize(800,600);
    mainWindow.show();

    return application.exec();
}

#include "create.moc"
