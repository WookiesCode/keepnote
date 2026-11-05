#include <QFile>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextLayout>
#include <QtTest>

#include "backend.h"
#include "syntaxhighlighter.h"

class KeepnoteTests : public QObject {
    Q_OBJECT

private slots:
    void restoresTextAfterReopen() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath("notes/note.txt");
        {
            Backend first(path);
            first.setText(QString::fromUtf8("First line\n☕ second line"));
            QVERIFY(first.flush());
            QVERIFY(!first.hasError());
        }
        Backend reopened(path);
        QCOMPARE(reopened.text(), QString::fromUtf8("First line\n☕ second line"));
    }

    void autosavesAfterTypingStops() {
        QTemporaryDir directory;
        const QString path = directory.filePath("note.txt");
        Backend note(path);
        note.setText("Draft");
        QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(path), 2000);
        Backend reopened(path);
        QCOMPARE(reopened.text(), QString("Draft"));
    }

    void restoresTabsAndSelectedTab() {
        QTemporaryDir directory;
        const QString path = directory.filePath("notes.json");
        {
            Backend notes(path);
            notes.setText("First note");
            notes.addTab();
            notes.setText("Second note");
            QVERIFY(notes.flush());
        }
        Backend reopened(path);
        QCOMPARE(reopened.tabs().size(), 2);
        QCOMPARE(reopened.currentIndex(), 1);
        QCOMPARE(reopened.text(), QString("Second note"));
        reopened.selectTab(0);
        QCOMPARE(reopened.text(), QString("First note"));
    }

    void restoresCustomTabNames() {
        QTemporaryDir directory;
        const QString path = directory.filePath("notes.json");
        {
            Backend notes(path);
            notes.setText("Body text");
            notes.renameTab(0, "  Projects  ");
            notes.addTab();
            notes.renameTab(1, "Ideas");
            QVERIFY(notes.flush());
        }
        Backend reopened(path);
        QCOMPARE(reopened.tabs().at(0).toMap().value("title").toString(), QString("Projects"));
        QCOMPARE(reopened.tabs().at(1).toMap().value("title").toString(), QString("Ideas"));
        reopened.renameTab(0, "");
        QCOMPARE(reopened.tabs().at(0).toMap().value("title").toString(), QString("Body text"));
    }

    void opensTabsSavedBeforeNamesExisted() {
        QTemporaryDir directory;
        const QString path = directory.filePath("notes.json");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(R"({"version":1,"currentIndex":0,"tabs":[{"text":"Older tab"}]})");
        file.close();
        Backend notes(path);
        QVERIFY(!notes.hasError());
        QCOMPARE(notes.tabs().at(0).toMap().value("title").toString(), QString("Older tab"));
    }

    void manuallySavesCurrentTabToFile() {
        QTemporaryDir directory;
        const QString statePath = directory.filePath("notes.json");
        const QString exportPath = directory.filePath("My note.txt");
        Backend notes(statePath);
        notes.setText("First version\nSecond line");
        QVERIFY(notes.saveCurrentTo(QUrl::fromLocalFile(exportPath)));
        QFile exported(exportPath);
        QVERIFY(exported.open(QIODevice::ReadOnly));
        QCOMPARE(exported.readAll(), QByteArray("First version\nSecond line"));
        exported.close();

        notes.setText("New version");
        QVERIFY(notes.flush());
        QVERIFY(exported.open(QIODevice::ReadOnly));
        QCOMPARE(exported.readAll(), QByteArray("First version\nSecond line"));
        exported.close();

        Backend reopened(statePath);
        QCOMPARE(reopened.text(), QString("New version"));
        reopened.saveCurrent();
        QVERIFY(exported.open(QIODevice::ReadOnly));
        QCOMPARE(exported.readAll(), QByteArray("New version"));
    }

    void failedManualSaveDoesNotReplaceExistingFile() {
        QTemporaryDir directory;
        Backend notes(directory.filePath("notes.json"));
        notes.setText("Keep this text");
        QVERIFY(!notes.saveCurrentTo(QUrl::fromLocalFile(directory.filePath("missing/file.txt"))));
        QCOMPARE(notes.text(), QString("Keep this text"));
        QVERIFY(notes.flush());
        Backend reopened(directory.filePath("notes.json"));
        QCOMPARE(reopened.text(), QString("Keep this text"));
    }

    void opensFileInTabAndSavesEdits() {
        QTemporaryDir directory;
        const QString sourcePath = directory.filePath("ideas.txt");
        QFile source(sourcePath);
        QVERIFY(source.open(QIODevice::WriteOnly));
        source.write("Idea one\nIdea two");
        source.close();

        Backend notes(directory.filePath("notes.json"));
        notes.setText("Existing draft");
        QVERIFY(notes.openFile(QUrl::fromLocalFile(sourcePath)));
        QCOMPARE(notes.tabs().size(), 2);
        QCOMPARE(notes.text(), QString("Idea one\nIdea two"));
        QCOMPARE(notes.tabs().at(1).toMap().value("title").toString(), QString("ideas"));
        QVERIFY(notes.openFile(QUrl::fromLocalFile(sourcePath)));
        QCOMPARE(notes.tabs().size(), 2);

        notes.setText("Changed idea");
        QVERIFY(notes.flush());
        QVERIFY(source.open(QIODevice::ReadOnly));
        QCOMPARE(source.readAll(), QByteArray("Idea one\nIdea two"));
        source.close();
        notes.saveCurrent();
        QVERIFY(source.open(QIODevice::ReadOnly));
        QCOMPARE(source.readAll(), QByteArray("Changed idea"));
    }

    void preservesWindowsLineEndingsOnManualSave() {
        QTemporaryDir directory;
        const QString sourcePath = directory.filePath("windows.txt");
        QFile source(sourcePath);
        QVERIFY(source.open(QIODevice::WriteOnly));
        source.write("Line one\r\nLine two\r\n");
        source.close();
        Backend notes(directory.filePath("notes.json"));
        QVERIFY(notes.openFile(QUrl::fromLocalFile(sourcePath)));
        QCOMPARE(notes.text(), QString("Line one\nLine two\n"));
        notes.setText("First\nSecond\n");
        notes.saveCurrent();
        QVERIFY(source.open(QIODevice::ReadOnly));
        QCOMPARE(source.readAll(), QByteArray("First\r\nSecond\r\n"));
    }

    void rejectsInvalidUtf8WithoutChangingNotes() {
        QTemporaryDir directory;
        const QString sourcePath = directory.filePath("bad.txt");
        QFile source(sourcePath);
        QVERIFY(source.open(QIODevice::WriteOnly));
        source.write(QByteArray::fromHex("fffe"));
        source.close();
        Backend notes(directory.filePath("notes.json"));
        notes.setText("Keep me");
        QVERIFY(!notes.openFile(QUrl::fromLocalFile(sourcePath)));
        QCOMPARE(notes.tabs().size(), 1);
        QCOMPARE(notes.text(), QString("Keep me"));
    }

    void detectsLanguageFromFileExtension() {
        QTemporaryDir directory;
        Backend notes(directory.filePath("notes.json"));
        const QList<QPair<QString, QString>> cases = {
            {"events.log", "log"}, {"data.json", "json"}, {"config.yaml", "yaml"},
            {"settings.toml", "toml"}, {"script.py", "python"}, {"app.js", "javascript"},
            {"init.lua", "lua"}, {"run.sh", "shell"}, {"types.ts", "typescript"},
            {"readme.md", "markdown"}, {"notes.txt", "plain"}
        };
        for (const auto &entry : cases) {
            QFile file(directory.filePath(entry.first));
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("sample\n");
            file.close();
            QVERIFY(notes.openFile(QUrl::fromLocalFile(file.fileName())));
            QCOMPARE(notes.language(), entry.second);
        }
    }

    void remembersManualLanguagePerTab() {
        QTemporaryDir directory;
        const QString path = directory.filePath("notes.json");
        {
            Backend notes(path);
            QCOMPARE(notes.language(), QString("markdown"));
            notes.setLanguage("python");
            notes.addTab();
            notes.setLanguage("json");
            QVERIFY(notes.flush());
        }
        Backend reopened(path);
        QCOMPARE(reopened.language(), QString("json"));
        reopened.selectTab(0);
        QCOMPARE(reopened.language(), QString("python"));
    }

    void highlightsRequestedLanguages_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<QString>("source");
        QTest::addColumn<QString>("token");
        QTest::addColumn<QString>("color");
        QTest::newRow("log") << "log" << "2026-11-05 12:00:00 ERROR broken" << "ERROR" << "#e06c75";
        QTest::newRow("json-key") << "json" << "{\"name\": \"value\"}" << "name" << "#61afef";
        QTest::newRow("json-string") << "json" << "{\"name\": \"value\"}" << "value" << "#98c379";
        QTest::newRow("yaml") << "yaml" << "service: \"api\" # note" << "service" << "#61afef";
        QTest::newRow("toml") << "toml" << "port = 8080" << "port" << "#61afef";
        QTest::newRow("python") << "python" << "def run(): # comment" << "def" << "#c678dd";
        QTest::newRow("javascript") << "javascript" << "const value = \"hi\";" << "const" << "#c678dd";
        QTest::newRow("lua") << "lua" << "local value = 'hi'" << "local" << "#c678dd";
        QTest::newRow("shell") << "shell" << "export PATH=\"$HOME/bin\"" << "export" << "#c678dd";
        QTest::newRow("typescript") << "typescript" << "interface Item { value: string }" << "interface" << "#61afef";
        QTest::newRow("markdown") << "markdown" << "# Heading" << "Heading" << "#e5c07b";
    }

    void highlightsRequestedLanguages() {
        QFETCH(QString, language);
        QFETCH(QString, source);
        QFETCH(QString, token);
        QFETCH(QString, color);
        QTextDocument document;
        document.setPlainText(source);
        SyntaxHighlighter highlighter(&document);
        highlighter.setLanguage(language);
        highlighter.rehighlight();
        const QTextBlock block = document.firstBlock();
        const int position = source.indexOf(token);
        QVERIFY(position >= 0);
        QColor found;
        for (const auto &range : block.layout()->formats()) {
            if (range.start <= position && position < range.start + range.length)
                found = range.format.foreground().color();
        }
        QCOMPARE(found, QColor(color));
    }

    void keepsMultilineStringsAndCommentsColored_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<QString>("source");
        QTest::addColumn<QString>("token");
        QTest::addColumn<QString>("color");
        QTest::newRow("python-triple") << "python" << "\"\"\"start\ndef\n\"\"\"" << "def" << "#98c379";
        QTest::newRow("js-block") << "javascript" << "/* start\nconst\n*/" << "const" << "#7f8c93";
        QTest::newRow("lua-long-comment") << "lua" << "--[[ start\nlocal\n]]" << "local" << "#7f8c93";
        QTest::newRow("toml-triple") << "toml" << "\"\"\"start\ntrue\n\"\"\"" << "true" << "#98c379";
        QTest::newRow("shell-quote") << "shell" << "'start\nif\n'" << "if" << "#98c379";
    }

    void keepsMultilineStringsAndCommentsColored() {
        QFETCH(QString, language);
        QFETCH(QString, source);
        QFETCH(QString, token);
        QFETCH(QString, color);
        QTextDocument document;
        document.setPlainText(source);
        SyntaxHighlighter highlighter(&document);
        highlighter.setLanguage(language);
        highlighter.rehighlight();
        const QTextCursor match = document.find(token);
        QVERIFY(!match.isNull());
        const QTextBlock block = match.block();
        const int position = match.selectionStart() - block.position();
        QColor found;
        for (const auto &range : block.layout()->formats()) {
            if (range.start <= position && position < range.start + range.length)
                found = range.format.foreground().color();
        }
        QCOMPARE(found, QColor(color));
    }

    void closingOneTabKeepsOtherNote() {
        QTemporaryDir directory;
        const QString path = directory.filePath("notes.json");
        Backend notes(path);
        notes.setText("Keep me");
        notes.addTab();
        notes.setText("Delete me");
        notes.closeTab(1);
        QVERIFY(notes.flush());
        Backend reopened(path);
        QCOMPARE(reopened.tabs().size(), 1);
        QCOMPARE(reopened.text(), QString("Keep me"));
    }

    void importsEarlierSingleNote() {
        QTemporaryDir directory;
        QFile oldNote(directory.filePath("note.txt"));
        QVERIFY(oldNote.open(QIODevice::WriteOnly));
        oldNote.write("Earlier text");
        oldNote.close();
        Backend notes(directory.filePath("notes.json"));
        QCOMPARE(notes.text(), QString("Earlier text"));
        QVERIFY(notes.flush());
        QVERIFY(QFile::exists(oldNote.fileName()));
    }

    void doesNotOverwriteUnreadableNotes() {
        QTemporaryDir directory;
        const QString path = directory.filePath("notes.json");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("invalid json");
        file.close();
        Backend notes(path);
        notes.setText("New text");
        QVERIFY(!notes.flush());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("invalid json"));
    }

    void keepsWindowOpenOnSaveFailure() {
        QTemporaryDir directory;
        const QString path = directory.filePath("occupied/note.txt");
        QFile occupied(directory.filePath("occupied"));
        QVERIFY(occupied.open(QIODevice::WriteOnly));
        occupied.close();
        Backend note(path);
        note.setText("Do not lose this");
        QVERIFY(!note.flush());
        QVERIFY(note.hasError());
        QCOMPARE(note.text(), QString("Do not lose this"));
    }
};

QTEST_MAIN(KeepnoteTests)
#include "keepnote_tests.moc"
