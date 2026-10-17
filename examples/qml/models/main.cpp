#include "LessonModel.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);
    QQuickStyle::setStyle("Basic");
    // 小型独立示例显式注册；主应用采用qt_add_qml_module和QML_ELEMENT自动注册。
    qmlRegisterType<LessonModel>("Lesson",1,0,"LessonModel");
    QQmlApplicationEngine engine;
    engine.load(QUrl::fromLocalFile(QStringLiteral(EXAMPLE_DIR "/Main.qml")));
    if(engine.rootObjects().isEmpty())return 1;
    return app.exec();
}
