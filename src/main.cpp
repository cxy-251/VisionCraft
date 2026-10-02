#include <QApplication>
#include <QWidget>
#include "DeviceLink.h"
#include "LegacyLab.h"
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QQuickItem>
#include <QTimer>

// 在可视树里按 objectName 找元素（Loader 加载的内容不一定是 QObject 意义上的子对象）
static QQuickItem *findItem(QQuickItem *item, const QString &name)
{
    if (!item)
        return nullptr;
    if (item->objectName() == name)
        return item;
    for (QQuickItem *child : item->childItems()) {
        if (QQuickItem *found = findItem(child, name))
            return found;
    }
    return nullptr;
}

// 开发辅助：设置 VC_SNAPSHOT=<文件.png> 时，启动后等 VC_SNAPSHOT_DELAY 毫秒（默认 2500）
// 把主窗口截图保存下来并退出，用于在没人看屏幕时检查界面
static void scheduleSnapshot(QQmlApplicationEngine &engine)
{
    const QString file = qEnvironmentVariable("VC_SNAPSHOT");
    if (file.isEmpty())
        return;
    const int delay = qEnvironmentVariableIsSet("VC_SNAPSHOT_DELAY")
                          ? qEnvironmentVariableIntValue("VC_SNAPSHOT_DELAY") : 2500;
    // VC_SNAPSHOT_SIZE=宽x高：截图前把窗口调到这个尺寸，用来一次截下长页面
    const QStringList size = qEnvironmentVariable("VC_SNAPSHOT_SIZE").split(QLatin1Char('x'));
    if (size.size() == 2 && !engine.rootObjects().isEmpty()) {
        if (auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first()))
            window->resize(size[0].toInt(), size[1].toInt());
    }
    // VC_SNAPSHOT_SCROLL=像素：把当前打开的手册正文滚动到这个位置再截图
    const int scroll = qEnvironmentVariableIntValue("VC_SNAPSHOT_SCROLL");
    // VC_SNAPSHOT_LAB=1：截的是「实验室」打开的旧版窗口
    if (qEnvironmentVariableIntValue("VC_SNAPSHOT_LAB") == 1) {
        QTimer::singleShot(delay, &engine, [&engine, file] {
            if (auto *lab = engine.singletonInstance<LegacyLab *>("VisionCraft", "LegacyLab")) {
                lab->open();
                QTimer::singleShot(1500, &engine, [file] {
                    for (QWidget *w : QApplication::topLevelWidgets()) {
                        if (w->isVisible() && w->inherits("QMainWindow"))
                            w->grab().save(file);
                    }
                    QCoreApplication::quit();
                });
            }
        });
        return;
    }
    QTimer::singleShot(delay, &engine, [&engine, file, scroll] {
        if (!engine.rootObjects().isEmpty()) {
            QObject *root = engine.rootObjects().first();
            if (auto *window = qobject_cast<QQuickWindow *>(root)) {
                if (scroll > 0) {
                    if (QQuickItem *section = findItem(window->contentItem(), QStringLiteral("handbookSection")))
                        section->setProperty("contentY", scroll);
                }
                QTimer::singleShot(300, window, [window, file] {   // 等滚动后的一帧画完
                    window->grabWindow().save(file);
                    QCoreApplication::quit();
                });
                return;
            }
        }
        QCoreApplication::quit();
    });
}

int main(int argc, char *argv[])
{
    // QApplication 而不是 QGuiApplication：「实验室」里的旧版界面是 Widgets 窗口
    QApplication app(argc, argv);
    app.setApplicationName("VisionCraft");
    app.setOrganizationName("VisionCraftStudio");

    // 控件外观全部自绘，Basic 风格最容易定制，且各平台一致
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    // 开发辅助：VC_PAGE=0..4 指定启动页；VC_AUTOCONNECT=sim|rtt 启动后自动连接设备
    if (qEnvironmentVariableIsSet("VC_PAGE"))
        engine.setInitialProperties({{QStringLiteral("startPage"), qEnvironmentVariableIntValue("VC_PAGE")}});
    engine.loadFromModule("VisionCraft", "Main");
    if (const QString mode = qEnvironmentVariable("VC_AUTOCONNECT"); !mode.isEmpty()) {
        if (auto *link = engine.singletonInstance<DeviceLink *>("VisionCraft", "DeviceLink")) {
            if (mode == QLatin1String("rtt"))
                link->connectRtt();
            else
                link->connectSimulator();
            QObject::connect(link, &DeviceLink::stateChanged, link, [link] {
                if (link->state() == DeviceLink::Connected)
                    link->subscribeTelemetry(300);
            });
        }
    }
    scheduleSnapshot(engine);

    return app.exec();
}
