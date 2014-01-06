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
#include <likely.h>

class SyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
    QRegularExpression comments, keywords, numbers, strings, variables;
    QTextCharFormat commentsFont, keywordsFont, markdownFont, numbersFont, stringsFont, variablesFont;
    QSet<QString> allowedSet; // Lua global variables
    bool commandMode = false;

public:
    SyntaxHighlighter(QTextDocument *parent)
        : QSyntaxHighlighter(parent)
    {
        comments.setPattern("--\\N*");
        keywords.setPattern("\\b(?:and|break|do|else|elseif|"
                            "end|false|goto|for|function|"
                            "if|in|local|nil|not|"
                            "or|repeat|return|then|true|"
                            "until|while)\\b");
        numbers.setPattern("-?\\d*\\.?\\d+(?:[Ee][+-]?\\d+)?");
        strings.setPattern("\"[^\"]*+\"");
        commentsFont.setForeground(Qt::darkGray);
        keywordsFont.setForeground(Qt::darkYellow);
        markdownFont.setForeground(Qt::darkGray);
        numbersFont.setFontUnderline(true);
        numbersFont.setUnderlineStyle(QTextCharFormat::DotLine);
        stringsFont.setForeground(Qt::darkGreen);
        variablesFont.setFontUnderline(true);
        variablesFont.setUnderlineStyle(QTextCharFormat::DotLine);
    }

public slots:
    void updateDictionary(lua_State *L)
    {
        allowedSet = getGlobals(L);
        variables.setPattern(getPattern(allowedSet));
        rehighlight();
    }

    void setCommandMode(bool enabled)
    {
        commandMode = enabled;
        rehighlight();
    }

private:
    void highlightBlock(const QString &text)
    {
        highlightHelp(text, keywords, keywordsFont);
        if (commandMode) highlightHelp(text, numbers, numbersFont);
        if (commandMode) highlightHelp(text, variables, variablesFont);
        highlightHelp(text, strings, stringsFont);
        highlightHelp(text, comments, commentsFont);
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
            setCurrentBlockState(previousBlockState() == -1 ? 1 : previousBlockState());

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

    static QSet<QString> getGlobals(lua_State *L)
    {
        QSet<QString> globals;

        // Get the existing globals...
        lua_getfield(L, -1, "_G");
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            globals.insert(lua_tostring(L, -2));
            lua_pop(L, 1);
        }
        lua_pop(L, 1);

        // ... and the newly created globals
        lua_getfield(L, -1, "_L");
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            globals.insert(lua_tostring(L, -2));
            lua_pop(L, 1);
        }
        lua_pop(L, 1);

        // As a precaution
        globals.remove(QString());

        return globals;
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
    lua_State *L = NULL;
    int wheelRemainderX = 0, wheelRemainderY = 0;
    QStringList shownVariables;

public:
    Source()
    {
        connect(this, SIGNAL(textChanged()), this, SLOT(exec()));
    }

    void restore()
    {
        // Try to open the previous file
        if (fileMenu("Open Quiet"))
            return;

        const QString source = settings.value("source").toString();
        settings.setValue("source", QString()); // Start empty the next time if this source code crashes
        settings.sync();
        setText(source);
    }

public slots:
    void setHeader(const QString &header)
    {
        this->header = header;
        exec();
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
            setText("");
        } else if (action.startsWith("Open")) {
            if (!action.endsWith("Quiet"))
                sourceFileName = QFileDialog::getOpenFileName(this, "Open Source File", sourceFilePath);
            if (!sourceFileName.isEmpty()) {
                QFile file(sourceFileName);
                if (file.open(QFile::ReadOnly | QFile::Text))
                    setText(QString::fromLocal8Bit(file.readAll()));
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
        if (a->text() == "Toggle") {
            toggle(textCursor());
        } else if (a->text().startsWith("Increment") ||
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
    void setText(const QString &text)
    {
        shownVariables.clear();
        QPlainTextEdit::setPlainText(text);
    }

    void toggle(QTextCursor tc)
    {
        tc.select(QTextCursor::WordUnderCursor);
        QString name = tc.selectedText();
        if (shownVariables.contains(name))
            shownVariables.removeOne(name);
        else
            shownVariables.append(name);
        exec();
    }

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

    void mousePressEvent(QMouseEvent *e)
    {
        if (e->modifiers() != Qt::ControlModifier)
            return QPlainTextEdit::mousePressEvent(e);
        toggle(cursorForPosition(e->pos()));
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
    void exec()
    {
        QString footer = "\n--Footer\n```";
        foreach (const QString &variable, shownVariables)
            footer += QString("\nshow(%1, \"%1\")").arg(variable);
        footer += "\n```\n";

        // This check needed because syntax highlighting triggers a textChanged() signal
        const QString source = header + toPlainText() + footer;
        if (source == previousSource) return;
        else                          previousSource = source;

        emit aboutToExec();
        QElapsedTimer elapsedTimer;
        elapsedTimer.start();
        L = likely_exec(qPrintable(source), L, 1);
        const qint64 nsec = elapsedTimer.nsecsElapsed();

        settings.setValue("source", toPlainText());
        emit newStatus(QString("Execution Speed: %1 Hz").arg(nsec == 0 ? QString("infinity") : QString::number(double(1E9)/nsec, 'g', 3)));
        emit newState(L);
    }

signals:
    void aboutToExec();
    void newFileName(QString);
    void newState(lua_State*);
    void newStatus(QString);
};

class Variable : public QFrame
{
    Q_OBJECT

protected:
    QWidget *top;
    QCheckBox *define;
    QLabel *text, *definition;
    QHBoxLayout *topLayout;
    QVBoxLayout *layout;

public:
    Variable(bool definable = false)
    {
        setFrameStyle(QFrame::Panel | QFrame::Raised);
        setLineWidth(2);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        top = new QWidget(this);
        define = new QCheckBox("Define", this);
        define->setVisible(definable);
        text = new QLabel(this);
        text->setWordWrap(true);
        definition = new QLabel(this);
        definition->setWordWrap(true);
        definition->setVisible(false);
        topLayout = new QHBoxLayout(top);
        topLayout->addWidget(text, 1);
        topLayout->addWidget(define);
        topLayout->setContentsMargins(0, 0, 0, 0);
        topLayout->setSpacing(3);
        layout = new QVBoxLayout(this);
        layout->addWidget(top);
        layout->addWidget(definition);
        layout->setContentsMargins(3, 3, 3, 3);
        layout->setSpacing(3);
        setLayout(layout);
        connect(define, SIGNAL(toggled(bool)), definition, SLOT(setVisible(bool)));
        connect(define, SIGNAL(toggled(bool)), this, SIGNAL(definitionChanged()));
    }

    QString getDefinition() const
    {
        return define->isChecked() ? definition->text() : QString();
    }

    void setDefinition(const QString &source)
    {
        const QString newDefinition = objectName() + " = " + source;
        if (newDefinition == definition->text())
            return;
        definition->setText(newDefinition);
        emit definitionChanged();
    }

public slots:
    virtual bool show(lua_State *L) = 0;

signals:
    void definitionChanged();
};

class Matrix : public Variable
{
    QLabel *image;
    QImage src;

public:
    Matrix() : Variable(true)
    {
        image = new QLabel(this);
        image->setAlignment(Qt::AlignCenter);
        image->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
        layout->addWidget(image);
    }

    static bool is(lua_State *L)
    {
        return luaL_testudata(L, 1, "likely") != NULL;
    }

private:
    bool show(lua_State *L)
    {
        if (!is(L))
            return false;

        const QString name = lua_tostring(L, 2);
        likely_mat mat = *reinterpret_cast<likely_mat*>(luaL_testudata(L, 1, "likely"));

        double min, max;
        likely_mat rendered = likely_render(mat, &min, &max);
        src = QImage(rendered->data, rendered->columns, rendered->rows, 3*rendered->columns, QImage::Format_RGB888).rgbSwapped();
        likely_release(rendered);
        text->setText(QString("%1%2x%3x%4x%5 %6 [%7,%8]").arg(name.isEmpty() ? QString() : QString("<b>%1</b>: ").arg(name),
                                                              QString::number(mat->channels),
                                                              QString::number(mat->columns),
                                                              QString::number(mat->rows),
                                                              QString::number(mat->frames),
                                                              likely_type_to_string(mat->type),
                                                              QString::number(min),
                                                              QString::number(max)));
        updatePixmap();
        return true;
    }

    void resizeEvent(QResizeEvent *e)
    {
        QWidget::resizeEvent(e);
        updatePixmap();
    }

    void updatePixmap()
    {
        image->setVisible(!src.isNull());
        if (src.isNull()) return;
        const int width = qMin(image->size().width(), src.width());
        const int height = src.height() * width/src.width();
        image->setPixmap(QPixmap::fromImage(src.scaled(QSize(width, height))));
        setDefinition(QString("{ width = %1, height = %2 }").arg(QString::number(image->size().width()), QString::number(image->size().height())));
    }
};

struct Closure : public Variable
{
    static bool is(lua_State *L)
    {
        if (!lua_getmetatable(L, 1))
            return false;
        luaL_getmetatable(L, "likely_closure");
        const bool closure = lua_rawequal(L, -1, -2);
        lua_pop(L, 2);
        return closure;
    }

private:
    bool show(lua_State *L)
    {
        if (!is(L))
            return false;

        lua_getfield(L, 1, "parameters");
        const int numParameters = luaL_len(L, -1);

        lua_pushinteger(L, 1);
        lua_gettable(L, -2);
        const QString documentation = lua_tostring(L, -1);
        lua_pop(L, 1);

        QStringList parameters;
        for (int i=2; i<=numParameters; i++) {
            lua_pushinteger(L, i);
            lua_gettable(L, -2);
            QString parameter = lua_tostring(L, -1);
            lua_pop(L, 1);

            lua_pushinteger(L, i);
            lua_gettable(L, 1);
            if (!lua_isnil(L, -1))
                parameter += QString("=") + likely_ir_to_string(L);
            lua_pop(L, 1);

            parameters.append(parameter);
        }

        text->setText(QString("<b>%1</b>(%2): %3").arg(lua_tostring(L, 2), parameters.join(", "), documentation));
        return true;
    }
};

class Generic : public Variable
{
    bool show(lua_State *L)
    {
        if (Matrix::is(L) || Closure::is(L))
            return false;
        const QString name = lua_tostring(L, 2);
        lua_pop(L, 1);
        const QString contents = likely_ir_to_string(L);
        lua_pushstring(L, qPrintable(name));
        text->setText(QString("%1%2%3").arg(name.isEmpty() ? QString() : QString("<b>%1</b>:").arg(name),
                                            name.isEmpty() ? "" : (contents.contains('\n') ? "<br>" : " "),
                                            QString(contents).replace("\n", "<br>")));
        return true;
    }
};

class Documentation : public QScrollArea
{
    Q_OBJECT
    QVBoxLayout *layout;
    int showIndex = 0;

public:
    Documentation(QWidget *parent = 0)
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
    void aboutToExec()
    {
        showIndex = 0;
    }

    void newState(lua_State *L)
    {
        if (lua_type(L, -1) == LUA_TSTRING) {
            // Show the compilation error
            lua_pushstring(L, "compiler");
            lua_getglobal(L, "show");
            lua_insert(L, -3);
            lua_call(L, 2, 0);
        }

        for (int i=showIndex; i<layout->count(); i++)
            layout->itemAt(i)->widget()->deleteLater();
    }

    void show(lua_State *L)
    {
        const int i = showIndex++;
        Variable *variable = NULL;
        QLayoutItem *item = layout->itemAt(i);
        if (item != NULL) {
            variable = static_cast<Variable*>(item->widget());
            if (variable->show(L))
                return;
            layout->removeWidget(variable);
            variable->deleteLater();
        }
        if      (Matrix::is(L))  variable = new Matrix();
        else if (Closure::is(L)) variable = new Closure();
        else                     variable = new Generic();
        layout->insertWidget(i, variable);
        connect(variable, SIGNAL(definitionChanged()), this, SLOT(definitionChanged()));
        variable->show(L);
    }

private slots:
    void definitionChanged()
    {
        QStringList definitions;
        for (int i=0; i<layout->count(); i++)
            definitions.append(static_cast<Variable*>(layout->itemAt(i)->widget())->getDefinition());
        definitions.removeAll(QString());
        emit newDefinitions(definitions.join("\n") + (definitions.isEmpty() ? "" : "\n"));
    }

    void removeObject(QObject *object)
    {
        Variable *variable = static_cast<Variable*>(object);
        layout->removeWidget(variable);
        if (!variable->getDefinition().isEmpty())
            definitionChanged();
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
        QAction *toggle = new QAction("Toggle", commandsMenu);
        QAction *increment = new QAction("Increment", commandsMenu);
        QAction *decrement = new QAction("Decrement", commandsMenu);
        QAction *increment10x = new QAction("Increment 10x", commandsMenu);
        QAction *decrement10x = new QAction("Decrement 10x", commandsMenu);
        toggle->setShortcut(QKeySequence("Ctrl+\n"));
        increment->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_Equal));
        decrement->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_Minus));
        increment10x->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_Equal));
        decrement10x->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_Minus));
        commandsMenu->addAction(toggle);
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
        Documentation *documentation = new Documentation();

        const int WindowWidth = 600;
        QSplitter *splitter = new QSplitter(Qt::Horizontal);
        splitter->addWidget(source);
        splitter->addWidget(documentation);
        splitter->setSizes(QList<int>() << WindowWidth/2 << WindowWidth/2);

        setCentralWidget(splitter);
        setMenuBar(menuBar);
        setStatusBar(statusBar);
        setWindowTitle("Likely");
        resize(800, WindowWidth);

        connect(commandMode, SIGNAL(commandMode(bool)), syntaxHighlighter, SLOT(setCommandMode(bool)));
        connect(documentation, SIGNAL(newDefinitions(QString)), source, SLOT(setHeader(QString)));
        connect(fileMenu, SIGNAL(triggered(QAction*)), source, SLOT(fileMenu(QAction*)));
        connect(commandsMenu, SIGNAL(triggered(QAction*)), source, SLOT(commandsMenu(QAction*)));
        connect(source, SIGNAL(aboutToExec()), documentation, SLOT(aboutToExec()));
        connect(source, SIGNAL(newFileName(QString)), this, SLOT(setWindowTitle(QString)));
        connect(source, SIGNAL(newState(lua_State*)), this, SLOT(stateChanged()));
        connect(source, SIGNAL(newState(lua_State*)), documentation, SLOT(newState(lua_State*)));
        connect(source, SIGNAL(newState(lua_State*)), syntaxHighlighter, SLOT(updateDictionary(lua_State*)));
        connect(source, SIGNAL(newStatus(QString)), statusBar, SLOT(showMessage(QString)));

        likely_set_show_callback(show_callback, documentation);
        source->restore();
    }

private slots:
    void stateChanged()
    {
        if ((windowTitle() != "Likely") && !windowTitle().endsWith("*"))
            setWindowTitle(windowTitle() + "*");
    }

    static void show_callback(lua_State *L, void *context)
    {
        reinterpret_cast<Documentation*>(context)->show(L);
    }
};

int main(int argc, char *argv[])
{
    if ((argc > 1) && !strcmp("-bitcode", argv[1])) {
        likely_ir ir = likely_ir_from_expression(argv[2]);
        QVector<likely_type> types;
        for (int i=3; i<argc-1; i++)
            types.append(likely_type_from_string(argv[i]));
        likely_write_bitcode(ir, qPrintable(QFileInfo(argv[argc-1]).baseName()), types.data(), types.size(), argv[argc-1], true);
    }

    for (int i=1; i<argc; i++) {
        QString source;
        if (QFileInfo(argv[i]).exists()) {
            QFile file(argv[i]);
            file.open(QFile::ReadOnly | QFile::Text);
            source = file.readAll();
            file.close();
            if (source.startsWith("#!"))
                source = source.mid(source.indexOf('\n')+1);
        } else {
            source = argv[i];
        }
        lua_close(likely_exec(qPrintable(source), NULL, 1));
    }

    if (argc > 1)
        return 0;

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
