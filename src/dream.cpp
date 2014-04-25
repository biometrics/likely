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
#include <QtOpenGL>
#include <llvm/Support/CommandLine.h>
#include <cassert>
#include <string>
#include <likely.h>

using namespace llvm;
using namespace std;

static cl::opt<string> Input(cl::Positional, cl::desc("<input file>"), cl::init(""));
static cl::opt<bool> Spartan("spartan", cl::desc("Hide the source code, only show the output"));

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
    int wheelRemainderX = 0, wheelRemainderY = 0;
    likely_env prev = NULL;

public:
    Source()
    {
        setLineWrapMode(QPlainTextEdit::NoWrap);
        connect(this, SIGNAL(textChanged()), this, SLOT(eval()));
    }

    ~Source()
    {
        likely_release_env(prev);
    }

public slots:
    void setHeader(const QString &header)
    {
        this->header = header;
        eval();
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
        likely_env env = likely_new_env_jit();
        likely_env new_env = likely_repl(qPrintable(source), true, env, prev);
        likely_release_env(env);
        if (!likely_erratum(new_env->type)) {
            const qint64 nsec = elapsedTimer.nsecsElapsed();
            emit newStatus(QString("Evaluation Speed: %1 Hz").arg(nsec == 0 ? QString("infinity") : QString::number(double(1E9)/nsec, 'g', 3)));
        }
        likely_release_env(prev);
        prev = new_env;
        emit finishedEval(toPlainText());
    }

signals:
    void finishedEval(QString);
    void newStatus(QString);
};

struct Image : public QGLWidget
{
    QImage image;

    Image(QWidget *parent)
        : QGLWidget(parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    }

    void setImage(const QImage &image)
    {
        this->image = image;
        setVisible(!image.isNull());
        updateGeometry();
        update();
    }

private:
    QSize sizeHint() const
    {
        return QSize(width(), image.height() * width() / image.width());
    }

    void paintEvent(QPaintEvent *e)
    {
        e->accept();
        QPainter painter(this);
        painter.drawImage(rect(), image);
    }

    void resizeEvent(QResizeEvent *event)
    {
        QGLWidget::resizeEvent(event);
        event->accept();
        updateGeometry();
    }
};

class Matrix : public QFrame
{
    Q_OBJECT
    QString name;
    QPoint prevMousePos;
    int width = 0, height = 0;
    double x = 0, y = 0, angle = 0, scale = 1;
    QLabel *type, *definition;
    Image *image;
    QVBoxLayout *layout;

public:
    Matrix()
        : type(new QLabel(this))
        , definition(new QLabel(this))
        , image(new Image(this))
        , layout(new QVBoxLayout(this))
    {
        definition->setWordWrap(true);
        layout->addWidget(type);
        layout->addWidget(image);
        layout->addWidget(definition);
        layout->setSpacing(3);
        grabGesture(Qt::PinchGesture);
        setLayout(layout);
        setLineWidth(2);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        spartan(false);
    }

    void show(likely_const_mat m, const QString &name)
    {
        if (likely_elements(m) <= 16) {
            image->setImage(QImage());

            likely_mat str = likely_to_string(m, true);
            type->setText((const char*) str->data);
            likely_release(str);
        } else {
            likely_const_mat show = (m->frames == 1) ? likely_retain(m)
                                                     : likely_new(m->type, m->channels, m->columns, m->rows, 1, m->data);
            double min, max;
            likely_const_mat rendered = likely_render(show, &min, &max);
            likely_release(show);
            image->setImage(QImage(rendered->data, rendered->columns, rendered->rows, 3*rendered->columns, QImage::Format_RGB888).rgbSwapped());
            likely_release(rendered);

            likely_mat str = likely_to_string(m, -1);
            type->setText(QString("%1 [%2,%3]").arg((const char*)str->data,
                                                    QString::number(min),
                                                    QString::number(max)));
            likely_release(str);
        }

        updateDefinition(name);
    }

    QString getDefinition() const
    {
        return definition->text();
    }

    void spartan(bool enabled)
    {
        type->setVisible(!enabled);
        if (enabled) {
            layout->setContentsMargins(0, 0, 0, 0);
            setFrameStyle(QFrame::NoFrame);
        } else {
            layout->setContentsMargins(3, 3, 3, 3);
            setFrameStyle(QFrame::Panel | QFrame::Raised);
        }
        updateDefinitionVisibility();
    }

    void reset()
    {
        if ((x == 0) && (y == 0) && (angle == 0) && (scale == 1))
            return;
        x = y = angle = 0;
        scale = 1;
        updateDefinition(name, true);
    }

private:
    bool event(QEvent *e)
    {
        if (e->type() == QEvent::Gesture) {
            QGestureEvent *ge = static_cast<QGestureEvent*>(e);
            if (QPinchGesture *pinch = static_cast<QPinchGesture*>(ge->gesture(Qt::PinchGesture))) {
                scale = scale / pinch->scaleFactor() * pinch->lastScaleFactor();
                angle += pinch->rotationAngle() - pinch->lastRotationAngle();
                updateDefinition(name, true);
            }
            return true;
        }

        return QWidget::event(e);
    }

    void mouseDoubleClickEvent(QMouseEvent *e)
    {
        e->accept();
        QPoint point = image->mapFromParent(e->pos());
        if (!image->rect().contains(point))
            return;

        const double resolution = qMax(image->size().width(), image->size().height());
        x += (point.x() - image->size().width()  / 2.0) / resolution * scale;
        y += (point.y() - image->size().height() / 2.0) / resolution * scale;
        scale /= 1.5;
        updateDefinition(name, true);
    }

    void mousePressEvent(QMouseEvent *e)
    {
        e->accept();
        if (image->rect().contains(image->mapFromParent(e->pos()))) prevMousePos = e->pos();
        else                                                        prevMousePos = QPoint();
    }

    void mouseMoveEvent(QMouseEvent *e)
    {
        e->accept();
        QPoint mousePos = e->pos();
        if (!prevMousePos.isNull()) {
            const double resolution = qMax(image->size().width(), image->size().height());
            x -= (mousePos.x() - prevMousePos.x()) / resolution * scale;
            y -= (mousePos.y() - prevMousePos.y()) / resolution * scale;
        }
        prevMousePos = mousePos;
        updateDefinition(name, true);
    }

    void resizeEvent(QResizeEvent *e)
    {
        QWidget::resizeEvent(e);
        e->accept();
        updateDefinition(name);
    }

    void wheelEvent(QWheelEvent *e)
    {
        e->accept();
        const double delta = double(e->delta()) / (360 * 8);
        if (e->orientation() == Qt::Horizontal) x += delta * scale;
        else                                    y += delta * scale;
        updateDefinition(name, true);
    }

    void updateDefinition(const QString &newName, bool forceUpdate = false)
    {
        const int newWidth  = image->image.isNull() ? 0 : image->size().width();
        const int newHeight = image->image.isNull() ? 0 : image->size().height();

        // Determine if the definition has changed
        bool same = ((name == newName) && (width == newWidth) && (height == newHeight));
        same = same || (name.isEmpty() && newName.isEmpty());
        same = same || (((width == 0) || (height == 0)) && ((newWidth == 0) || (newHeight == 0)));
        same = same && !forceUpdate;

        // Update the definition
        name = newName;
        width = newWidth;
        height = newHeight;

        if (same)
            return;

        if (name.isEmpty() || (width == 0) || (height == 0)) {
            definition->clear();
        } else {
            definition->setText(QString("    %1_x      = %2\n"
                                        "    %1_y      = %3\n"
                                        "    %1_width  = %4\n"
                                        "    %1_height = %5\n"
                                        "    %1_angle  = %6\n"
                                        "    %1_scale  = %7").arg(name,
                                                                  QString::number(x),
                                                                  QString::number(y),
                                                                  QString::number(width),
                                                                  QString::number(height),
                                                                  QString::number(angle),
                                                                  QString::number(scale)));
        }

        updateDefinitionVisibility();
        emit definitionChanged();
    }

    void updateDefinitionVisibility()
    {
        definition->setVisible(!definition->text().isEmpty() &&
                               (frameStyle() != QFrame::NoFrame)); // don't show in spartan mode
    }

signals:
    void definitionChanged();
};

class Printer : public QScrollArea
{
    Q_OBJECT
    QVBoxLayout *layout;
    int offset = 0;
    bool dirty = false;

public:
    Printer(QWidget *parent = 0)
        : QScrollArea(parent)
    {
        setFrameShape(QFrame::NoFrame);
        setWidget(new QWidget());
        setWidgetResizable(true);
        layout = new QVBoxLayout(this);
        layout->setAlignment(Qt::AlignTop);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(6);
        widget()->setLayout(layout);
    }

    void spartan(bool enabled)
    {
        assert(offset == 0);
        while (QLayoutItem *item = layout->itemAt(offset++))
            static_cast<Matrix*>(item->widget())->spartan(enabled);
        finishedPrinting();
    }

public slots:
    void finishedPrinting()
    {
        for (int i=offset; i<layout->count(); i++)
            layout->itemAt(i)->widget()->deleteLater();
        offset = 0;
        if (dirty) {
            definitionChanged();
            dirty = false;
        }
    }

    void print(likely_const_mat mat, const QString &name)
    {
        const int i = offset++;
        if (QLayoutItem *item = layout->itemAt(i)) // Try to recycle the widget
            return static_cast<Matrix*>(item->widget())->show(mat, name);
        Matrix *matrix = new Matrix();
        layout->insertWidget(i, matrix);
        connect(matrix, SIGNAL(definitionChanged()), this, SLOT(definitionChanged()));
        matrix->show(mat, name);
    }

    void reset()
    {
        assert(offset == 0);
        while (QLayoutItem *item = layout->itemAt(offset++))
            static_cast<Matrix*>(item->widget())->reset();
        finishedPrinting();
    }

private slots:
    void definitionChanged()
    {
        if (offset > 0) {
            // Wait until printing is done to emit new definitions
            dirty = true;
        } else {
            QStringList definitions;
            for (int i=0; i<layout->count(); i++)
                definitions.append(static_cast<Matrix*>(layout->itemAt(i)->widget())->getDefinition());
            definitions.removeAll(QString());
            emit newDefinitions(definitions.join("\n") + (definitions.isEmpty() ? "" : "\n"));
        }
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
    QSettings settings;
    Source *source;
    Printer *printer;

public:
    MainWindow(QApplication &application)
        : source(new Source())
        , printer(new Printer())
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
        QAction *fullScreen = new QAction("Full Screen", commandsMenu);
        QAction *spartan = new QAction("Spartan", commandsMenu);
        QAction *reset = new QAction("Reset", commandsMenu);
        QAction *increment = new QAction("Increment", commandsMenu);
        QAction *decrement = new QAction("Decrement", commandsMenu);
        QAction *increment10x = new QAction("Increment 10x", commandsMenu);
        QAction *decrement10x = new QAction("Decrement 10x", commandsMenu);
        fullScreen->setCheckable(true);
        fullScreen->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_Y));
        spartan->setCheckable(true);
        spartan->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_T));
        reset->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_R));
        increment->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_Equal));
        decrement->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_Minus));
        increment10x->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_Equal));
        decrement10x->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_Minus));
        commandsMenu->addAction(fullScreen);
        commandsMenu->addAction(spartan);
        commandsMenu->addAction(reset);
        commandsMenu->addSeparator();
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

        SyntaxHighlighter *syntaxHighlighter = new SyntaxHighlighter(source->document());

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
        connect(fileMenu, SIGNAL(triggered(QAction*)), this, SLOT(fileMenu(QAction*)));
        connect(commandsMenu, SIGNAL(triggered(QAction*)), source, SLOT(commandsMenu(QAction*)));
        connect(source, SIGNAL(finishedEval(QString)), this, SLOT(finishedEval(QString)));
        connect(source, SIGNAL(finishedEval(QString)), printer, SLOT(finishedPrinting()));
        connect(source, SIGNAL(newStatus(QString)), statusBar, SLOT(showMessage(QString)));
        connect(fullScreen, SIGNAL(toggled(bool)), this, SLOT(fullScreen(bool)));
        connect(spartan, SIGNAL(toggled(bool)), this, SLOT(spartan(bool)));
        connect(reset, SIGNAL(triggered()), printer, SLOT(reset()));

        likely_set_error_callback(error_callback, statusBar);
        likely_set_show_callback(show_callback, printer);
        restore();
        this->spartan(Spartan);
    }

private slots:
    void fullScreen(bool enabled)
    {
        if (enabled) showFullScreen();
        else         showNormal();
    }

    void spartan(bool enabled)
    {
        source->setVisible(!enabled);
        printer->spartan(enabled);
    }

    void stateChanged()
    {
        if ((windowTitle() != "Likely") && !windowTitle().endsWith("*"))
            setWindowTitle(windowTitle() + "*");
    }

    void restore()
    {
        if (!Input.empty()) {
            settings.setValue("sourceFileName", QString::fromStdString(Input));
            settings.sync();
        }

        // Try to open the previous file
        if (fileMenu("Open Quiet"))
            return;

        const QString code = settings.value("source").toString();
        settings.setValue("source", QString()); // Start empty the next time if this source code crashes
        settings.sync();
        source->setPlainText(code);
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
            source->setPlainText("");
        } else if (action.startsWith("Open")) {
            if (!action.endsWith("Quiet"))
                sourceFileName = QFileDialog::getOpenFileName(this, "Open Source File", sourceFilePath);
            if (!sourceFileName.isEmpty()) {
                QFile file(sourceFileName);
                if (file.open(QFile::ReadOnly | QFile::Text))
                    source->setPlainText(QString::fromLocal8Bit(file.readAll()));
                else
                    sourceFileName.clear();
            }
        } else {
            if (sourceFileName.isEmpty() || action.startsWith("Save As"))
                sourceFileName = QFileDialog::getSaveFileName(this, "Save Source File", sourceFilePath);
            if (!sourceFileName.isEmpty()) {
                QFile file(sourceFileName);
                file.open(QFile::WriteOnly | QFile::Text);
                file.write(source->toPlainText().toLocal8Bit());
            }
        }

        if (!sourceFileName.isEmpty())
            sourceFilePath = QFileInfo(sourceFileName).filePath();

        settings.setValue("sourceFileName", sourceFileName);
        settings.setValue("sourceFilePath", sourceFilePath);
        settings.sync();
        setWindowTitle(sourceFileName.isEmpty() ? "Likely" : QFileInfo(sourceFileName).fileName());
        return !sourceFileName.isEmpty();
    }

    void fileMenu(QAction *a)
    {
        fileMenu(a->text());
    }

    void finishedEval(const QString &source)
    {
        settings.setValue("source", source);
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

    static void show_callback(likely_const_mat m, const char *name, void *context)
    {
        reinterpret_cast<Printer*>(context)->print(m, name);
    }
};

int main(int argc, char *argv[])
{
    cl::ParseCommandLineOptions(argc, argv);

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
