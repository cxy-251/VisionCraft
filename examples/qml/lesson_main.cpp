// 纯QML教学窗口共用启动代码；QML从源码读取，方便修改后重新打开。
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);
    QQuickStyle::setStyle("Basic");
    QQmlApplicationEngine engine;
    engine.load(QUrl::fromLocalFile(QStringLiteral(EXAMPLE_DIR "/Main.qml")));
    if(engine.rootObjects().isEmpty()) return 1;
    return app.exec();
}
