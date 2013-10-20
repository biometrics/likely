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

class Messenger : public QObject
{
    Q_OBJECT
    static Messenger *singleton;

    Messenger() : QObject(0)
    {
        likely_set_message_callback(Messenger::send);
    }

    void sendMessage(QString message, bool error)
    {
        if (error)
            message = "<font color=\"red\">"+message+"</font>";
        emit newMessage(message);
    }

public:
    static Messenger *get()
    {
        if (!singleton)
            singleton = new Messenger();
        return singleton;
    }

    static void send(const char *message, bool error)
    {
        get()->sendMessage(message, error);
    }

signals:
    void newMessage(QString);
};

Messenger *Messenger::singleton = NULL;

class Variable : public QWidget
{
    Q_OBJECT

protected:
    QLabel *text;
    QLayout *layout;

public:
    explicit Variable(const QString &name)
    {
        setObjectName(name);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        text = new QLabel(this);
        layout = new QVBoxLayout(this);
        layout->addWidget(text);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        setLayout(layout);
    }

public slots:
    virtual void refresh(lua_State *L) = 0;

protected:
    bool check(lua_State *L)
    {
        lua_getglobal(L, qPrintable(objectName()));
        const bool isNil = lua_isnil(L, -1);
        if (isNil) {
            lua_pop(L, 1);
            deleteLater();
        }
        return !isNil;
    }
};

class Matrix : public Variable
{
    QLabel *image;
    QImage src;

public:
    explicit Matrix(const QString &name)
        : Variable(name)
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
        likely_mat mat = (likely_mat) luaL_testudata(L, -1, "likely");
        lua_pop(L, 1);
        if (!mat) {
            deleteLater();
            return;
        }

        src = QImage(mat->data, mat->columns, mat->rows, QImage::Format_RGB888).rgbSwapped();
        text->setText(QString("<b>%1</b>: %2x%3x%4x%5 %6 (0x%7)").arg(objectName(),
                                                                      QString::number(mat->channels),
                                                                      QString::number(mat->columns),
                                                                      QString::number(mat->rows),
                                                                      QString::number(mat->frames),
                                                                      likely_hash_to_string(mat->hash),
                                                                      QString::number((ulong)mat->data, 16)));
        updatePixmap();
    }

    void resizeEvent(QResizeEvent *e)
    {
        QWidget::resizeEvent(e);
        e->accept();
        updatePixmap();
    }

    void updatePixmap()
    {
        image->setVisible(!src.isNull());
        if (!src.isNull()) {
            const int width = qMin(image->size().width(), src.width());
            const int height = src.height() * width/src.width();
            image->setPixmap(QPixmap::fromImage(src.scaled(QSize(width, height))));
        }
    }
};

struct Function : public Variable
{
    Function(const QString &name)
        : Variable(name)
    {}

private:
    void refresh(lua_State *L)
    {
        if (!check(L)) return;
        lua_getfield(L, -1, "documentation");
        text->setText(QString("<b>%1</b>: %2").arg(objectName(), lua_tostring(L, -1)));
        lua_pop(L, 2);
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
        if (contents.isEmpty())
            contents = lua_typename(L, -1);
        text->setText(QString("<b>%1</b>: %2").arg(objectName(), contents));
        lua_pop(L, 1);
    }
};

class SyntaxHighlighter : public QSyntaxHighlighter
{
    QRegularExpression comments, keywords, numbers, strings, allowed, toggled;
    QTextCharFormat commentsFont, keywordsFont, numbersFont, stringsFont, allowedFont, toggledFont;
    QSet<QString> excludedSet, allowedSet, toggledSet; // Lua global variables

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
        numbers.setPattern("(?:0x[\\da-fA-F]*\\.?[\\da-fA-F]+(?:[pP]-?\\d+)?|-?\\d*\\.?\\d+(?:[Ee][+-]?\\d+)?)");
        strings.setPattern("\"[^\"]*+\"");
        commentsFont.setForeground(Qt::darkGray);
        keywordsFont.setForeground(Qt::darkYellow);
        numbersFont.setFontWeight(QFont::Bold);
        numbersFont.setForeground(Qt::darkBlue);
        stringsFont.setForeground(Qt::darkGreen);
        allowedFont.setFontWeight(QFont::Bold);
        toggledFont.setFontWeight(QFont::Bold);
        toggledFont.setForeground(Qt::darkMagenta);

        lua_State *L = luaL_newstate();
        luaL_openlibs(L);
        excludedSet = getGlobals(L, QSet<QString>());
        lua_close(L);
    }

    void updateDictionary(lua_State *L)
    {
        allowedSet = getGlobals(L, excludedSet + toggledSet);
        allowed.setPattern(getPattern(allowedSet));
        rehighlight();
    }

    int toggleVariable(const QString &variable)
    {
        int toggledResult = 0;

        if (toggledSet.contains(variable)) {
            toggledSet.remove(variable);
            allowedSet.insert(variable);
            toggledResult = -1;
        } else if (allowedSet.contains(variable)) {
            allowedSet.remove(variable);
            toggledSet.insert(variable);
            toggledResult = 1;
        }

        if (toggledResult) {
            allowed.setPattern(getPattern(allowedSet));
            toggled.setPattern(getPattern(toggledSet));
            rehighlight();
        }

        return toggledResult;
    }

private:
    void highlightBlock(const QString &text)
    {
        highlightHelp(text, keywords, keywordsFont);
        highlightHelp(text, numbers, numbersFont);
        highlightHelp(text, allowed, allowedFont);
        highlightHelp(text, toggled, toggledFont);
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

    static QSet<QString> getGlobals(lua_State *L, const QSet<QString> &exclude)
    {
        QSet<QString> globals;
        lua_getglobal(L, "_G");
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            const QString global = lua_tostring(L, -2);
            if (!exclude.contains(global))
                globals.insert(global);
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

class Source : public QTextEdit
{
    Q_OBJECT
    QString sourceFileName, previousSource;
    QSettings settings;
    lua_State *L = NULL;
    SyntaxHighlighter *highlighter;
    QHash<QString, Variable*> variables;
    int wheelRemainderX = 0, wheelRemainderY = 0;

public:
    Source(QWidget *p = 0)
        : QTextEdit(p)
    {
        highlighter = new SyntaxHighlighter(document());
        setAcceptRichText(false);
        connect(this, SIGNAL(textChanged()), this, SLOT(exec()));
    }

    static lua_State *exec(const QString &source, bool *error)
    {
        lua_State *L = luaL_newstate();
        luaL_openlibs(L);
        luaL_requiref(L, "likely", luaopen_likely, 1);
        lua_pop(L, 1);
        *error = luaL_dostring(L, likely_standard_library()) ||
                 luaL_dostring(L, qPrintable(source));
        return L;
    }

public slots:
    void setDefaultSource()
    {
        QString source = settings.value("source").toString();
        if (source.isEmpty())
            source = QString("-- Source code is compiled as you type\n"
                             "message = \"Hello World!\"\n"
                             "\n"
                             "-- Console output appears on the right\n"
                             "print(message)\n"
                             "\n"
                             "-- %1+click bold code to display value\n"
                             "lenna = read(\"img/Lenna.tiff\")\n"
                             "\n"
                             "-- %1+scroll to edit numerical constants\n"
                             "x = 1 + 1\n"
                             "print(\"x = \" .. x)\n"
                             "").arg(QKeySequence(Qt::ControlModifier).toString(QKeySequence::NativeText));
        setText(source);
    }

    void setSource(QAction *a)
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

    void exec()
    {
        // This check needed because syntax highlighting triggers a textChanged() signal
        const QString source = toPlainText();
        if (source == previousSource) return;
        else previousSource = source;

        emit recompiling();
        if (L) lua_close(L);
        bool error;
        QElapsedTimer elapsedTimer;
        elapsedTimer.start();
        L = exec(source, &error);
        const qint64 nsec = elapsedTimer.nsecsElapsed();
        emit newStatus(QString("Execution Speed: %1 Hz").arg(nsec == 0 ? QString("infinity") : QString::number(double(1E9)/nsec, 'g', 3)));

        if (error) {
            Messenger::send(lua_tostring(L, -1), true);
            lua_pop(L, 1);
        }
        settings.setValue("source", source);
        highlighter->updateDictionary(L);
        emit newState(L);
    }

private:
    void keyPressEvent(QKeyEvent *e)
    {
        if (e->key() != Qt::Key_Control) {
            QTextEdit::keyPressEvent(e);
            return;
        }

        e->accept();
        viewport()->setCursor(Qt::PointingHandCursor);
    }

    void keyReleaseEvent(QKeyEvent *e)
    {
        if (e->key() != Qt::Key_Control) {
            QTextEdit::keyPressEvent(e);
            return;
        }

        e->accept();
        viewport()->setCursor(Qt::IBeamCursor);
    }

    void mousePressEvent(QMouseEvent *e)
    {
        if (e->modifiers() != Qt::ControlModifier) {
            QTextEdit::mousePressEvent(e);
            return;
        }

        e->accept();
        QTextCursor tc = cursorForPosition(e->pos());
        tc.select(QTextCursor::WordUnderCursor);
        const QString name = tc.selectedText();
        int toggled = highlighter->toggleVariable(name);

        if (toggled > 0) {
            QString type;
            lua_getglobal(L, qPrintable(name));
            if (lua_istable(L, -1) || lua_isuserdata(L, -1)) {
                lua_getfield(L, -1, "likely");
                type = lua_tostring(L, -1);
                lua_pop(L, 1);
            }
            lua_pop(L, 1);

            Variable *variable;
            if      (type == "matrix")   variable = new Matrix(name);
            else if (type == "function") variable = new Function(name);
            else                         variable = new Generic(name);
            connect(this, SIGNAL(newState(lua_State*)), variable, SLOT(refresh(lua_State*)));
            connect(variable, SIGNAL(destroyed(QObject*)), this, SLOT(removeObject(QObject*)));
            variable->refresh(L);
            variables.insert(name, variable);
            emit newVariable(variable);
        } else if (toggled < 0) {
            variables.take(name)->deleteLater();
        }
    }

    void wheelEvent(QWheelEvent *e)
    {
        if (e->modifiers() != Qt::ControlModifier) {
            QTextEdit::wheelEvent(e);
            return;
        }

        e->accept();
        const int deltaX = getIncrement(e->angleDelta().x(), wheelRemainderX, wheelRemainderY);
        const int deltaY = getIncrement(e->angleDelta().y(), wheelRemainderY, wheelRemainderX);
        if ((deltaX == 0) && (deltaY == 0))
            return;

        QTextCursor tc = cursorForPosition(e->pos());
        tc.select(QTextCursor::WordUnderCursor);
        { // Does the number start with a negative sign?
            QTextCursor tcNegative(tc);
            tcNegative.movePosition(QTextCursor::StartOfWord);
            tcNegative.movePosition(QTextCursor::PreviousCharacter);
            tcNegative.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
            tcNegative.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
            if (tcNegative.selectedText().startsWith('-'))
                tc = tcNegative;
        }

        bool ok;
        const int val = tc.selectedText().toInt(&ok);
        if (ok) tc.insertText(QString::number(qRound(qPow(10, deltaX) * (val + deltaY))));
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

private slots:
    void removeObject(QObject *object)
    {
        const QString key = variables.key((Variable*)object);
        if (!key.isNull()) {
            highlighter->toggleVariable(object->objectName());
            variables.remove(key);
        }
    }

signals:
    void recompiling();
    void newVariable(QWidget*);
    void newState(lua_State*);
    void newStatus(QString);
};

class Console : public QTextEdit
{
    Q_OBJECT

public:
    Console(QWidget *parent = 0)
        : QTextEdit(parent)
    {
        setReadOnly(true);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    }
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
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        widget()->setLayout(layout);
    }

public slots:
    void addWidget(QWidget *widget)
    {
        layout->addWidget(widget);
        connect(widget, SIGNAL(destroyed(QObject*)), this, SLOT(removeObject(QObject*)));
    }

private slots:
    void removeObject(QObject *object)
    {
        layout->removeWidget((QWidget*)object);
    }
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

        bool error;
        lua_close(Source::exec(source, &error));
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
    QMenuBar *menuBar = new QMenuBar();
    menuBar->addMenu(fileMenu);

    QStatusBar *statusBar = new QStatusBar();
    statusBar->setSizeGripEnabled(true);

    Source *source = new Source();
    Console *console = new Console();
    Documentation *documentation = new Documentation();
    QObject::connect(fileMenu, SIGNAL(triggered(QAction*)), source, SLOT(setSource(QAction*)));
    QObject::connect(source, SIGNAL(newStatus(QString)), statusBar, SLOT(showMessage(QString)));
    QObject::connect(source, SIGNAL(newVariable(QWidget*)), documentation, SLOT(addWidget(QWidget*)));
    QObject::connect(source, SIGNAL(recompiling()), console, SLOT(clear()));
    QObject::connect(Messenger::get(), SIGNAL(newMessage(QString)), console, SLOT(append(QString)));
    source->setDefaultSource();

    const int WindowWidth = 600;
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(source);
    splitter->addWidget(documentation);
    documentation->addWidget(console);
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
