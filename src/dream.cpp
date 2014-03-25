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
#include <assert.h>
#include <likely.h>

class SyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
    QRegularExpression numbers, strings;
    QTextCharFormat markdownFont, numbersFont, stringsFont;
    bool commandMode = false;

public:
    SyntaxHighlighter(QTextDocument *parent)
        : QSyntaxHighlighter(parent)
    {
        numbers.setPattern("-?\\d*\\.?\\d+(?:[Ee][+-]?\\d+)?");
        strings.setPattern("\"[^\"]*+\"");
        markdownFont.setForeground(Qt::darkGray);
        numbersFont.setFontUnderline(true);
        numbersFont.setUnderlineStyle(QTextCharFormat::DotLine);
        stringsFont.setForeground(Qt::darkGreen);
    }

public slots:
    void setCommandMode(bool enabled)
    {
        commandMode = enabled;
        rehighlight();
    }

private:
    void highlightBlock(const QString &text)
    {
        if (commandMode) highlightHelp(text, numbers, numbersFont);
        highlightHelp(text, strings, stringsFont);
        highlightMarkdown(text, markdownFont);
    }

    void highlightHelp(const QString &text, const QRegularExpression &re, const QTextCharFormat &font)
    {
        if (re.pattern().isEmpty())
            return;
        QRegularExpressionMatch match = re.match(text);
        int index = match.capturedStart();
        while (index >= 0) {
            const int length = match.capturedLength();
            setFormat(index, match.capturedLength(), font);
            match = re.match(text, index + length);
            index = match.capturedStart();
        }
    }

    void highlightMarkdown(const QString &text, const QTextCharFormat &font)
    {
        const bool codeBlockMarker = text.startsWith("```");
        if (codeBlockMarker)
            setCurrentBlockState(previousBlockState() == 0);
        else
            setCurrentBlockState(previousBlockState() == -1
                                 ? (text.startsWith("(") ? 0 : 1)
                                 : previousBlockState());

        if (!text.startsWith("    ") && (currentBlockState() || codeBlockMarker)) {
            int start = -1, stop = -1;
            do {
                start = stop;
                stop = codeBlockMarker ? -1 : text.indexOf('`', start+1);
                if (start == -1) start = 0;
                if (stop == -1) stop = text.size();
                else            stop++;
                setFormat(start, stop - start, font);
                if (stop == text.size())
                    break;
                stop = text.indexOf('`', stop);
            } while (true);
        }
    }

    static QString getPattern(const QSet<QString> &values)
    {
        if (values.isEmpty()) return "";
        return "\\b(?:" + QStringList(values.toList()).join('|') + ")\\b";
    }
};

class Source : public QPlainTextEdit
{
    Q_OBJECT
    QString header, previousSource;
    QSettings settings;
    int wheelRemainderX = 0, wheelRemainderY = 0;

public:
    Source()
    {
        setLineWrapMode(QPlainTextEdit::NoWrap);
        connect(this, SIGNAL(textChanged()), this, SLOT(eval()));
    }

    void restore()
    {
        // Try to open the previous file
        if (fileMenu("Open Quiet"))
            return;

        const QString source = settings.value("source").toString();
        settings.setValue("source", QString()); // Start empty the next time if this source code crashes
        settings.sync();
        setPlainText(source);
    }

public slots:
    void setHeader(const QString &header)
    {
        this->header = header;
        eval();
    }

    bool fileMenu(const QString &action)
    {
        QString sourceFileName = settings.value("sourceFileName").toString();
        QString sourceFilePath = settings.value("sourceFilePath").toString();

        // Start empty the next time if this source code crashes
        settings.setValue("sourceFileName", QString());
        settings.sync();

        if (action.startsWith("New")) {
            sourceFileName.clear();
            setPlainText("");
        } else if (action.startsWith("Open")) {
            if (!action.endsWith("Quiet"))
                sourceFileName = QFileDialog::getOpenFileName(this, "Open Source File", sourceFilePath);
            if (!sourceFileName.isEmpty()) {
                QFile file(sourceFileName);
                if (file.open(QFile::ReadOnly | QFile::Text))
                    setPlainText(QString::fromLocal8Bit(file.readAll()));
                else
                    sourceFileName.clear();
            }
        } else {
            if (sourceFileName.isEmpty() || action.startsWith("Save As"))
                sourceFileName = QFileDialog::getSaveFileName(this, "Save Source File", sourceFilePath);
            if (!sourceFileName.isEmpty()) {
                QFile file(sourceFileName);
                file.open(QFile::WriteOnly | QFile::Text);
                file.write(toPlainText().toLocal8Bit());
            }
        }

        if (!sourceFileName.isEmpty())
            sourceFilePath = QFileInfo(sourceFileName).filePath();

        settings.setValue("sourceFileName", sourceFileName);
        settings.setValue("sourceFilePath", sourceFilePath);
        settings.sync();
        emit newFileName(sourceFileName.isEmpty() ? "Likely" : QFileInfo(sourceFileName).fileName());
        return !sourceFileName.isEmpty();
    }

    void fileMenu(QAction *a)
    {
        fileMenu(a->text());
    }

    void commandsMenu(QAction *a)
    {
        if (a->text().startsWith("Increment") ||
            a->text().startsWith("Decrement")) {
            QTextCursor tc = textCursor();
            bool ok;
            int n = selectNumber(tc, &ok);
            if (ok) {
                int scale = 0, add = 0;
                if      (a->text() == "Increment")     add = 1;
                else if (a->text() == "Decrement")     add = -1;
                else if (a->text() == "Increment 10x") scale = 1;
                else if (a->text() == "Decrement 10x") scale = -1;
                tc.insertText(QString::number(qRound(qPow(10,scale)*n+add)));
            }
        }
    }

private:
    int selectNumber(QTextCursor &tc, bool *ok)
    {
        int n = selectValUnderCursor(tc, ok);
        if (!*ok) {
            tc.movePosition(QTextCursor::NextWord);
            n = selectValUnderCursor(tc, ok);
        }
        if (!*ok) {
            tc.movePosition(QTextCursor::PreviousWord, QTextCursor::MoveAnchor, 2);
            n = selectValUnderCursor(tc, ok);
        }
        return n;
    }

    void wheelEvent(QWheelEvent *e)
    {
        if (e->modifiers() != Qt::ControlModifier)
            return QPlainTextEdit::wheelEvent(e);

        const int deltaY =                    getIncrement(e->angleDelta().y(), wheelRemainderY, wheelRemainderX);
        const int deltaX = (deltaY != 0 ? 0 : getIncrement(e->angleDelta().x(), wheelRemainderX, wheelRemainderY));
        if ((deltaX == 0) && (deltaY == 0))
            return;

        QTextCursor tc = cursorForPosition(e->pos());
        bool ok;
        int n = selectNumber(tc, &ok);
        if (ok) tc.insertText(QString::number(qRound(qPow(10, deltaX) * (n + deltaY))));
    }

    static int getIncrement(int delta, int &remainder, int &remainderOther)
    {
        remainder += delta;
        const int increment = remainder / 120;
        if (increment != 0) {
            remainder = remainder % 120;
            remainderOther = 0;
        }
        return increment;
    }

    static int selectValUnderCursor(QTextCursor &tc, bool *ok)
    {
        QTextCursor tcCopy(tc);
        tcCopy.select(QTextCursor::WordUnderCursor);

        // Does the number start with a negative sign?
        QTextCursor tcNegative(tcCopy);
        tcNegative.movePosition(QTextCursor::StartOfWord);
        tcNegative.movePosition(QTextCursor::PreviousCharacter);
        tcNegative.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        tcNegative.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
        if (tcNegative.selectedText().startsWith('-'))
            tcCopy = tcNegative;

        const int val = tcCopy.selectedText().toInt(ok);
        if (*ok) tc = tcCopy;
        return val;
    }

private slots:
    void eval()
    {
        // This check needed because syntax highlighting triggers a textChanged() signal
        const QString source = header + toPlainText();
        if (source == previousSource) return;
        else                          previousSource = source;

        QElapsedTimer elapsedTimer;
        elapsedTimer.start();
        likely_const_ast asts = likely_asts_from_string(qPrintable(source), true);
        if (asts) {
            likely_env env = likely_new_jit();
            for (size_t i=0; i<asts->num_atoms; i++) {
                likely_const_mat result = likely_eval(asts->atoms[i], env);
                if (result) {
                    emit newResult(result);
                    likely_release(result);
                }
            }
            likely_release_env(env);
            likely_release_ast(asts);
            const qint64 nsec = elapsedTimer.nsecsElapsed();
            emit newStatus(QString("Evaluation Speed: %1 Hz").arg(nsec == 0 ? QString("infinity") : QString::number(double(1E9)/nsec, 'g', 3)));
        }
        emit finishedEval();
        settings.setValue("source", toPlainText());
    }

signals:
    void finishedEval();
    void newFileName(QString);
    void newResult(likely_const_mat result);
    void newStatus(QString);
};

class Matrix : public QFrame
{
    Q_OBJECT

    QImage src;
    QLabel *image;
    QWidget *caption;
    QLabel *type;
    QLineEdit *name;
    QLabel *definition;
    QHBoxLayout *captionLayout;
    QVBoxLayout *layout;

public:
    Matrix()
    {
        setFrameStyle(QFrame::Panel | QFrame::Raised);
        setLineWidth(2);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        image = new QLabel(this);
        image->setAlignment(Qt::AlignCenter);
        image->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
        caption = new QWidget(this);
        type = new QLabel(this);
        name = new QLineEdit(this);
        definition = new QLabel(this);
        definition->setWordWrap(true);
        definition->setVisible(false);
        captionLayout = new QHBoxLayout(caption);
        captionLayout->addWidget(type);
        captionLayout->addWidget(name);
        captionLayout->setContentsMargins(0, 0, 0, 0);
        captionLayout->setSpacing(3);
        layout = new QVBoxLayout(this);
        layout->addWidget(image);
        layout->addWidget(name);
        layout->addWidget(caption);
        layout->addWidget(definition);
        layout->setContentsMargins(3, 3, 3, 3);
        layout->setSpacing(3);
        setLayout(layout);
        connect(name, SIGNAL(textChanged(QString)), this, SLOT(updateDefinition()));
    }

    void show(likely_const_mat m)
    {
        if (likely_elements(m) <= 16) {
            src = QImage();

            likely_mat str = likely_to_string(m, true);
            type->setText((const char*) str->data);
            likely_release(str);
        } else {
            double min, max;
            likely_const_mat rendered = likely_render(m, &min, &max);
            src = QImage(rendered->data, rendered->columns, rendered->rows, 3*rendered->columns, QImage::Format_RGB888).rgbSwapped();
            likely_release(rendered);

            likely_mat str = likely_type_to_string(m->type);
            type->setText(QString("%1x%2x%3x%4 %5 [%6,%7]").arg(QString::number(m->channels),
                                                                QString::number(m->columns),
                                                                QString::number(m->rows),
                                                                QString::number(m->frames),
                                                                (const char*)str->data,
                                                                QString::number(min),
                                                                QString::number(max)));
            likely_release(str);
        }

        updatePixmap();
    }

    QString getDefinition() const
    {
        return definition->text();
    }

private:
    void resizeEvent(QResizeEvent *e)
    {
        QWidget::resizeEvent(e);
        updatePixmap();
    }

    void updatePixmap()
    {
        const bool visible = !src.isNull();
        image->setVisible(visible);
        if (visible) {
            const int width = qMin(image->size().width(), src.width());
            const int height = src.height() * width / src.width();
            image->setPixmap(QPixmap::fromImage(src.scaled(QSize(width, height))));
        }
        updateDefinition();
    }

private slots:
    void updateDefinition()
    {
        name->setVisible(!src.isNull());
        if (!name->isVisible() || name->text().isEmpty()) {
            definition->clear();
            definition->setVisible(false);
        } else {
            definition->setText(QString("(define %1.width %2)\n"
                                        "(define %1.height %3)").arg(name->text(), QString::number(image->size().width()), QString::number(image->size().height())));
            definition->setVisible(true);
        }
        emit definitionChanged();
    }

signals:
    void definitionChanged();
};

class Printer : public QScrollArea
{
    Q_OBJECT
    QVBoxLayout *layout;
    int showIndex = 0;

public:
    Printer(QWidget *parent = 0)
        : QScrollArea(parent)
    {
        setFrameShape(QFrame::NoFrame);
        setWidget(new QWidget());
        setWidgetResizable(true);
        layout = new QVBoxLayout(this);
        layout->setAlignment(Qt::AlignTop);
        layout->setContentsMargins(0, 6, 6, 6);
        layout->setSpacing(6);
        widget()->setLayout(layout);
    }

public slots:
    void finishedEval()
    {
        for (int i=showIndex; i<layout->count(); i++)
            layout->itemAt(i)->widget()->deleteLater();
        showIndex = 0;
    }

    void print(likely_const_mat mat)
    {
        const int i = showIndex++;
        Matrix *matrix = NULL;
        QLayoutItem *item = layout->itemAt(i);
        if (item != NULL)
            return static_cast<Matrix*>(item->widget())->show(mat);
        matrix = new Matrix();
        layout->insertWidget(i, matrix);
        connect(matrix, SIGNAL(definitionChanged()), this, SLOT(definitionChanged()));
        matrix->show(mat);
    }

private slots:
    void definitionChanged()
    {
        QStringList definitions;
        for (int i=0; i<layout->count(); i++)
            definitions.append(static_cast<Matrix*>(layout->itemAt(i)->widget())->getDefinition());
        definitions.removeAll(QString());
        emit newDefinitions(definitions.join("\n") + (definitions.isEmpty() ? "" : "\n"));
    }

signals:
    void newDefinitions(QString);
};

class CommandMode : public QObject
{
    Q_OBJECT
    bool enabled = false;

    bool eventFilter(QObject *obj, QEvent *event)
    {
        if ((event->type() == QEvent::KeyPress) ||
            (event->type() == QEvent::KeyRelease)) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Control) {
                if (event->type() == QEvent::KeyPress) {
                    if (!enabled) {
                        QGuiApplication::setOverrideCursor(Qt::PointingHandCursor);
                        enabled = true;
                    }
                } else {
                    QGuiApplication::restoreOverrideCursor();
                    enabled = false;
                }
                emit commandMode(enabled);
            }
        }
        return QObject::eventFilter(obj, event);
    }

signals:
    void commandMode(bool);
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QApplication &application)
    {
        QMenu *fileMenu = new QMenu("File");
        QAction *newSource = new QAction("New...", fileMenu);
        QAction *openSource = new QAction("Open...", fileMenu);
        QAction *saveSource = new QAction("Save", fileMenu);
        QAction *saveSourceAs = new QAction("Save As...", fileMenu);
        newSource->setShortcut(QKeySequence("Ctrl+N"));
        openSource->setShortcut(QKeySequence("Ctrl+O"));
        saveSource->setShortcut(QKeySequence("Ctrl+S"));
        saveSourceAs->setShortcut(QKeySequence("Ctrl+Shift+S"));
        fileMenu->addAction(newSource);
        fileMenu->addAction(openSource);
        fileMenu->addAction(saveSource);
        fileMenu->addAction(saveSourceAs);

        QMenu *commandsMenu = new QMenu("Commands");
        QAction *increment = new QAction("Increment", commandsMenu);
        QAction *decrement = new QAction("Decrement", commandsMenu);
        QAction *increment10x = new QAction("Increment 10x", commandsMenu);
        QAction *decrement10x = new QAction("Decrement 10x", commandsMenu);
        increment->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_Equal));
        decrement->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_Minus));
        increment10x->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_Equal));
        decrement10x->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_Minus));
        commandsMenu->addAction(increment);
        commandsMenu->addAction(decrement);
        commandsMenu->addAction(increment10x);
        commandsMenu->addAction(decrement10x);

        QMenuBar *menuBar = new QMenuBar();
        menuBar->addMenu(fileMenu);
        menuBar->addMenu(commandsMenu);

        QStatusBar *statusBar = new QStatusBar();
        statusBar->setSizeGripEnabled(true);

        CommandMode *commandMode = new CommandMode();
        application.installEventFilter(commandMode);

        Source *source = new Source();
        SyntaxHighlighter *syntaxHighlighter = new SyntaxHighlighter(source->document());
        Printer *printer = new Printer();

        const int WindowWidth = 600;
        QSplitter *splitter = new QSplitter(Qt::Horizontal);
        splitter->addWidget(source);
        splitter->addWidget(printer);
        splitter->setSizes(QList<int>() << WindowWidth/2 << WindowWidth/2);

        setCentralWidget(splitter);
        setMenuBar(menuBar);
        setStatusBar(statusBar);
        setWindowTitle("Likely");
        resize(800, WindowWidth);

        connect(commandMode, SIGNAL(commandMode(bool)), syntaxHighlighter, SLOT(setCommandMode(bool)));
        connect(printer, SIGNAL(newDefinitions(QString)), source, SLOT(setHeader(QString)));
        connect(fileMenu, SIGNAL(triggered(QAction*)), source, SLOT(fileMenu(QAction*)));
        connect(commandsMenu, SIGNAL(triggered(QAction*)), source, SLOT(commandsMenu(QAction*)));
        connect(source, SIGNAL(finishedEval()), printer, SLOT(finishedEval()));
        connect(source, SIGNAL(newResult(likely_const_mat)), printer, SLOT(print(likely_const_mat)));
        connect(source, SIGNAL(newFileName(QString)), this, SLOT(setWindowTitle(QString)));
        connect(source, SIGNAL(newStatus(QString)), statusBar, SLOT(showMessage(QString)));

        likely_set_error_callback(error_callback, statusBar);
        likely_set_show_callback(show_callback, printer);
        source->restore();
    }

private slots:
    void stateChanged()
    {
        if ((windowTitle() != "Likely") && !windowTitle().endsWith("*"))
            setWindowTitle(windowTitle() + "*");
    }

private:
    static void error_callback(likely_error error, void *context)
    {
        likely_mat str = likely_error_to_string(error);
        const char *message = (const char*) str->data;
        qDebug() << message;
        reinterpret_cast<QStatusBar*>(context)->showMessage(message);
        likely_release(str);
    }

    static void show_callback(likely_const_mat m, void *context)
    {
        reinterpret_cast<Printer*>(context)->print(m);
    }
};

int main(int argc, char *argv[])
{
    QApplication::setApplicationName("Dream");
    QApplication::setOrganizationName("Likely");
    QApplication::setOrganizationDomain("liblikely.org");
    QApplication application(argc, argv);
    application.setStyleSheet("QWidget { font-family: Monaco }\n");

    MainWindow mainWindow(application);
    mainWindow.show();

    return application.exec();
}

#include "dream.moc"
