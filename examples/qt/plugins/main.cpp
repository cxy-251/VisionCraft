// 插件：主程序运行时扫描目录、读元数据、加载、调用；最后试试「热替换」
//
// 运行：./example_qt_plugins     输出见 output.txt

#include "operator.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QPluginLoader>
#include <cstdio>

static QString pluginDir(const char *variant)
{
    return QCoreApplication::applicationDirPath() + "/plugins/" + variant;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    std::printf("==== 1. 扫描目录，先只读元数据 ====\n");
    // [region scan]
    QDir dir(pluginDir("v1"));
    for (const QString &file : dir.entryList(QDir::Files)) {
        QPluginLoader loader(dir.absoluteFilePath(file));
        const QJsonObject meta = loader.metaData();           // 只读文件里的元数据，不执行插件代码
        std::printf("  %-22s IID=%-30s name=%-8s 已加载=%d\n", qPrintable(file),
                    qPrintable(meta.value("IID").toString()),
                    qPrintable(meta.value("MetaData").toObject().value("name").toString()), loader.isLoaded());
    }
    // [endregion]

    std::printf("\n==== 2. 加载并调用 ====\n");
    {
        // [region load]
        QPluginLoader loader(pluginDir("v1") + "/libinvert.so");
        QObject *obj = loader.instance();                     // 这一步才真正 dlopen、创建插件对象
        auto *op = qobject_cast<Operator *>(obj);             // 按接口 IID 检查类型
        // [endregion]
        if (op) {
            const QByteArray out = op->apply(QByteArray("\x00\x10\xff", 3));
            std::printf("  %s：00 10 ff → %02x %02x %02x\n", qPrintable(op->name()), uchar(out[0]), uchar(out[1]), uchar(out[2]));
        }
    }

    std::printf("\n==== 3. 接口版本不匹配 ====\n");
    for (const char *file : {"libthreshold_old.so", "libmislabeled.so"}) {
        QPluginLoader loader(pluginDir("v1") + "/" + file);
        QObject *obj = loader.instance();
        std::printf("  %-20s 元数据 IID %s；instance() %s，qobject_cast<Operator *> %s\n", file,
                    qPrintable(loader.metaData().value("IID").toString()), obj ? "非空" : "为空",
                    qobject_cast<Operator *>(obj) ? "成功" : "得到 nullptr");
    }

    std::printf("\n==== 4. 不是插件的库 ====\n");
    {
        QPluginLoader loader("/usr/lib/libz.so.1");
        std::printf("  instance() %s：%s\n", loader.instance() ? "非空" : "为空", qPrintable(loader.errorString()));
    }

    std::printf("\n==== 5. 运行中替换插件 ====\n");
    {
        const QString live = QCoreApplication::applicationDirPath() + "/plugins/live";
        QDir().mkpath(live);
        const QString target = live + "/libinvert.so";
        QFile::remove(target);
        QFile::copy(pluginDir("v1") + "/libinvert.so", target);
        // [region hot-swap]
        QPluginLoader loader(target);
        std::printf("  第一次加载：%s\n", qPrintable(qobject_cast<Operator *>(loader.instance())->name()));
        const bool unloaded = loader.unload();                // 删除插件对象，dlclose
        auto mapped = [](const QString &path) {               // 看进程的内存映射里还有没有这个文件
            QFile maps("/proc/self/maps");
            maps.open(QIODevice::ReadOnly);
            return maps.readAll().contains(path.toUtf8());
        };
        const bool stillMapped = mapped(target);
        QFile::remove(target);                                // 先删再拷：新文件是新的 inode
        QFile::copy(pluginDir("v2") + "/libinvert.so", target);
        QPluginLoader again(target);
        std::printf("  unload=%d，换成 v2 后重新加载：%s\n", unloaded, qPrintable(qobject_cast<Operator *>(again.instance())->name()));
        // [endregion]
        std::printf("  unload 之后，/proc/self/maps 里还有这个 .so：%s\n", stillMapped ? "是" : "否");
        // [region new-name]
        const QString renamed = live + "/libinvert-2.so";     // 换一个文件名
        QFile::remove(renamed);
        QFile::copy(pluginDir("v2") + "/libinvert.so", renamed);
        QPluginLoader byNewName(renamed);
        // [endregion]
        std::printf("  v2 换个文件名加载：%s\n", qPrintable(qobject_cast<Operator *>(byNewName.instance())->name()));
        QPluginLoader same(target);
        std::printf("  同一个文件再建一个 QPluginLoader：instance 与上一个%s\n",
                    same.instance() == again.instance() ? "是同一个对象" : "是不同对象");
    }
    return 0;
}
