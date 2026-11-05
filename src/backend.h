#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <QVector>

class SyntaxHighlighter;

class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString text READ text NOTIFY textChanged)
    Q_PROPERTY(QVariantList tabs READ tabs NOTIFY tabsChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY statusChanged)
    Q_PROPERTY(QString language READ language NOTIFY languageChanged)

public:
    explicit Backend(const QString &storagePath = {}, QObject *parent = nullptr);
    QString text() const;
    QVariantList tabs() const;
    int currentIndex() const { return m_currentIndex; }
    QString status() const { return m_status; }
    bool hasError() const { return m_hasError; }
    QString language() const;
    QString storagePath() const { return m_storagePath; }

    Q_INVOKABLE void setText(const QString &text);
    Q_INVOKABLE void renameTab(int index, const QString &name);
    Q_INVOKABLE void selectTab(int index);
    Q_INVOKABLE void addTab();
    Q_INVOKABLE void closeTab(int index);
    Q_INVOKABLE bool flush();
    Q_INVOKABLE void saveCurrent();
    Q_INVOKABLE void saveCurrentAsDialog();
    Q_INVOKABLE bool saveCurrentTo(const QUrl &url);
    Q_INVOKABLE bool openFile(const QUrl &url);
    Q_INVOKABLE void setLanguage(const QString &language);
    Q_INVOKABLE void attachEditor(QObject *textDocument);

signals:
    void textChanged();
    void tabsChanged();
    void currentIndexChanged();
    void statusChanged();
    void saveFileRequested(const QUrl &suggestedUrl);
    void languageChanged();

private:
    struct Note {
        QString text;
        QString name;
        QString filePath;
        bool crlf = false;
        bool bom = false;
        QString language = QStringLiteral("markdown");
    };
    static QString titleFor(const Note &note, int index);
    static QString languageForPath(const QString &path);
    static QString extensionForLanguage(const QString &language);
    QUrl suggestedFileUrl() const;
    void setStatus(const QString &status, bool error);
    void markDirty();

    QString m_storagePath;
    QVector<Note> m_notes;
    int m_currentIndex = 0;
    QString m_status;
    QTimer m_saveTimer;
    bool m_dirty = false;
    bool m_hasError = false;
    bool m_loadFailed = false;
    SyntaxHighlighter *m_highlighter = nullptr;
};
