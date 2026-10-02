#pragma once

#include <QObject>
#include <QPointer>
#include <QQuickTextDocument>
#include <QSyntaxHighlighter>
#include <QtQml/qqmlregistration.h>

// 给 QML 里的 TextEdit 加语法高亮：
//   TextEdit { id: edit }
//   CodeHighlighter { document: edit.textDocument; language: "cpp"; dark: Theme.dark }
class CodeHighlighter : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QQuickTextDocument *document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(bool dark READ dark WRITE setDark NOTIFY darkChanged)

public:
    explicit CodeHighlighter(QObject *parent = nullptr);
    ~CodeHighlighter() override;

    QQuickTextDocument *document() const { return m_document; }
    void setDocument(QQuickTextDocument *doc);

    QString language() const { return m_language; }
    void setLanguage(const QString &language);

    bool dark() const { return m_dark; }
    void setDark(bool dark);

signals:
    void documentChanged();
    void languageChanged();
    void darkChanged();

private:
    void rebuild();

    QPointer<QQuickTextDocument> m_document;
    QString m_language = QStringLiteral("cpp");
    bool m_dark = true;
    QPointer<QSyntaxHighlighter> m_highlighter; // 父对象是文档，文档销毁时自动置空
};
