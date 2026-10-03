// QML 属性绑定：值什么时候自动更新，什么时候不再更新
//
// 运行：./example_qml_bindings      输出见 output.txt
// 用 C++ 加载 Bindings.qml，从外面改属性、调函数，打印每一步之后的值。不需要窗口。

#include <QCoreApplication>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQmlEngine>
#include <cstdio>

static void show(QObject *o, const char *step)
{
    std::printf("parentWidth=%-4d childWidth=%-4d label=%-12s ← %s\n", o->property("parentWidth").toInt(),
                o->property("childWidth").toInt(), qPrintable(o->property("label").toString()), step);
    std::fflush(stdout);
}

int main(int argc, char *argv[])
{
    // 这台机器上 Qt 默认把日志发给 systemd 日志（journald），不在终端显示；强制输出到终端
    qputenv("QT_FORCE_STDERR_LOGGING", "1");
    QCoreApplication app(argc, argv);
    // 打开 Qt 的一个调试开关：绑定被赋值覆盖时打印一条提示
    QLoggingCategory::setFilterRules(QStringLiteral("qt.qml.binding.removal.info=true"));
    qSetMessagePattern(QStringLiteral("  [Qt] %{message}"));

    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(EXAMPLE_DIR "/Bindings.qml")));
    QObject *o = component.create();
    if (!o) {
        std::printf("%s\n", qPrintable(component.errorString()));
        return 1;
    }

    // [region drive]
    show(o, "加载完成");
    o->setProperty("parentWidth", 600);
    show(o, "parentWidth = 600");
    QMetaObject::invokeMethod(o, "setFixed");
    show(o, "调用 setFixed()（childWidth = 100）");
    o->setProperty("parentWidth", 800);
    show(o, "parentWidth = 800");
    QMetaObject::invokeMethod(o, "restore");
    show(o, "调用 restore()（Qt.binding）");
    o->setProperty("parentWidth", 1000);
    show(o, "parentWidth = 1000");
    // [endregion]

    std::printf("\n给 readonly 属性赋值：\n");
    std::fflush(stdout);
    QMetaObject::invokeMethod(o, "tryWrite");
    std::fflush(stderr);
    std::printf("doubled=%d\n", o->property("doubled").toInt());

    std::printf("\n加载 Loop.qml：\n");
    std::fflush(stdout);
    QQmlComponent loop(&engine, QUrl::fromLocalFile(QStringLiteral(EXAMPLE_DIR "/Loop.qml")));
    QObject *l = loop.create();
    std::fflush(stderr);
    std::printf("a=%d b=%d\n", l->property("a").toInt(), l->property("b").toInt());
    delete l;
    delete o;
    return 0;
}
