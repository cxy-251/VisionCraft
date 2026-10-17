#include <QApplication>
#include <QWidget>
#include <QDir>
#include <QSettings>
#include <QMessageBox>
#include "DeviceLink.h"
#include "StationController.h"
#include "LegacyLab.h"
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QQuickItem>
#include <QTimer>
#include <memory>

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
                    if (auto *section = window->findChild<QQuickItem *>(QStringLiteral("handbookSection")))
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
    app.setApplicationVersion(QStringLiteral(VC_VERSION));   // QML 里用 Qt.application.version 读

    // 单文件便携启动器只解压程序；设置必须保存在启动器旁边的持久目录。
    if (const QString home = qEnvironmentVariable("VC_PORTABLE_HOME"); !home.isEmpty()) {
        if (!QDir().mkpath(home)) {
            QMessageBox::critical(nullptr, QStringLiteral("VisionCraft"),
                                  QStringLiteral("Cannot create portable settings directory: %1").arg(home));
            return 1;
        }
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, home);
    }

    // 控件外观全部自绘，Basic 风格最容易定制，且各平台一致
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    // 开发辅助：VC_PAGE=0..9（前五项保持兼容）；VC_AUTOCONNECT=sim|rtt|serial，串口另设VC_SERIAL_PORT。
    if (qEnvironmentVariableIsSet("VC_PAGE"))
        engine.setInitialProperties({{QStringLiteral("startPage"), qEnvironmentVariableIntValue("VC_PAGE")}});
    engine.loadFromModule("VisionCraft", "Main");
    // VC_STATION_RUN=1：启动后自动开始产线
    if (qEnvironmentVariableIntValue("VC_STATION_RUN") == 1) {
        if (auto *station = engine.singletonInstance<StationController *>("VisionCraft", "Station"))
            QTimer::singleShot(4000, station, [station] { station->setRunning(true); });
    }
    if (const QString mode = qEnvironmentVariable("VC_AUTOCONNECT"); !mode.isEmpty()) {
        if (auto *link = engine.singletonInstance<DeviceLink *>("VisionCraft", "DeviceLink")) {
            // Serial opens synchronously. Register scope/shell observers below
            // before opening any transport so the Connected signal is observed.
            QTimer::singleShot(0, link, [link, mode] {
                if (mode == QLatin1String("rtt"))
                    link->connectRtt();
                else if (mode == QLatin1String("serial"))
                    link->connectSerial(qEnvironmentVariable("VC_SERIAL_PORT"), 115200);
                else
                    link->connectSimulator();
            });
            // VC_SCOPE=1：在数据页截图/验证一帧板上回环，不启用连续采集。
            if (qEnvironmentVariableIntValue("VC_SCOPE") == 1) {
                auto scopeConnection = std::make_shared<QMetaObject::Connection>();
                *scopeConnection = QObject::connect(link, &DeviceLink::stateChanged, link, [link, scopeConnection] {
                    if (link->state() != DeviceLink::Connected) return;
                    QObject::disconnect(*scopeConnection);
                    QTimer::singleShot(250, link, [link] { link->startScope(50000, 2048); });
                });
            }
            // VC_SHELL=help;uptime：连上后依次在板子上执行这几条终端命令（截图用）
            const QStringList cmds = qEnvironmentVariable("VC_SHELL").split(QLatin1Char(';'), Qt::SkipEmptyParts);
            if (!cmds.isEmpty()) {
                auto conn = std::make_shared<QMetaObject::Connection>();
                *conn = QObject::connect(link, &DeviceLink::stateChanged, link, [link, cmds, conn] {
                    if (link->state() != DeviceLink::Connected)
                        return;
                    QObject::disconnect(*conn);
                    for (int i = 0; i < cmds.size(); ++i)   // 一条一条来：板子一次只接一条
                        QTimer::singleShot(500 + 400 * i, link, [link, c = cmds[i]] { link->shell(c); });
                });
            }
        }
    }
    scheduleSnapshot(engine);

    return app.exec();
}
