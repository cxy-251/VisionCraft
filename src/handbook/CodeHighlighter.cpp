#include "CodeHighlighter.h"

#include <QRegularExpression>
#include <utility>
#include <QSyntaxHighlighter>
#include <QTextDocument>

// ---------------------------------------------------------------------------
// 规则式高亮：每条规则是一个正则 + 一种格式，按顺序叠加，后面的覆盖前面的。
// 注释和字符串放最后，保证其中的关键字不会被染色。
// ---------------------------------------------------------------------------
class Highlighter : public QSyntaxHighlighter {
public:
    Highlighter(QTextDocument *doc, const QString &language, bool dark)
        : QSyntaxHighlighter(doc)
    {
        // 配色：深色基于 Tailwind 400 色阶，浅色基于 700 色阶
        const QColor keyword  = dark ? QColor("#c084fc") : QColor("#7e22ce");
        const QColor type     = dark ? QColor("#38bdf8") : QColor("#0369a1");
        const QColor func     = dark ? QColor("#fbbf24") : QColor("#b45309");
        const QColor number   = dark ? QColor("#fb923c") : QColor("#c2410c");
        const QColor string   = dark ? QColor("#4ade80") : QColor("#15803d");
        const QColor comment  = dark ? QColor("#64748b") : QColor("#64748b");
        const QColor preproc  = dark ? QColor("#f472b6") : QColor("#be185d");

        auto fmt = [](const QColor &c, bool bold = false, bool italic = false) {
            QTextCharFormat f;
            f.setForeground(c);
            if (bold)
                f.setFontWeight(QFont::DemiBold);
            f.setFontItalic(italic);
            return f;
        };

        if (language == QLatin1String("text")) {   // 程序输出等纯文本：不染色
            m_blockComments = false;
            return;
        }

        QStringList keywords;
        if (language == QLatin1String("qml")) {
            keywords = {"import", "property", "readonly", "required", "default", "signal", "function",
                        "var", "let", "const", "if", "else", "for", "while", "return", "true", "false",
                        "null", "undefined", "on", "as", "alias", "component", "pragma", "this", "new"};
        } else if (language == QLatin1String("shell")) {
            keywords = {"if", "then", "else", "elif", "fi", "for", "do", "done", "while", "case", "esac",
                        "export", "source", "set", "unset", "echo", "exit", "proc", "return", "foreach", "expr"};
        } else if (language == QLatin1String("cmake")) {
            keywords = {"if", "else", "elseif", "endif", "foreach", "endforeach", "function",
                        "endfunction", "macro", "endmacro", "set", "option", "PRIVATE", "PUBLIC",
                        "INTERFACE", "REQUIRED", "COMPONENTS"};
        } else {
            keywords = {"class", "struct", "union", "enum", "public", "private", "protected", "virtual",
                        "override", "final", "auto", "const", "constexpr", "static", "inline", "extern",
                        "volatile", "return", "if", "else", "switch", "case", "default", "break",
                        "continue", "while", "do", "for", "template", "typename", "using", "namespace",
                        "explicit", "new", "delete", "nullptr", "NULL", "true", "false", "this",
                        "try", "catch", "throw", "noexcept", "sizeof", "typedef", "operator",
                        "signals", "slots", "emit", "Q_OBJECT", "Q_PROPERTY", "Q_INVOKABLE",
                        "Q_SIGNALS", "Q_SLOTS", "Q_EMIT", "QML_ELEMENT", "QML_SINGLETON"};
        }
        for (const QString &k : keywords)
            m_rules.append({QRegularExpression(QStringLiteral("\\b%1\\b").arg(k)), fmt(keyword, true)});

        if (language == QLatin1String("qml")) {
            m_rules.append({QRegularExpression(QStringLiteral(R"(\b[A-Z]\w*\b)")), fmt(type)});
            m_rules.append({QRegularExpression(QStringLiteral(R"(\b[a-z]\w*(?=\s*:))")), fmt(func)});
        } else if (language == QLatin1String("shell")) {
            m_rules.append({QRegularExpression(QStringLiteral(R"(\$\{?\w+\}?)")), fmt(type)});
        } else if (language == QLatin1String("cmake")) {
            m_rules.append({QRegularExpression(QStringLiteral(R"(\b\w+(?=\s*\())")), fmt(func, true)});
            m_rules.append({QRegularExpression(QStringLiteral(R"(\$\{\w+\})")), fmt(type)});
        } else {
            m_rules.append({QRegularExpression(QStringLiteral(
                R"(\b(void|bool|char|short|int|long|float|double|unsigned|signed|size_t|u?int(8|16|32|64)_t)\b)")),
                fmt(type)});
            m_rules.append({QRegularExpression(QStringLiteral(R"(\b(Q[A-Z]\w*|cv::\w+|std::\w+|[A-Z][a-z]\w*(?=::)))")),
                fmt(type)});
            m_rules.append({QRegularExpression(QStringLiteral(R"(\b\w+(?=\s*\())")), fmt(func)});
            m_rules.append({QRegularExpression(QStringLiteral(R"(^\s*#\s*\w+)")), fmt(preproc)});
        }

        m_rules.append({QRegularExpression(QStringLiteral(R"(\b(0x[0-9A-Fa-f]+|\d+(\.\d+)?)[uUlLfF]*\b)")), fmt(number)});
        m_rules.append({QRegularExpression(QStringLiteral(R"("(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)')")), fmt(string)});

        const bool hashComments = language == QLatin1String("cmake") || language == QLatin1String("shell");
        const QString lineComment = hashComments ? QStringLiteral("(^|\\s)#[^\\n]*") : QStringLiteral("//[^\\n]*");
        m_rules.append({QRegularExpression(lineComment), fmt(comment, false, true)});
        m_commentFormat = fmt(comment, false, true);
        m_blockComments = !hashComments;
    }

protected:
    void highlightBlock(const QString &text) override
    {
        for (const Rule &rule : std::as_const(m_rules)) {
            auto it = rule.pattern.globalMatch(text);
            while (it.hasNext()) {
                const auto m = it.next();
                setFormat(int(m.capturedStart()), int(m.capturedLength()), rule.format);
            }
        }
        if (!m_blockComments)
            return;

        // 跨行的 /* ... */：用 blockState 记住「上一行结束时还在注释里」
        static const QRegularExpression startRe(QStringLiteral(R"(/\*)"));
        static const QRegularExpression endRe(QStringLiteral(R"(\*/)"));
        setCurrentBlockState(0);
        qsizetype start = previousBlockState() == 1 ? 0 : text.indexOf(startRe);
        while (start >= 0) {
            const auto m = endRe.match(text, start);
            qsizetype len;
            if (!m.hasMatch()) {
                setCurrentBlockState(1);
                len = text.size() - start;
            } else {
                len = m.capturedEnd() - start;
            }
            setFormat(int(start), int(len), m_commentFormat);
            start = text.indexOf(startRe, start + len);
        }
    }

private:
    struct Rule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QList<Rule> m_rules;
    QTextCharFormat m_commentFormat;
    bool m_blockComments = true;
};

CodeHighlighter::CodeHighlighter(QObject *parent)
    : QObject(parent)
{
}

CodeHighlighter::~CodeHighlighter() = default;

void CodeHighlighter::setDocument(QQuickTextDocument *doc)
{
    if (m_document == doc)
        return;
    m_document = doc;
    rebuild();
    emit documentChanged();
}

void CodeHighlighter::setLanguage(const QString &language)
{
    if (m_language == language)
        return;
    m_language = language;
    rebuild();
    emit languageChanged();
}

void CodeHighlighter::setDark(bool dark)
{
    if (m_dark == dark)
        return;
    m_dark = dark;
    rebuild();
    emit darkChanged();
}

void CodeHighlighter::rebuild()
{
    delete m_highlighter.data();
    if (m_document && m_document->textDocument())
        m_highlighter = new Highlighter(m_document->textDocument(), m_language, m_dark);
}
