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

class Dataset : public QAction
{
    Q_OBJECT
    QString file;

public:
    Dataset(const QString &file, QWidget *parent = 0)
        : QAction(QIcon(file), QFileInfo(file).baseName(), parent)
    {
        this->file = file;
        connect(this, SIGNAL(triggered()), this, SLOT(select()));
    }

private slots:
    void select()
    {
        emit selected(file);
    }

signals:
    void selected(QString file);
};

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
    void setDataset(const QString &file)
    {
        src = QImage(file);
        updatePixmap();
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
};

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    DatasetViewer *datasetViewer = new DatasetViewer();

    QMenu *datasetsMenu = new QMenu("Datasets");
    foreach (const QString &file, QDir(":/img").entryList(QDir::Files, QDir::Name)) {
        Dataset *dataset = new Dataset(":/img/"+file);
        QObject::connect(dataset, SIGNAL(selected(QString)), datasetViewer, SLOT(setDataset(QString)));
        datasetsMenu->addAction(dataset);
    }

    QMenuBar *menuBar = new QMenuBar();
    menuBar->addMenu(datasetsMenu);

    QMainWindow mainWindow;
    mainWindow.setCentralWidget(datasetViewer);
    mainWindow.setMenuBar(menuBar);
    mainWindow.setWindowTitle("Likely Creator");
    mainWindow.show();

    return application.exec();
}

#include "create.moc"
