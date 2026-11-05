#pragma once

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QVector>

class SyntaxHighlighter : public QSyntaxHighlighter {
public:
    explicit SyntaxHighlighter(QTextDocument *document);
    void setLanguage(const QString &language);
    QString language() const { return m_language; }

protected:
    void highlightBlock(const QString &text) override;

private:
    enum State {
        Normal = 0,
        BlockComment,
        TripleSingle,
        TripleDouble,
        Backtick,
        LuaLongComment,
        LuaLongString,
        ShellSingle,
        ShellDouble,
        MarkdownFence
    };

    void highlightMarkdown(const QString &text);
    void highlightLog(const QString &text);
    void highlightCode(const QString &text);
    void applyPattern(const QString &text, const QRegularExpression &pattern,
                      const QTextCharFormat &format, const QVector<bool> &protectedChars,
                      int capture = 0);

    QString m_language = QStringLiteral("plain");
    QTextCharFormat m_comment;
    QTextCharFormat m_string;
    QTextCharFormat m_number;
    QTextCharFormat m_keyword;
    QTextCharFormat m_key;
    QTextCharFormat m_heading;
    QTextCharFormat m_error;
    QTextCharFormat m_warning;
};
