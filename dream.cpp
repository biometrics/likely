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
    Variable(const QString &name, bool definable = false)
    {
        setFrameStyle(QFrame::Panel | QFrame::Raised);
        setLineWidth(2);
        setObjectName(name);
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
    virtual void refresh(lua_State *L) = 0;

protected:
    bool check(lua_State *L)
    {
        lua_getfield(L, -1, qPrintable(objectName()));
        const bool isValid = !lua_isnil(L, -1);

        bool success = true;
        if (!isValid) {
            lua_getfield(L, -2, "compiler");
            success = lua_isnil(L, -1);
            lua_pop(L, 1);
        }
        setEnabled(success);

        if (!isValid) lua_pop(L, 1);
        setVisible(isValid || !success);
        return isValid;
    }

private:
    void mousePressEvent(QMouseEvent *e)
    {
        if (e->modifiers() != Qt::ControlModifier)
            return QFrame::mousePressEvent(e);
        deleteLater();
    }

signals:
    void typeChanged();
    void definitionChanged();
};

class Matrix : public Variable
{
    QLabel *image;
    QImage src;

public:
    Matrix(const QString &name)
        : Variable(name, true)
    {
        image = new QLabel(this);
        image->setAlignment(Qt::AlignCenter);
        image->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
        layout->addWidget(image);
    }

private:
    void refresh(lua_State *L)
    {
        if (!check(L)) return;

        likely_mat *mp = reinterpret_cast<likely_mat*>(luaL_testudata(L, -1, "likely"));
        lua_pop(L, 1);
        if (!mp) {
            emit typeChanged();
            return;
        }
        likely_mat mat = *mp;

        double min, max;
        likely_mat rendered = likely_render(mat, &min, &max);
        src = QImage(rendered->data, rendered->columns, rendered->rows, QImage::Format_RGB888).rgbSwapped();
        likely_release(rendered);
        text->setText(QString("<b>%1</b>: %2x%3x%4x%5 %6 [%7,%8]").arg(objectName(),
                                                                              QString::number(mat->channels),
                                                                              QString::number(mat->columns),
                                                                              QString::number(mat->rows),
                                                                              QString::number(mat->frames),
                                                                              likely_type_to_string(mat->type),
                                                                              QString::number(min),
                                                                              QString::number(max)));
        updatePixmap();
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
    Closure(const QString &name)
        : Variable(name)
    {}

private:
    void refresh(lua_State *L)
    {
        if (!check(L)) return;

        lua_getfield(L, -1, "documentation");
        if (lua_isnil(L, -1)) {
            lua_pop(L, 2);
            emit typeChanged();
            return;
        }
        const QString documentation = lua_tostring(L, -1);
        lua_pop(L, 1);

        QStringList parameterDescriptions, parameterNames;
        lua_getfield(L, -1, "parameters");
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            lua_pushnil(L);
            lua_next(L, -2);
            QString name = lua_tostring(L, -1);
            lua_pop(L, 1);
            lua_next(L, -2);
            QString docs = lua_tostring(L, -1);
            lua_pop(L, 1);
            QString value;
            if (lua_next(L, -2)) {
                value = lua_tostring(L, -1);
                lua_pop(L, 2);
            }
            lua_pop(L, 1);
            parameterDescriptions.append(QString("<br>&nbsp;&nbsp;%1%2: %3").arg(name, value.isEmpty() ? QString() : "=" + value, docs));
            if (value.isEmpty())
                parameterNames.append(name);
        }
        lua_pop(L, 2);

        text->setText(QString("<b>%1</b>(%2): %3%4").arg(objectName(), parameterNames.join(", "), documentation, parameterDescriptions.join("")));
    }
};

struct Generic : public Variable
{
    Generic(const QString &name)
        : Variable(name)
    {}

private:
    void refresh(lua_State *L)
    {
        if (!check(L)) return;
        QString contents = lua_tostring(L, -1);
        lua_pop(L, 1);
        if (contents.isEmpty()) setVisible(false);
        else                    text->setText(QString("<b>%1</b>:%2%3").arg(objectName(),
                                                                            contents.contains('\n') ? "<br>" : " ",
                                                                            contents.replace("\n", "<br>")));
    }
};

class SyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
    QRegularExpression comments, keywords, numbers, strings, variables;
    QTextCharFormat commentsFont, keywordsFont, numbersFont, stringsFont, variablesFont;
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

    static QSet<QString> getGlobals(lua_State *L)
    {
        QSet<QString> globals;
        // Get the newly created variables...
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            globals.insert(lua_tostring(L, -2));
            lua_pop(L, 1);
        }

        // ...and the existing globals
        lua_getglobal(L, "_G");
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            globals.insert(lua_tostring(L, -2));
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
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
    QString sourceFileName, definitions, previousSource;
    QSettings settings;
    lua_State *L = NULL;
    int wheelRemainderX = 0, wheelRemainderY = 0;

public:
    Source()
    {
        setFont(QFont("Monaco"));
        connect(this, SIGNAL(textChanged()), this, SLOT(exec()));
    }

    void restore()
    {
        const QString source = settings.value("source").toString();
        // Start empty the next time if this source code crashes
        settings.setValue("source", QString());
        settings.sync();
        setText(source);
    }

public slots:
    void activate(const QString &name)
    {
        if (!L) return;
        lua_getfield(L, -1, qPrintable(name));
        Variable *variable;
        if      (luaL_testudata(L, -1, "likely")) variable = new Matrix(name);
        else if (lua_istable(L, -1))              variable = new Closure(name);
        else                                      variable = new Generic(name);
        lua_pop(L, 1);
        connect(this, SIGNAL(newState(lua_State*)), variable, SLOT(refresh(lua_State*)));
        emit newVariable(variable);
        variable->refresh(L); // Render the widget _after_ emitting it to reduce rendering glitches
    }

    void setDefinitions(const QString &definitions)
    {
        this->definitions = definitions;
        exec();
    }

    void fileMenu(QAction *a)
    {
        if (a->text() == "Open...") {
            sourceFileName = QFileDialog::getOpenFileName(NULL, "Open Source File");
            if (!sourceFileName.isEmpty()) {
                QFile file(sourceFileName);
                file.open(QFile::ReadOnly | QFile::Text);
                setText(QString::fromLocal8Bit(file.readAll()));
                file.close();
            }
        } else {
            if (sourceFileName.isEmpty() || (a->text() == "Save As..."))
                sourceFileName = QFileDialog::getSaveFileName(NULL, "Save Source File");
            if (!sourceFileName.isEmpty()) {
                QFile file(sourceFileName);
                file.open(QFile::WriteOnly | QFile::Text);
                file.write(toPlainText().toLocal8Bit());
                file.close();
            }
        }
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

    void examplesMenu(QAction *a)
    {
        setText(a->data().toString());
    }

private:
    void setText(QString text)
    {
        static const QRegularExpression ctrlExpression("\\\\dream\\{CTRL\\}");
        text = text.replace(ctrlExpression, QKeySequence(Qt::ControlModifier).toString(QKeySequence::NativeText));

        static const QRegularExpression activateExpression("\\\\dream\\{activate\\:(\\w+)\\}");
        QStringList activateVariables; activateVariables.append("compiler");
        QRegularExpressionMatchIterator i = activateExpression.globalMatch(text);
        while (i.hasNext())
            activateVariables.append(i.next().captured(1));
        text = text.replace(activateExpression, "");

        emit newSource();
        QPlainTextEdit::setPlainText(text);
        foreach (const QString &variable, activateVariables)
            activate(variable);
    }

    void toggle(QTextCursor tc)
    {
        tc.select(QTextCursor::WordUnderCursor);
        emit toggled(tc.selectedText());
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
        // This check needed because syntax highlighting triggers a textChanged() signal
        const QString source = definitions + "\n" + toPlainText();
        if (source == previousSource) return;
        else                          previousSource = source;

        emit recompiling();
        QElapsedTimer elapsedTimer;
        elapsedTimer.start();
        L = likely_exec(qPrintable(source), L);

        const qint64 nsec = elapsedTimer.nsecsElapsed();
        emit newStatus(QString("Execution Speed: %1 Hz").arg(nsec == 0 ? QString("infinity") : QString::number(double(1E9)/nsec, 'g', 3)));

        // Check for an error
        if (lua_type(L, -1) == LUA_TSTRING)
            lua_setfield(L, -2, "compiler");

        settings.setValue("source", toPlainText());
        emit newState(L);
    }

signals:
    void newState(lua_State*);
    void newStatus(QString);
    void newSource();
    void newVariable(Variable*);
    void recompiling();
    void toggled(QString);
};

class Documentation : public QScrollArea
{
    Q_OBJECT
    QVBoxLayout *layout;

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
    void addVariable(Variable *variable)
    {
        layout->addWidget(variable);
        connect(variable, SIGNAL(destroyed(QObject*)), this, SLOT(removeObject(QObject*)));
        connect(variable, SIGNAL(typeChanged()), this, SLOT(typeChanged()));
        connect(variable, SIGNAL(definitionChanged()), this, SLOT(definitionChanged()));
    }

    void clear()
    {
        for (int i=0; i<layout->count(); i++)
            layout->itemAt(i)->widget()->deleteLater();
    }

    void toggle(const QString &name)
    {
        bool found = false;
        for (int i=0; i<layout->count(); i++) {
            QWidget *widget = layout->itemAt(i)->widget();
            if (widget->objectName() == name) {
                found = true;
                widget->deleteLater();
            }
        }
        if (!found) emit activate(name);
    }

private slots:
    void typeChanged()
    {
        QObject *variable = sender();
        emit activate(variable->objectName());
        variable->deleteLater();
    }

    void definitionChanged()
    {
        QStringList definitions;
        for (int i=0; i<layout->count(); i++)
            definitions.append(static_cast<Variable*>(layout->itemAt(i)->widget())->getDefinition());
        definitions.removeAll(QString());
        emit newDefinitions(definitions.join("\n"));
    }

    void removeObject(QObject *object)
    {
        Variable *variable = static_cast<Variable*>(object);
        layout->removeWidget(variable);
        if (!variable->getDefinition().isEmpty())
            definitionChanged();
    }

signals:
    void activate(QString);
    void newDefinitions(QString);
};

class CommandMode : public QObject
{
    Q_OBJECT

    bool eventFilter(QObject *obj, QEvent *event)
    {
        if ((event->type() == QEvent::KeyPress) ||
            (event->type() == QEvent::KeyRelease)) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Control) {
                if (event->type() == QEvent::KeyPress) QGuiApplication::setOverrideCursor(Qt::PointingHandCursor);
                else                                   QGuiApplication::restoreOverrideCursor();
                emit commandMode(event->type() == QEvent::KeyPress);
            }
        }
        return QObject::eventFilter(obj, event);
    }

signals:
    void commandMode(bool);
};

int main(int argc, char *argv[])
{
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
        lua_close(likely_exec(qPrintable(source)));
    }

    if (argc > 1)
        return 0;

    QApplication::setApplicationName("Dream");
    QApplication::setOrganizationName("Likely");
    QApplication::setOrganizationDomain("liblikely.org");
    QApplication application(argc, argv);

    QMenu *fileMenu = new QMenu("File");
    QAction *openSource = new QAction("Open...", fileMenu);
    QAction *saveSource = new QAction("Save", fileMenu);
    QAction *saveSourceAs = new QAction("Save As...", fileMenu);
    openSource->setShortcut(QKeySequence("Ctrl+O"));
    saveSource->setShortcut(QKeySequence("Ctrl+S"));
    saveSourceAs->setShortcut(QKeySequence("Ctrl+Shift+S"));
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

    QMenu *examplesMenu = new QMenu("Examples");
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    luaL_requiref(L, "likely", luaopen_likely, 1);
    lua_pop(L, 1);
    luaL_dostring(L, likely_standard_library());
    lua_getglobal(L, "likely");
    lua_getfield(L, -1, "examples");
    lua_pushnil(L);
    while (lua_next(L, -2)) {
        const int index = lua_tonumber(L, -2);
        const QString source = lua_tostring(L, -1);
        QAction *example = new QAction(QString("%1. %2").arg(QString::number(index), source.mid(3, source.indexOf('\n')-3)), examplesMenu);
        example->setData(source);
        examplesMenu->addAction(example);
        lua_pop(L, 1);
    }
    lua_pop(L, 2);
    lua_close(L);

    QMenuBar *menuBar = new QMenuBar();
    menuBar->addMenu(fileMenu);
    menuBar->addMenu(commandsMenu);
    menuBar->addMenu(examplesMenu);

    QStatusBar *statusBar = new QStatusBar();
    statusBar->setSizeGripEnabled(true);

    CommandMode *commandMode = new CommandMode();
    application.installEventFilter(commandMode);

    Source *source = new Source();
    SyntaxHighlighter *syntaxHighlighter = new SyntaxHighlighter(source->document());
    Documentation *documentation = new Documentation();
    QObject::connect(commandMode, SIGNAL(commandMode(bool)), syntaxHighlighter, SLOT(setCommandMode(bool)));
    QObject::connect(documentation, SIGNAL(activate(QString)), source, SLOT(activate(QString)));
    QObject::connect(documentation, SIGNAL(newDefinitions(QString)), source, SLOT(setDefinitions(QString)));
    QObject::connect(fileMenu, SIGNAL(triggered(QAction*)), source, SLOT(fileMenu(QAction*)));
    QObject::connect(commandsMenu, SIGNAL(triggered(QAction*)), source, SLOT(commandsMenu(QAction*)));
    QObject::connect(examplesMenu, SIGNAL(triggered(QAction*)), source, SLOT(examplesMenu(QAction*)));
    QObject::connect(source, SIGNAL(toggled(QString)), documentation, SLOT(toggle(QString)));
    QObject::connect(source, SIGNAL(newSource()), documentation, SLOT(clear()));
    QObject::connect(source, SIGNAL(newState(lua_State*)), syntaxHighlighter, SLOT(updateDictionary(lua_State*)));
    QObject::connect(source, SIGNAL(newStatus(QString)), statusBar, SLOT(showMessage(QString)));
    QObject::connect(source, SIGNAL(newVariable(Variable*)), documentation, SLOT(addVariable(Variable*)));
    source->restore();

    const int WindowWidth = 600;
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(source);
    splitter->addWidget(documentation);
    splitter->setSizes(QList<int>() << WindowWidth/2 << WindowWidth/2);

    QMainWindow mainWindow;
    mainWindow.setCentralWidget(splitter);
    mainWindow.setMenuBar(menuBar);
    mainWindow.setStatusBar(statusBar);
    mainWindow.setWindowTitle("Likely Dream");
    mainWindow.resize(800,WindowWidth);
    mainWindow.show();

    return application.exec();
}

#include "dream.moc"
