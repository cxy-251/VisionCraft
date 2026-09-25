#pragma once

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QVector>

class CppSyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit CppSyntaxHighlighter(QTextDocument *parent = nullptr) : QSyntaxHighlighter(parent) {
        setupRules();
    }

protected:
    void highlightBlock(const QString &text) override {
        // 应用正则规则
        for (const auto &rule : m_highlightRules) {
            auto it = rule.pattern.globalMatch(text);
            while (it.hasNext()) {
                auto match = it.next();
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }

        // 处理多行注释 /* ... */
        setCurrentBlockState(0);
        int startIndex = 0;
        if (previousBlockState() != 1) {
            startIndex = text.indexOf(m_commentStartRegex);
        }

        while (startIndex >= 0) {
            auto match = m_commentEndRegex.match(text, startIndex);
            int endIndex = match.capturedStart();
            int commentLength = 0;
            if (endIndex == -1) {
                setCurrentBlockState(1);
                commentLength = text.length() - startIndex;
            } else {
                commentLength = endIndex - startIndex + match.capturedLength();
            }
            setFormat(startIndex, commentLength, m_multiLineCommentFormat);
            startIndex = text.indexOf(m_commentStartRegex, startIndex + commentLength);
        }
    }

private:
    struct HighlightRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    void setupRules() {
        HighlightRule rule;

        // 1. C++ 关键字 (紫色/洋红)
        QTextCharFormat keywordFormat;
        keywordFormat.setForeground(QColor("#c084fc"));
        keywordFormat.setFontWeight(QFont::Bold);
        const QString keywordPatterns[] = {
            QStringLiteral("\\bclass\\b"), QStringLiteral("\\bstruct\\b"), QStringLiteral("\\bpublic\\b"),
            QStringLiteral("\\bprivate\\b"), QStringLiteral("\\bprotected\\b"), QStringLiteral("\\bvirtual\\b"),
            QStringLiteral("\\boverride\\b"), QStringLiteral("\\bauto\\b"), QStringLiteral("\\bconst\\b"),
            QStringLiteral("\\bconstexpr\\b"), QStringLiteral("\\balignas\\b"), QStringLiteral("\\breturn\\b"),
            QStringLiteral("\\bif\\b"), QStringLiteral("\\belse\\b"), QStringLiteral("\\bwhile\\b"),
            QStringLiteral("\\bfor\\b"), QStringLiteral("\\btemplate\\b"), QStringLiteral("\\btypename\\b"),
            QStringLiteral("\\busing\\b"), QStringLiteral("\\bnamespace\\b"), QStringLiteral("\\bstatic\\b"),
            QStringLiteral("\\bexplicit\\b"), QStringLiteral("\\benum\\b"), QStringLiteral("\\bnew\\b"),
            QStringLiteral("\\bdelete\\b"), QStringLiteral("\\bnullptr\\b"), QStringLiteral("\\btrue\\b"),
            QStringLiteral("\\bfalse\\b"), QStringLiteral("\\btry\\b"), QStringLiteral("\\bcatch\\b")
        };
        for (const auto &pattern : keywordPatterns) {
            rule.pattern = QRegularExpression(pattern);
            rule.format = keywordFormat;
            m_highlightRules.append(rule);
        }

        // 2. 基础数据类型与 Qt/OpenCV 核心类型 (天蓝/青色)
        QTextCharFormat typeFormat;
        typeFormat.setForeground(QColor("#38bdf8"));
        typeFormat.setFontWeight(QFont::DemiBold);
        const QString typePatterns[] = {
            QStringLiteral("\\bvoid\\b"), QStringLiteral("\\bint\\b"), QStringLiteral("\\bdouble\\b"),
            QStringLiteral("\\bfloat\\b"), QStringLiteral("\\bbool\\b"), QStringLiteral("\\bchar\\b"),
            QStringLiteral("\\buchar\\b"), QStringLiteral("\\bsize_t\\b"), QStringLiteral("\\buint\\b"),
            QStringLiteral("\\bcv::Mat\\b"), QStringLiteral("\\bcv::Rect\\b"), QStringLiteral("\\bcv::Point\\b"),
            QStringLiteral("\\bcv::Point2f\\b"), QStringLiteral("\\bcv::Scalar\\b"), QStringLiteral("\\bcv::Size\\b"),
            QStringLiteral("\\bcv::Vec3b\\b"), QStringLiteral("\\bcv::Vec2f\\b"),
            QStringLiteral("\\bQString\\b"), QStringLiteral("\\bQList\\b"), QStringLiteral("\\bQVector\\b"),
            QStringLiteral("\\bQMap\\b"), QStringLiteral("\\bQImage\\b"), QStringLiteral("\\bQPixmap\\b"),
            QStringLiteral("\\bQObject\\b"), QStringLiteral("\\bQWidget\\b"), QStringLiteral("\\bQLabel\\b"),
            QStringLiteral("\\bQPushButton\\b"), QStringLiteral("\\bQThread\\b"), QStringLiteral("\\bQMutex\\b"),
            QStringLiteral("\\bQWaitCondition\\b"), QStringLiteral("\\bQStateMachine\\b"), QStringLiteral("\\bQState\\b"),
            QStringLiteral("\\bQPluginLoader\\b"), QStringLiteral("\\bQTranslator\\b"), QStringLiteral("\\bstd::atomic\\b"),
            QStringLiteral("\\bstd::vector\\b"), QStringLiteral("\\bstd::array\\b"), QStringLiteral("\\bstd::optional\\b")
        };
        for (const auto &pattern : typePatterns) {
            rule.pattern = QRegularExpression(pattern);
            rule.format = typeFormat;
            m_highlightRules.append(rule);
        }

        // 3. Qt 宏与特性 (亮橙色)
        QTextCharFormat macroFormat;
        macroFormat.setForeground(QColor("#fb923c"));
        macroFormat.setFontWeight(QFont::Bold);
        const QString macroPatterns[] = {
            QStringLiteral("\\bQ_OBJECT\\b"), QStringLiteral("\\bsignals\\b"), QStringLiteral("\\bslots\\b"),
            QStringLiteral("\\bQ_PROPERTY\\b"), QStringLiteral("\\bQ_INTERFACES\\b"),
            QStringLiteral("\\bQ_DECLARE_INTERFACE\\b"), QStringLiteral("\\bQ_PLUGIN_METADATA\\b"),
            QStringLiteral("\\btr\\b"), QStringLiteral("\\bemit\\b")
        };
        for (const auto &pattern : macroPatterns) {
            rule.pattern = QRegularExpression(pattern);
            rule.format = macroFormat;
            m_highlightRules.append(rule);
        }

        // 4. 预编译指令 (玫瑰红)
        QTextCharFormat preprocFormat;
        preprocFormat.setForeground(QColor("#f43f5e"));
        rule.pattern = QRegularExpression(QStringLiteral("^\\s*#[a-zA-Z_]+"));
        rule.format = preprocFormat;
        m_highlightRules.append(rule);

        // 5. 字符串字面量 (暖黄色)
        QTextCharFormat stringFormat;
        stringFormat.setForeground(QColor("#facc15"));
        rule.pattern = QRegularExpression(QStringLiteral("\".*?\""));
        rule.format = stringFormat;
        m_highlightRules.append(rule);

        // 6. 数值常量 (浅绿色)
        QTextCharFormat numberFormat;
        numberFormat.setForeground(QColor("#4ade80"));
        rule.pattern = QRegularExpression(QStringLiteral("\\b[0-9]+(\\.[0-9]+)?f?\\b"));
        rule.format = numberFormat;
        m_highlightRules.append(rule);

        // 7. 单行注释 (灰蓝色倾斜)
        QTextCharFormat singleLineCommentFormat;
        singleLineCommentFormat.setForeground(QColor("#64748b"));
        singleLineCommentFormat.setFontItalic(true);
        rule.pattern = QRegularExpression(QStringLiteral("//[^\n]*"));
        rule.format = singleLineCommentFormat;
        m_highlightRules.append(rule);

        // 8. 多行注释
        m_multiLineCommentFormat.setForeground(QColor("#64748b"));
        m_multiLineCommentFormat.setFontItalic(true);
        m_commentStartRegex = QRegularExpression(QStringLiteral("/\\*"));
        m_commentEndRegex = QRegularExpression(QStringLiteral("\\*/"));
    }

    QVector<HighlightRule> m_highlightRules;
    QRegularExpression m_commentStartRegex;
    QRegularExpression m_commentEndRegex;
    QTextCharFormat m_multiLineCommentFormat;
};
