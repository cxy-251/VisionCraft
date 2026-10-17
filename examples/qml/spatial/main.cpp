#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
int main(int argc, char **argv) {
    QGuiApplication app(argc,argv);
    QQuickStyle::setStyle("Basic");
    QQmlApplicationEngine engine;
    engine.loadFromModule("SpatialLessons","Main");
    return engine.rootObjects().isEmpty() ? 1 : app.exec();
}
