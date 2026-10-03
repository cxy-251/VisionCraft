// 手册里每一处 CodeRef 都能取到内容，并且引用的文件会被打包进程序。
//
// （本文件不用原始字符串字面量和双引号字符字面量：moc 的分词器会被它们弄乱，找不到 Q_OBJECT 类）
// 手册正文引用真实文件的片段（region / match / from-to）。文件改了、标记删了、正则写错了，
// 界面上只会出现一行「（……中没有 region……）」，很容易漏看。这个测试把所有引用都取一遍：
//   1. 用程序里同一个 SourceProvider（开发模式，从源码目录读）取片段，不能出现错误提示；
//   2. 写了 to 的 from-to 片段，最后一行必须真的匹配 to（否则会一路读到文件末尾）；
//   3. 引用的文件必须在打包清单（构建生成的 vc_handbook_content.qrc）里，否则发布版找不到。

#include "SourceProvider.h"

#include <QDirIterator>
#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QTest>

namespace {

struct Ref {
    QString qmlFile;
    int line = 0;
    QString kind;   // CodeRef / Entry / Try
    QMap<QString, QString> props;
};

// QML 字符串字面量里的转义：\\ → \，\" → "
QString unescape(const QString &s)
{
    QString out;
    for (int i = 0; i < s.size(); ++i) {
        if (s[i] == QChar(0x5C) /* 反斜杠 */ && i + 1 < s.size()) {
            ++i;
            out += s[i] == QLatin1Char('n') ? QLatin1Char('\n') : s[i];
        } else {
            out += s[i];
        }
    }
    return out;
}

// 取出 CodeRef { … }、Entry { … }、Try { … } 里直接写成字符串的属性
QList<Ref> collect(const QString &root)
{
    static const QRegularExpression blockRe(QString::fromUtf8("\\b(CodeRef|Entry|Try|Figure)\\s*\\{"));
    static const QRegularExpression propRe(QString::fromUtf8("(\\w+)\\s*:\\s*\"((?:[^\"\\\\]|\\\\.)*)\""));
    QList<Ref> refs;
    QDirIterator it(root + QStringLiteral("/handbook"), {QStringLiteral("*.qml")}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        QFile f(path);
        f.open(QIODevice::ReadOnly);
        const QString text = QString::fromUtf8(f.readAll());
        for (auto m = blockRe.globalMatch(text); m.hasNext();) {
            const auto bm = m.next();
            // 找到配对的右括号；Try 的块里可能嵌着别的块，只取到它第一层的属性即可
            // 字符串里的括号不算（正则里常有 "^}"、"\\{"）
            int depth = 1, i = int(bm.capturedEnd());
            bool inString = false;
            while (depth && i < text.size()) {
                const QChar c = text[i];
                if (inString) {
                    if (c == QChar(0x5C) /* 反斜杠 */) ++i;
                    else if (c == QChar(0x22) /* 双引号 */) inString = false;
                } else if (c == QChar(0x22) /* 双引号 */) {
                    inString = true;
                } else if (c == QLatin1Char('{')) {
                    ++depth;
                } else if (c == QLatin1Char('}')) {
                    --depth;
                }
                ++i;
            }
            Ref r;
            r.qmlFile = path.mid(root.size() + 1);
            r.line = int(text.left(bm.capturedStart()).count(QLatin1Char('\n'))) + 1;
            r.kind = bm.captured(1);
            const QString body = text.mid(bm.capturedEnd(), i - bm.capturedEnd() - 1);
            // Figure 的 files 是一个字符串数组：每一项单独记成一条引用
            static const QRegularExpression filesRe(QStringLiteral("files\\s*:\\s*\\[([^\\]]*)\\]"));
            static const QRegularExpression strRe(QStringLiteral("\"([^\"]+)\""));
            if (r.kind == QLatin1String("Figure")) {
                const auto fm = filesRe.match(body);
                for (auto sm = strRe.globalMatch(fm.captured(1)); sm.hasNext();) {
                    Ref img = r;
                    img.props[QStringLiteral("file")] = sm.next().captured(1);
                    refs << img;
                }
                continue;
            }
            for (auto pm = propRe.globalMatch(body); pm.hasNext();) {
                const auto p = pm.next();
                if (!r.props.contains(p.captured(1)))
                    r.props[p.captured(1)] = unescape(p.captured(2));
            }
            refs << r;
        }
    }
    return refs;
}

QSet<QString> bundledFiles()
{
    QFile f(QStringLiteral(VC_HANDBOOK_QRC));
    f.open(QIODevice::ReadOnly);
    static const QRegularExpression aliasRe(QString::fromUtf8("<file alias=\"([^\"]+)\""));
    QSet<QString> out;
    for (auto m = aliasRe.globalMatch(QString::fromUtf8(f.readAll())); m.hasNext();)
        out.insert(m.next().captured(1));
    return out;
}

} // namespace

class TstHandbookRefs : public QObject {
    Q_OBJECT
private slots:
    void initTestCase()
    {
        qputenv("VC_DEV", "1");   // SourceProvider 从源码目录读，和开发模式下的手册一致
    }

    void everyReferenceResolves()
    {
        SourceProvider sp;
        QVERIFY(sp.devMode());
        const QString root = QStringLiteral(VC_SOURCE_DIR);
        const QList<Ref> refs = collect(root);
        const QSet<QString> bundled = bundledFiles();
        QVERIFY2(bundled.size() > 100, "没读到打包清单");

        QStringList problems;
        int checked = 0;
        for (const Ref &r : refs) {
            const QString where = QStringLiteral("%1:%2 %3").arg(r.qmlFile).arg(r.line).arg(r.kind);
            const QString file = r.kind == QLatin1String("Try") ? r.props.value(QStringLiteral("answerFile")) : r.props.value(QStringLiteral("file"));
            if (file.isEmpty())
                continue;   // 没有引用文件的 Entry（待写）、只有文字答案的 Try
            ++checked;
            if (!QFile::exists(root + QLatin1Char('/') + file)) {
                problems << where + QStringLiteral(" 文件不存在：") + file;
                continue;
            }
            if (!bundled.contains(file))
                problems << where + QStringLiteral(" 没有打包进程序：") + file;
            if (r.kind == QLatin1String("Entry") || r.kind == QLatin1String("Figure"))
                continue;

            const QString region = r.props.value(r.kind == QLatin1String("Try") ? QStringLiteral("answerRegion") : QStringLiteral("region"));
            const QString match = r.props.value(QStringLiteral("match"));
            const QString from = r.props.value(QStringLiteral("from"));
            const QString to = r.props.value(QStringLiteral("to"));
            QString code;
            if (!region.isEmpty())
                code = sp.region(file, region);
            else if (!match.isEmpty())
                code = sp.matching(file, match);
            else if (!from.isEmpty())
                code = sp.between(file, from, to.isEmpty() ? QStringLiteral("^}") : to);
            else
                code = sp.read(file);

            if (code.startsWith(QStringLiteral("（")))
                problems << where + QLatin1Char(' ') + code;
            else if (!from.isEmpty() && !to.isEmpty()
                     && !QRegularExpression(to).match(code.section(QLatin1Char('\n'), -1)).hasMatch())
                problems << where + QStringLiteral(" from-to 没有找到结尾「%1」，读到了文件末尾").arg(to);
        }
        qInfo("检查了 %d 处引用", checked);
        QVERIFY2(problems.isEmpty(), qPrintable(QStringLiteral("\n") + problems.join(QLatin1Char('\n'))));
    }
};

QTEST_GUILESS_MAIN(TstHandbookRefs)
#include "tst_handbook_refs.moc"
