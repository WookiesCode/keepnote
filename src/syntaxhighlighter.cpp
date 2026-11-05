#include "syntaxhighlighter.h"

#include <QColor>
#include <QRegularExpression>
#include <QTextDocument>

namespace {
QTextCharFormat colored(const char *hex, bool bold = false) {
    QTextCharFormat format;
    format.setForeground(QColor(hex));
    if (bold)
        format.setFontWeight(QFont::Bold);
    return format;
}

int closingQuote(const QString &text, int start, QChar quote, bool escaped) {
    for (int i = start; i < text.size(); ++i) {
        if (escaped && text.at(i) == QLatin1Char('\\')) {
            ++i;
            continue;
        }
        if (text.at(i) == quote)
            return i;
    }
    return -1;
}
}

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *document)
    : QSyntaxHighlighter(document),
      m_comment(colored("#7f8c93")), m_string(colored("#98c379")),
      m_number(colored("#d19a66")), m_keyword(colored("#c678dd", true)),
      m_key(colored("#61afef")), m_heading(colored("#e5c07b", true)),
      m_error(colored("#e06c75", true)), m_warning(colored("#e5c07b", true)) {}

void SyntaxHighlighter::setLanguage(const QString &language) {
    if (m_language == language)
        return;
    m_language = language;
    rehighlight();
}

void SyntaxHighlighter::applyPattern(const QString &text, const QRegularExpression &pattern,
                                    const QTextCharFormat &format,
                                    const QVector<bool> &protectedChars, int capture) {
    auto matches = pattern.globalMatch(text);
    while (matches.hasNext()) {
        const auto match = matches.next();
        const int start = match.capturedStart(capture);
        const int length = match.capturedLength(capture);
        if (start < 0 || length <= 0)
            continue;
        bool blocked = false;
        for (int i = start; i < start + length; ++i) {
            if (protectedChars.at(i)) {
                blocked = true;
                break;
            }
        }
        if (!blocked)
            setFormat(start, length, format);
    }
}

void SyntaxHighlighter::highlightBlock(const QString &text) {
    if (m_language == "plain") {
        setCurrentBlockState(Normal);
        return;
    }
    if (m_language == "markdown") {
        highlightMarkdown(text);
        return;
    }
    if (m_language == "log") {
        highlightLog(text);
        return;
    }
    highlightCode(text);
}

void SyntaxHighlighter::highlightMarkdown(const QString &text) {
    const bool fence = text.trimmed().startsWith("```") || text.trimmed().startsWith("~~~");
    const bool inside = previousBlockState() == MarkdownFence;
    if (fence || inside) {
        setFormat(0, text.size(), fence ? m_comment : m_string);
        setCurrentBlockState(fence ? (inside ? Normal : MarkdownFence) : MarkdownFence);
        return;
    }
    setCurrentBlockState(Normal);
    static const QRegularExpression heading(R"(^\s{0,3}#{1,6}\s+.+$)");
    static const QRegularExpression link(R"(\[[^\]]+\]\([^)]+\))");
    static const QRegularExpression code(R"(`[^`]+`)");
    static const QRegularExpression emphasis(R"(\*\*[^*]+\*\*|\*[^*]+\*)");
    if (heading.match(text).hasMatch())
        setFormat(0, text.size(), m_heading);
    const QVector<bool> unprotected(text.size(), false);
    applyPattern(text, link, m_key, unprotected);
    applyPattern(text, emphasis, m_keyword, unprotected);
    applyPattern(text, code, m_string, unprotected);
}

void SyntaxHighlighter::highlightLog(const QString &text) {
    setCurrentBlockState(Normal);
    static const QRegularExpression timestamp(R"(^\s*\[?\d{4}-\d{2}-\d{2}[ T]\d{2}:\d{2}:\d{2}(?:[.,]\d+)?\]?)");
    static const QRegularExpression error(R"(\b(?:ERROR|ERR|FATAL|CRITICAL)\b)", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression warning(R"(\b(?:WARN|WARNING)\b)", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression info(R"(\b(?:INFO|DEBUG|TRACE)\b)", QRegularExpression::CaseInsensitiveOption);
    const QVector<bool> unprotected(text.size(), false);
    applyPattern(text, timestamp, m_comment, unprotected);
    applyPattern(text, info, m_key, unprotected);
    applyPattern(text, warning, m_warning, unprotected);
    applyPattern(text, error, m_error, unprotected);
}

void SyntaxHighlighter::highlightCode(const QString &text) {
    const bool python = m_language == "python";
    const bool javascript = m_language == "javascript" || m_language == "typescript";
    const bool shell = m_language == "shell";
    const bool lua = m_language == "lua";
    const bool toml = m_language == "toml";
    const bool yaml = m_language == "yaml";
    const bool json = m_language == "json";
    QVector<bool> protectedChars(text.size(), false);
    int state = previousBlockState();
    if (state < Normal || state > ShellDouble)
        state = Normal;
    int i = 0;

    auto paint = [&](int from, int to, const QTextCharFormat &format) {
        if (to <= from)
            return;
        setFormat(from, to - from, format);
        for (int n = from; n < to; ++n)
            protectedChars[n] = true;
    };

    while (i < text.size()) {
        if (state != Normal) {
            const int start = i;
            QString end;
            if (state == BlockComment) end = "*/";
            else if (state == TripleSingle) end = "'''";
            else if (state == TripleDouble) end = "\"\"\"";
            else if (state == Backtick) end = "`";
            else if (state == LuaLongComment || state == LuaLongString) end = "]]";
            else end = state == ShellSingle ? "'" : "\"";
            int close = -1;
            if (state == Backtick || state == ShellDouble)
                close = closingQuote(text, i, end.at(0), true);
            else if (state == ShellSingle)
                close = closingQuote(text, i, end.at(0), false);
            else
                close = text.indexOf(end, i);
            const int next = close < 0 ? text.size() : close + end.size();
            paint(start, next, state == BlockComment || state == LuaLongComment ? m_comment : m_string);
            i = next;
            if (close >= 0)
                state = Normal;
            continue;
        }

        const bool hashComment = (python || toml || yaml || shell) && text.at(i) == QLatin1Char('#')
            && ((!shell && !yaml) || i == 0 || text.at(i - 1).isSpace());
        if ((javascript && text.mid(i, 2) == "//") || (lua && text.mid(i, 2) == "--" && text.mid(i, 4) != "--[[") || hashComment) {
            paint(i, text.size(), m_comment);
            break;
        }
        if (javascript && text.mid(i, 2) == "/*") {
            state = BlockComment;
            const int start = i;
            i += 2;
            const int close = text.indexOf("*/", i);
            i = close < 0 ? text.size() : close + 2;
            paint(start, i, m_comment);
            if (close >= 0) state = Normal;
            continue;
        }
        if (lua && text.mid(i, 4) == "--[[") {
            state = LuaLongComment;
            const int start = i;
            i += 4;
            const int close = text.indexOf("]]", i);
            i = close < 0 ? text.size() : close + 2;
            paint(start, i, m_comment);
            if (close >= 0) state = Normal;
            continue;
        }
        if (lua && text.mid(i, 2) == "[[") {
            state = LuaLongString;
            const int start = i;
            i += 2;
            const int close = text.indexOf("]]", i);
            i = close < 0 ? text.size() : close + 2;
            paint(start, i, m_string);
            if (close >= 0) state = Normal;
            continue;
        }
        if ((python || toml) && (text.mid(i, 3) == "'''" || text.mid(i, 3) == "\"\"\"")) {
            state = text.at(i) == QLatin1Char('\'') ? TripleSingle : TripleDouble;
            const int start = i;
            i += 3;
            const QString delimiter = state == TripleSingle ? "'''" : "\"\"\"";
            const int close = text.indexOf(delimiter, i);
            i = close < 0 ? text.size() : close + 3;
            paint(start, i, m_string);
            if (close >= 0) state = Normal;
            continue;
        }
        if (javascript && text.at(i) == QLatin1Char('`')) {
            state = Backtick;
            const int start = i++;
            const int close = closingQuote(text, i, QLatin1Char('`'), true);
            i = close < 0 ? text.size() : close + 1;
            paint(start, i, m_string);
            if (close >= 0) state = Normal;
            continue;
        }
        if (text.at(i) == QLatin1Char('"') || (!json && text.at(i) == QLatin1Char('\''))) {
            const QChar quote = text.at(i);
            const int start = i++;
            const int close = closingQuote(text, i, quote, !(shell && quote == QLatin1Char('\'')));
            i = close < 0 ? text.size() : close + 1;
            paint(start, i, m_string);
            if (close < 0 && shell)
                state = quote == QLatin1Char('\'') ? ShellSingle : ShellDouble;
            continue;
        }
        ++i;
    }
    setCurrentBlockState(state);

    static const QRegularExpression numbers(R"((?<![\w])(?:0[xX][0-9a-fA-F]+|\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)(?![\w]))");
    applyPattern(text, numbers, m_number, protectedChars);

    if (json) {
        static const QRegularExpression values(R"(\b(?:true|false|null)\b)");
        static const QRegularExpression keys(R"("(?:\\.|[^"\\])*"(?=\s*:))");
        applyPattern(text, values, m_keyword, protectedChars);
        auto matches = keys.globalMatch(text);
        while (matches.hasNext()) {
            const auto match = matches.next();
            setFormat(match.capturedStart(), match.capturedLength(), m_key);
        }
        return;
    }
    if (yaml || toml) {
        static const QRegularExpression yamlKey(R"(^\s*(?:-\s*)?([\w.-]+)(?=\s*:))");
        static const QRegularExpression tomlKey(R"(^\s*([\w.-]+)\s*=)");
        static const QRegularExpression tomlSection(R"(^\s*\[\[?[^\]]+\]\]?\s*$)");
        static const QRegularExpression values(R"(\b(?:true|false|yes|no|null)\b)", QRegularExpression::CaseInsensitiveOption);
        applyPattern(text, yaml ? yamlKey : tomlKey, m_key, protectedChars, 1);
        applyPattern(text, values, m_keyword, protectedChars);
        if (toml)
            applyPattern(text, tomlSection, m_heading, protectedChars);
        return;
    }
    if (python) {
        static const QRegularExpression keywords(R"(\b(?:and|as|assert|async|await|break|class|continue|def|del|elif|else|except|False|finally|for|from|global|if|import|in|is|lambda|None|nonlocal|not|or|pass|raise|return|self|True|try|while|with|yield|match|case)\b)");
        static const QRegularExpression decorator(R"(^\s*@\w+(?:\.\w+)*)");
        applyPattern(text, keywords, m_keyword, protectedChars);
        applyPattern(text, decorator, m_key, protectedChars);
    } else if (javascript) {
        static const QRegularExpression jsKeywords(R"(\b(?:async|await|break|case|catch|class|const|continue|debugger|default|delete|do|else|export|extends|false|finally|for|from|function|if|import|in|instanceof|let|new|null|of|return|static|super|switch|this|throw|true|try|typeof|var|void|while|yield)\b)");
        static const QRegularExpression tsKeywords(R"(\b(?:abstract|any|as|asserts|boolean|declare|enum|implements|infer|interface|is|keyof|module|namespace|never|number|private|protected|public|readonly|require|string|type|unknown)\b)");
        applyPattern(text, jsKeywords, m_keyword, protectedChars);
        if (m_language == "typescript")
            applyPattern(text, tsKeywords, m_key, protectedChars);
    } else if (lua) {
        static const QRegularExpression keywords(R"(\b(?:and|break|do|else|elseif|end|false|for|function|goto|if|in|local|nil|not|or|repeat|return|then|true|until|while)\b)");
        applyPattern(text, keywords, m_keyword, protectedChars);
    } else if (shell) {
        static const QRegularExpression keywords(R"(\b(?:case|do|done|elif|else|esac|export|fi|for|function|if|in|local|read|return|select|then|until|while)\b)");
        static const QRegularExpression variables(R"(\$\{?[A-Za-z_][A-Za-z0-9_]*\}?)");
        applyPattern(text, keywords, m_keyword, protectedChars);
        applyPattern(text, variables, m_key, protectedChars);
    }
}
