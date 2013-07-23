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

#include <QApplication>
#include <QDebug>
#include <QDragEnterEvent>
#include <QLabel>
#include <QMainWindow>
#include <QMimeData>
#include <QSizePolicy>

class ImageViewer : public QLabel
{
    Q_OBJECT
    QImage src;

public:
    explicit ImageViewer(QWidget *parent = 0)
        : QLabel(parent)
    {
        setAcceptDrops(true);
        setAlignment(Qt::AlignCenter);
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        setText("<b>Drag and Drop Image Here</b>");
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

    QMainWindow mainWindow;
    mainWindow.setCentralWidget(new ImageViewer());
    mainWindow.setWindowTitle("Likely Creator");
    mainWindow.show();

    return application.exec();
}

#include "create.moc"
