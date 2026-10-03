// 上位机的分层规则：下层不能 #include 上层，也不能依赖它用不到的库。
// 规则一旦被打破（比如有人在检测算法里 #include 了 DeviceLink.h），这个测试就失败。

#include <QDirIterator>
#include <QFile>
#include <QRegularExpression>
#include <QTest>

namespace {

// [region rules]
struct Rule {
    const char *dir;          // 这一层的源码目录
    const char *forbidden;    // 不允许出现的 #include（正则）
    const char *why;
};

const Rule kRules[] = {
    {"protocol", "^(Q|opencv2/)",
     "协议是上位机和固件共用的 C 代码，固件里没有 Qt 和 OpenCV"},
    {"src/vision", "^(QtQuick|QtQml|QQuick|QQml|QImage|QPainter|QWidget|DeviceLink|.*Transport|StationController)",
     "检测算法只依赖 OpenCV 和 Qt Core，可以脱离界面和设备单独测试、单独复用"},
    {"src/device", "^(opencv2/|Inspector|PartGenerator|StationController|QQuick)",
     "设备通信不知道检测算法和界面的存在"},
};
// [endregion]

QStringList includesOf(const QString &file)
{
    QFile f(file);
    f.open(QIODevice::ReadOnly);
    static const QRegularExpression inc(QStringLiteral("^\\s*#\\s*include\\s*[<\\x22]([^>\\x22]+)[>\\x22]"),
                                        QRegularExpression::MultilineOption);
    QStringList out;
    for (auto m = inc.globalMatch(QString::fromUtf8(f.readAll())); m.hasNext();)
        out << m.next().captured(1);
    return out;
}

} // namespace

class TstLayering : public QObject {
    Q_OBJECT
private slots:
    void lowerLayersDoNotIncludeUpperOnes()
    {
        QStringList problems;
        for (const Rule &r : kRules) {
            const QRegularExpression bad(QString::fromUtf8(r.forbidden));
            int files = 0;
            QDirIterator it(QStringLiteral(VC_SOURCE_DIR "/") + QString::fromUtf8(r.dir),
                            {QStringLiteral("*.h"), QStringLiteral("*.c"), QStringLiteral("*.cpp")}, QDir::Files);
            while (it.hasNext()) {
                const QString file = it.next();
                ++files;
                for (const QString &inc : includesOf(file))
                    if (bad.match(inc).hasMatch())
                        problems << QStringLiteral("%1 包含了 %2（%3）").arg(file.mid(int(strlen(VC_SOURCE_DIR)) + 1), inc, QString::fromUtf8(r.why));
            }
            qInfo("%s：检查了 %d 个文件", r.dir, files);
            QVERIFY2(files > 0, r.dir);
        }
        QVERIFY2(problems.isEmpty(), qPrintable(QStringLiteral("\n") + problems.join(QLatin1Char('\n'))));
    }
};

QTEST_GUILESS_MAIN(TstLayering)
#include "tst_layering.moc"
