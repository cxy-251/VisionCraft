// QSettings：程序的配置存在哪、存成什么样、读回来是什么类型
//
// 运行：./example_qt_settings     输出见 output.txt
// 为了不碰真正的配置，示例把文件写在临时目录里，用 INI 格式（和本程序在 Linux 上用的格式相同）。

#include <QCoreApplication>
#include <QFile>
#include <QProcess>
#include <QSettings>
#include <QTemporaryDir>
#include <cstdio>

// [region read]
static void readBack(const QString &path)
{
    QSettings s(path, QSettings::IniFormat);
    const QVariant mode = s.value("ui/themeMode");
    std::printf("  ui/themeMode：类型 %s，toInt() = %d\n", mode.typeName(), mode.toInt());
    const QVariant flag = s.value("handbook/expanded");
    std::printf("  handbook/expanded：类型 %s，toBool() = %s，和 QVariant(true) 比较：%s\n", flag.typeName(),
                flag.toBool() ? "true" : "false", flag == QVariant(true) ? "相等" : "不相等");
    std::printf("  handbook/recent：%s\n", qPrintable(s.value("handbook/recent").toStringList().join(", ")));
    std::printf("  不存在的键，给默认值：%d\n", s.value("ui/fontSize", 14).toInt());
    std::fflush(stdout);
}
// [endregion]

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    if (argc == 3 && QByteArray(argv[1]) == "read") {
        readBack(QString::fromLocal8Bit(argv[2]));
        return 0;
    }
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("demo.conf"));

    std::printf("==== 1. 写入 ====\n");
    {
        // [region write]
        QSettings s(path, QSettings::IniFormat);
        s.setValue("ui/themeMode", 2);                        // 斜杠分组：写成 [ui] 下的 themeMode
        s.beginGroup("handbook");
        s.setValue("lastFile", "handbook/qt/qml/Bindings.qml");
        s.setValue("fontScale", 1.25);
        s.setValue("expanded", true);
        s.setValue("recent", QStringList{"a.qml", "b.qml"});
        s.endGroup();
        std::printf("  setValue 之后，文件存在吗？%s\n", QFile::exists(path) ? "存在" : "还不存在");
        s.sync();                                             // 立刻写盘（不调用也会在析构时写）
        std::printf("  sync() 之后，文件存在吗？%s\n", QFile::exists(path) ? "存在" : "还不存在");
        // [endregion]
    }

    std::printf("\n==== 2. 文件内容 ====\n");
    QFile f(path);
    f.open(QIODevice::ReadOnly);
    std::printf("%s\n", f.readAll().constData());

    std::printf("==== 3. 同一个程序里，再建一个 QSettings 读 ====\n");
    readBack(path);

    std::printf("\n==== 4. 另一个进程读（相当于下次启动程序） ====\n");
    std::fflush(stdout);
    QProcess::execute(QCoreApplication::applicationFilePath(), {QStringLiteral("read"), path});
    return 0;
}
