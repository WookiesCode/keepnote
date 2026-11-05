#include "backend.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QQuickTextDocument>
#include <QTextDocument>

#include "syntaxhighlighter.h"

Backend::Backend(const QString &storagePath, QObject *parent)
    : QObject(parent),
      m_storagePath(storagePath.isEmpty()
          ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/notes.json"
          : storagePath) {
    QFile file(m_storagePath);
    if (file.exists()) {
        if (!file.open(QIODevice::ReadOnly)) {
            m_loadFailed = true;
            setStatus(tr("Could not read your notes: %1").arg(file.errorString()), true);
        } else {
            QJsonParseError error;
            const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
            const QJsonObject root = document.object();
            const QJsonArray notes = root.value("tabs").toArray();
            if (error.error != QJsonParseError::NoError || !document.isObject() || notes.isEmpty()) {
                m_loadFailed = true;
                setStatus(tr("Could not read your notes. The notes file was left untouched."), true);
            } else {
                for (const QJsonValue &value : notes) {
                    const QJsonObject note = value.toObject();
                    if (!value.isObject() || !note.value("text").isString() ||
                        (note.contains("name") && !note.value("name").isString()) ||
                        (note.contains("filePath") && !note.value("filePath").isString()) ||
                        (note.contains("crlf") && !note.value("crlf").isBool()) ||
                        (note.contains("bom") && !note.value("bom").isBool()) ||
                        (note.contains("language") && !note.value("language").isString())) {
                        m_loadFailed = true;
                        setStatus(tr("Could not read your notes. The notes file was left untouched."), true);
                        m_notes.clear();
                        break;
                    }
                    const QString filePath = note.value("filePath").toString();
                    const QString savedLanguage = note.value("language").toString();
                    m_notes.append({note.value("text").toString(), note.value("name").toString(),
                        filePath, note.value("crlf").toBool(), note.value("bom").toBool(),
                        savedLanguage.isEmpty() ? (filePath.isEmpty() ? QStringLiteral("markdown")
                            : languageForPath(filePath)) : savedLanguage});
                }
                if (!m_loadFailed)
                    m_currentIndex = qBound(0, root.value("currentIndex").toInt(), m_notes.size() - 1);
            }
        }
    } else {
        // Keep existing single-note text when upgrading from the first version.
        QFile oldNote(QFileInfo(m_storagePath).dir().filePath("note.txt"));
        if (oldNote.exists()) {
            if (oldNote.open(QIODevice::ReadOnly)) {
                m_notes.append({QString::fromUtf8(oldNote.readAll()), {}, {}});
                m_dirty = true;
            } else {
                m_loadFailed = true;
                setStatus(tr("Could not read your earlier note: %1").arg(oldNote.errorString()), true);
            }
        }
    }
    if (m_notes.isEmpty())
        m_notes.append(Note{});

    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(250);
    connect(&m_saveTimer, &QTimer::timeout, this, &Backend::flush);
    if (m_dirty)
        m_saveTimer.start();
}

QString Backend::text() const {
    return m_notes.at(m_currentIndex).text;
}

QString Backend::language() const {
    return m_notes.at(m_currentIndex).language;
}

QString Backend::languageForPath(const QString &path) {
    const QString name = QFileInfo(path).fileName().toLower();
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (name == ".bashrc" || name == ".zshrc" || name == ".profile") return "shell";
    if (suffix == "md" || suffix == "markdown" || suffix == "mdown") return "markdown";
    if (suffix == "log") return "log";
    if (suffix == "json" || suffix == "jsonl") return "json";
    if (suffix == "yaml" || suffix == "yml") return "yaml";
    if (suffix == "toml") return "toml";
    if (suffix == "py" || suffix == "pyw") return "python";
    if (suffix == "js" || suffix == "mjs" || suffix == "cjs" || suffix == "jsx") return "javascript";
    if (suffix == "lua") return "lua";
    if (suffix == "sh" || suffix == "bash" || suffix == "zsh") return "shell";
    if (suffix == "ts" || suffix == "tsx" || suffix == "mts" || suffix == "cts") return "typescript";
    return "plain";
}

QString Backend::extensionForLanguage(const QString &language) {
    if (language == "markdown") return ".md";
    if (language == "log") return ".log";
    if (language == "json") return ".json";
    if (language == "yaml") return ".yaml";
    if (language == "toml") return ".toml";
    if (language == "python") return ".py";
    if (language == "javascript") return ".js";
    if (language == "lua") return ".lua";
    if (language == "shell") return ".sh";
    if (language == "typescript") return ".ts";
    return ".txt";
}

void Backend::setLanguage(const QString &language) {
    static const QStringList supported = {"plain", "markdown", "log", "json", "yaml", "toml",
        "python", "javascript", "lua", "shell", "typescript"};
    if (!supported.contains(language) || m_notes[m_currentIndex].language == language)
        return;
    m_notes[m_currentIndex].language = language;
    if (m_highlighter)
        m_highlighter->setLanguage(language);
    emit languageChanged();
    markDirty();
}

void Backend::attachEditor(QObject *textDocument) {
    auto *quickDocument = qobject_cast<QQuickTextDocument *>(textDocument);
    if (!quickDocument || !quickDocument->textDocument())
        return;
    if (m_highlighter)
        delete m_highlighter;
    m_highlighter = new SyntaxHighlighter(quickDocument->textDocument());
    m_highlighter->setLanguage(language());
}

QString Backend::titleFor(const Note &note, int index) {
    if (!note.name.isEmpty())
        return note.name;
    const QString firstLine = note.text.section('\n', 0, 0).trimmed();
    if (firstLine.isEmpty())
        return tr("Note %1").arg(index + 1);
    return firstLine.left(22) + (firstLine.size() > 22 ? QStringLiteral("…") : QString());
}

QVariantList Backend::tabs() const {
    QVariantList result;
    for (int i = 0; i < m_notes.size(); ++i)
        result.append(QVariantMap{{"title", titleFor(m_notes.at(i), i)},
            {"name", m_notes.at(i).name}, {"index", i}});
    return result;
}

void Backend::setText(const QString &text) {
    Note &note = m_notes[m_currentIndex];
    if (note.text == text)
        return;
    const QString oldTitle = titleFor(note, m_currentIndex);
    note.text = text;
    emit textChanged();
    if (titleFor(note, m_currentIndex) != oldTitle)
        emit tabsChanged();
    markDirty();
}

void Backend::renameTab(int index, const QString &name) {
    if (index < 0 || index >= m_notes.size())
        return;
    const QString cleanName = name.section('\n', 0, 0).trimmed();
    if (m_notes[index].name == cleanName)
        return;
    m_notes[index].name = cleanName;
    emit tabsChanged();
    markDirty();
}

void Backend::selectTab(int index) {
    if (index < 0 || index >= m_notes.size() || index == m_currentIndex)
        return;
    m_currentIndex = index;
    if (m_highlighter)
        m_highlighter->setLanguage(language());
    emit currentIndexChanged();
    emit textChanged();
    emit languageChanged();
    markDirty();
}

void Backend::addTab() {
    m_notes.append(Note{});
    m_currentIndex = m_notes.size() - 1;
    if (m_highlighter)
        m_highlighter->setLanguage(language());
    emit tabsChanged();
    emit currentIndexChanged();
    emit textChanged();
    emit languageChanged();
    markDirty();
}

void Backend::closeTab(int index) {
    if (index < 0 || index >= m_notes.size())
        return;
    if (m_notes.size() == 1) {
        m_notes[0].text.clear();
        m_notes[0].name.clear();
        m_notes[0].filePath.clear();
        m_notes[0].crlf = false;
        m_notes[0].bom = false;
        m_notes[0].language = "markdown";
        if (m_highlighter)
            m_highlighter->setLanguage(language());
        emit textChanged();
    } else {
        m_notes.removeAt(index);
        if (index < m_currentIndex)
            --m_currentIndex;
        else if (m_currentIndex >= m_notes.size())
            m_currentIndex = m_notes.size() - 1;
        if (m_highlighter)
            m_highlighter->setLanguage(language());
        emit currentIndexChanged();
        emit textChanged();
    }
    emit languageChanged();
    emit tabsChanged();
    markDirty();
}

void Backend::markDirty() {
    m_dirty = true;
    if (m_loadFailed)
        return;
    setStatus(tr("Saving…"), false);
    m_saveTimer.start();
}

bool Backend::flush() {
    m_saveTimer.stop();
    if (m_loadFailed)
        return false;
    if (!m_dirty)
        return !m_hasError;

    const QFileInfo info(m_storagePath);
    if (!QDir().mkpath(info.absolutePath())) {
        setStatus(tr("Could not create the notes folder."), true);
        return false;
    }

    QJsonArray notes;
    for (const Note &note : m_notes)
        notes.append(QJsonObject{{"text", note.text}, {"name", note.name},
            {"filePath", note.filePath}, {"crlf", note.crlf}, {"bom", note.bom},
            {"language", note.language}});
    const QByteArray bytes = QJsonDocument(QJsonObject{{"version", 1},
        {"currentIndex", m_currentIndex}, {"tabs", notes}}).toJson();
    QSaveFile file(m_storagePath);
    if (!file.open(QIODevice::WriteOnly)) {
        setStatus(tr("Could not save your notes: %1").arg(file.errorString()), true);
        return false;
    }
    if (file.write(bytes) != bytes.size() || !file.commit()) {
        setStatus(tr("Could not save your notes: %1").arg(file.errorString()), true);
        return false;
    }

    m_dirty = false;
    setStatus(tr("Draft saved automatically"), false);
    return true;
}

QUrl Backend::suggestedFileUrl() const {
    QString name = titleFor(m_notes.at(m_currentIndex), m_currentIndex);
    name.replace(QRegularExpression(QStringLiteral("[^\\p{L}\\p{N} _-]")), QStringLiteral("_"));
    if (name.trimmed().isEmpty())
        name = tr("Note");
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    return QUrl::fromLocalFile(QDir(directory).filePath(name.trimmed() + extensionForLanguage(language())));
}

void Backend::saveCurrent() {
    const QString path = m_notes.at(m_currentIndex).filePath;
    if (path.isEmpty())
        saveCurrentAsDialog();
    else
        saveCurrentTo(QUrl::fromLocalFile(path));
}

void Backend::saveCurrentAsDialog() {
    const QString path = m_notes.at(m_currentIndex).filePath;
    emit saveFileRequested(path.isEmpty() ? suggestedFileUrl() : QUrl::fromLocalFile(path));
}

bool Backend::saveCurrentTo(const QUrl &url) {
    if (!url.isLocalFile() || url.toLocalFile().isEmpty()) {
        setStatus(tr("Choose a local file to save this note."), true);
        return false;
    }
    const QString path = QFileInfo(url.toLocalFile()).absoluteFilePath();
    for (int i = 0; i < m_notes.size(); ++i) {
        if (i != m_currentIndex && m_notes.at(i).filePath == path) {
            setStatus(tr("Another tab is already saved to that file."), true);
            return false;
        }
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        setStatus(tr("Could not save file: %1").arg(file.errorString()), true);
        return false;
    }
    QString output = text();
    if (m_notes.at(m_currentIndex).crlf)
        output.replace(QStringLiteral("\n"), QStringLiteral("\r\n"));
    QByteArray bytes = output.toUtf8();
    if (m_notes.at(m_currentIndex).bom)
        bytes.prepend("\xEF\xBB\xBF");
    if (file.write(bytes) != bytes.size() || !file.commit()) {
        setStatus(tr("Could not save file: %1").arg(file.errorString()), true);
        return false;
    }

    m_notes[m_currentIndex].filePath = path;
    markDirty();
    if (!flush())
        return false;
    setStatus(tr("Saved file: %1").arg(QFileInfo(path).fileName()), false);
    return true;
}

bool Backend::openFile(const QUrl &url) {
    if (m_loadFailed || !url.isLocalFile() || url.toLocalFile().isEmpty()) {
        setStatus(tr("Could not open that file."), true);
        return false;
    }
    const QString path = QFileInfo(url.toLocalFile()).absoluteFilePath();
    for (int i = 0; i < m_notes.size(); ++i) {
        if (m_notes.at(i).filePath == path) {
            selectTab(i);
            return true;
        }
    }

    if (!QFileInfo(path).isFile()) {
        setStatus(tr("Choose a text file to open."), true);
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        setStatus(tr("Could not open file: %1").arg(file.errorString()), true);
        return false;
    }
    QByteArray bytes = file.readAll();
    if (file.error() != QFile::NoError) {
        setStatus(tr("Could not read file: %1").arg(file.errorString()), true);
        return false;
    }
    const bool bom = bytes.startsWith("\xEF\xBB\xBF");
    if (bom)
        bytes.remove(0, 3);
    const QString decoded = QString::fromUtf8(bytes);
    if (decoded.toUtf8() != bytes || bytes.contains('\0')) {
        setStatus(tr("Only UTF-8 text files can be opened."), true);
        return false;
    }
    const bool crlf = bytes.contains("\r\n");
    Note opened{decoded, QFileInfo(path).completeBaseName(), path, crlf, bom, languageForPath(path)};
    opened.text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    if (m_notes.size() == 1 && m_notes[0].text.isEmpty() &&
        m_notes[0].name.isEmpty() && m_notes[0].filePath.isEmpty()) {
        m_notes[0] = opened;
    } else {
        m_notes.append(opened);
        m_currentIndex = m_notes.size() - 1;
    }
    emit tabsChanged();
    emit currentIndexChanged();
    emit textChanged();
    if (m_highlighter)
        m_highlighter->setLanguage(language());
    emit languageChanged();
    markDirty();
    return true;
}

void Backend::setStatus(const QString &status, bool error) {
    if (m_status == status && m_hasError == error)
        return;
    m_status = status;
    m_hasError = error;
    emit statusChanged();
}
