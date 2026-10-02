#include "SourceProvider.h"

#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QQmlEngine>
#include <QRegularExpression>
#include <QTimer>
#include <algorithm>
#include <climits>

namespace {

// 一个 region 的行范围 [begin, end)，不含标记行本身
struct RegionRange {
    int begin = -1;
    int end = -1;
};

RegionRange findRegion(const QStringList &lines, const QString &name)
{
    static const QRegularExpression startRe(QStringLiteral(R"(^\s*//\s*\[region\s+([\w\-.]+)\])"));
    static const QRegularExpression endRe(QStringLiteral(R"(^\s*//\s*\[endregion\])"));

    RegionRange r;
    int depth = 0;
    for (int i = 0; i < lines.size(); ++i) {
        if (r.begin < 0) {
            const auto m = startRe.match(lines[i]);
            if (m.hasMatch() && m.captured(1) == name)
                r.begin = i + 1;
            continue;
        }
        if (startRe.match(lines[i]).hasMatch()) {
            ++depth;
        } else if (endRe.match(lines[i]).hasMatch()) {
            if (depth == 0) {
                r.end = i;
                return r;
            }
            --depth;
        }
    }
    if (r.begin >= 0)
        r.end = int(lines.size());
    return r;
}

} // namespace

SourceProvider::SourceProvider(QObject *parent)
    : QObject(parent)
    , m_devMode(qEnvironmentVariableIntValue("VC_DEV") == 1
                && QFileInfo::exists(QStringLiteral(VC_SOURCE_DIR "/CMakeLists.txt")))
{
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &file) {
        // 很多编辑器保存时是「写临时文件再改名」，原文件会从监视列表里消失，需要重新加入
        QTimer::singleShot(100, this, [this, file] {
            if (QFileInfo::exists(file) && !m_watcher.files().contains(file))
                m_watcher.addPath(file);
            const QString rel = QString(file).remove(0, QStringLiteral(VC_SOURCE_DIR "/").size());
            emit fileChanged(rel);
        });
    });
}

QString SourceProvider::monoFont() const
{
    return QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
}

QString SourceProvider::diskPath(const QString &path) const
{
    return m_devMode ? QStringLiteral(VC_SOURCE_DIR "/") + path : QStringLiteral(":/") + path;
}

QString SourceProvider::read(const QString &path) const
{
    QFile f(diskPath(path));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return tr("（找不到文件：%1）").arg(path);
    return QString::fromUtf8(f.readAll());
}

QString SourceProvider::region(const QString &path, const QString &name) const
{
    const QStringList lines = read(path).split(QLatin1Char('\n'));
    const RegionRange r = findRegion(lines, name);
    if (r.begin < 0)
        return tr("（%1 中没有 region「%2」）").arg(path, name);

    static const QRegularExpression markerRe(QStringLiteral(R"(^\s*//\s*\[(region\s+[\w\-.]+|endregion)\])"));
    QStringList out;
    for (int i = r.begin; i < r.end; ++i) {
        if (!markerRe.match(lines[i]).hasMatch())
            out << lines[i];
    }

    // 去掉公共缩进，空行不参与计算
    int indent = INT_MAX;
    for (const QString &l : out) {
        if (l.trimmed().isEmpty())
            continue;
        int n = 0;
        while (n < l.size() && l[n] == QLatin1Char(' '))
            ++n;
        indent = std::min(indent, n);
    }
    if (indent == INT_MAX)
        indent = 0;
    for (QString &l : out)
        l = l.mid(std::min<int>(indent, int(l.size())));

    while (!out.isEmpty() && out.last().trimmed().isEmpty())
        out.removeLast();
    return out.join(QLatin1Char('\n'));
}

int SourceProvider::regionLine(const QString &path, const QString &name) const
{
    const RegionRange r = findRegion(read(path).split(QLatin1Char('\n')), name);
    return r.begin < 0 ? 0 : r.begin + 1;
}

QUrl SourceProvider::contentUrl(const QString &path) const
{
    return m_devMode ? QUrl::fromLocalFile(diskPath(path)) : QUrl(QStringLiteral("qrc:/") + path);
}

void SourceProvider::watch(const QString &path)
{
    if (!m_devMode)
        return;
    const QString file = diskPath(path);
    if (!m_watcher.files().contains(file))
        m_watcher.addPath(file);
}

void SourceProvider::clearCache()
{
    if (QQmlEngine *engine = qmlEngine(this))
        engine->clearComponentCache();
}
