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

class Matrix : public QLabel
{
    Q_OBJECT
    QImage src;

public:
    explicit Matrix(const QString &name, lua_State *L, QWidget *parent = 0)
        : QLabel(parent)
    {
        setAlignment(Qt::AlignCenter);
        setObjectName(name);
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
        refreshState(L);
    }

public slots:
    void refreshState(lua_State *L)
    {
        lua_getglobal(L, qPrintable(objectName()));
        likely_mat mat = (likely_mat) luaL_checkudata(L, -1, "likely");
        if (mat) src = QImage(mat->data, mat->columns, mat->rows, QImage::Format_RGB888).rgbSwapped();
        else     src = QImage();
        updatePixmap();
    }

private:
    void updatePixmap()
    {
        if (src.isNull()) {
            clear();
            setText(objectName());
        } else {
            const int width = qMin(size().width(), src.width());
            const int height = src.height() * width/src.width();
            setPixmap(QPixmap::fromImage(src.scaled(QSize(width, height))));
        }
    }

    void resizeEvent(QResizeEvent *e)
    {
        QLabel::resizeEvent(e);
        e->accept();
        updatePixmap();
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
    QHash<QString, QWidget*> variables;

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
                             "-- %1+click bold code to display information\n"
                             "lenna = read(\"img/Lenna.tiff\")\n"
                             "\n"
                             "-- Console output appears on the right\n"
                             "print(\"Width: \" .. lenna.columns)\n"
                             "print(\"Height: \" .. lenna.rows)\n").arg(QKeySequence(Qt::ControlModifier).toString(QKeySequence::NativeText));
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
    }

private:
    void mousePressEvent(QMouseEvent *e)
    {
        QTextEdit::mousePressEvent(e);
        QTextCursor tc = textCursor();
        tc.select(QTextCursor::WordUnderCursor);
        const QString variable = tc.selectedText();
        int toggled = highlighter->toggleVariable(variable);

        if (toggled > 0) {
            Matrix *matrix = new Matrix(variable, L);
            variables.insert(variable, matrix);
            emit newVariable(matrix);
        } else if (toggled < 0) {
            variables.take(variable)->deleteLater();
        }
    }

signals:
    void recompiling();
    void newVariable(QWidget*);
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
