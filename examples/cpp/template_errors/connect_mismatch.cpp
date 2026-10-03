// 槽的参数类型和信号对不上：Qt 在编译期用 static_assert 拦下来
#include <QObject>
#include <QString>

class Sensor : public QObject {
    Q_OBJECT
signals:
    void measured(int value);
};

void wire(Sensor *sensor, QObject *context)
{
    QObject::connect(sensor, &Sensor::measured, context, [](const QString &text) { (void)text; });
}
