// QFileSystemWatcher：文件被改了，程序怎么知道
//
// 运行：./example_qt_file_watch     输出见 output.txt
// 在临时目录里建一个文件并监视它，用三种常见的「保存」方式改它，记录收到的信号和监视列表的变化。

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileSystemWatcher>
#include <QTemporaryDir>
#include <QTimer>
#include <cstdio>

static void runFor(int ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

static void writeFile(const QString &path, const QByteArray &data, QIODevice::OpenMode mode = QIODevice::WriteOnly)
{
    QFile f(path);
    f.open(mode);
    f.write(data);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTemporaryDir dir;
    const QString file = dir.filePath(QStringLiteral("section.qml"));
    writeFile(file, "v1");

    // [region watch]
    QFileSystemWatcher watcher;
    watcher.addPath(file);
    int events = 0;
    QObject::connect(&watcher, &QFileSystemWatcher::fileChanged, [&](const QString &) { ++events; });
    // [endregion]

    auto step = [&](const char *what, const std::function<void()> &change) {
        events = 0;
        change();
        runFor(300);
        std::printf("fileChanged %d 次，还在监视列表里：%s  ← %s\n", events,
                    watcher.files().contains(file) ? "是" : "否", what);
        std::fflush(stdout);
    };

    // [region saves]
    step("1. 原地改写（打开、清空、写入）", [&] { writeFile(file, "v2", QIODevice::WriteOnly | QIODevice::Truncate); });
    step("2. 追加两次", [&] { writeFile(file, "a", QIODevice::Append); writeFile(file, "b", QIODevice::Append); });
    step("3. 写临时文件再改名覆盖（很多编辑器这样存）", [&] {
        writeFile(file + ".tmp", "v3");
        QFile::remove(file);
        QFile::rename(file + ".tmp", file);
    });
    step("4. 再改一次（没有重新 addPath）", [&] { writeFile(file, "v4", QIODevice::WriteOnly | QIODevice::Truncate); });
    // [endregion]

    // [region readd]
    watcher.addPath(file);                       // 文件已经是新的了，重新加入监视
    step("5. 重新 addPath 之后再改", [&] { writeFile(file, "v5", QIODevice::WriteOnly | QIODevice::Truncate); });
    // [endregion]
    return 0;
}
