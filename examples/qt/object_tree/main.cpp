// QObject 对象树：查找、自省、线程归属
//
// 运行：./example_qt_object_tree     输出见 output.txt

#include <QCoreApplication>
#include <QMetaProperty>
#include <QThread>
#include <QTimer>
#include <cstdio>

// [region class]
class Sensor : public QObject {
    Q_OBJECT
    Q_PROPERTY(double temperature READ temperature WRITE setTemperature NOTIFY temperatureChanged)
public:
    using QObject::QObject;
    double temperature() const { return m_t; }
    void setTemperature(double t)
    {
        if (t == m_t) return;
        m_t = t;
        emit temperatureChanged();
    }
    Q_INVOKABLE void calibrate() { std::printf("  calibrate() 被按名字调用了\n"); }
signals:
    void temperatureChanged();

private:
    double m_t = 25.0;
};
// [endregion]

static void tree()
{
    std::printf("==== 1. 按名字查找子对象 ====\n");
    // [region find]
    QObject station;
    station.setObjectName("station");
    auto *board = new QObject(&station);
    board->setObjectName("board");
    auto *temp = new Sensor(board);
    temp->setObjectName("temp");
    new Sensor(board);                                        // 没起名字

    std::printf("  station 的直接子对象：%lld 个\n", qlonglong(station.children().size()));
    Sensor *found = station.findChild<Sensor *>("temp");     // 默认递归查找整棵树
    std::printf("  findChild<Sensor*>(\"temp\")：%s\n", found == temp ? "找到了" : "没找到");
    std::printf("  只找直接子对象：%s\n",
                station.findChild<Sensor *>("temp", Qt::FindDirectChildrenOnly) ? "找到了" : "没找到");
    std::printf("  树里的 Sensor 一共：%lld 个\n", qlonglong(station.findChildren<Sensor *>().size()));
    // [endregion]
}

static void introspect()
{
    std::printf("\n==== 2. moc 生成的元信息：运行时按名字访问 ====\n");
    // [region meta]
    Sensor s;
    const QMetaObject *mo = s.metaObject();
    std::printf("  类名：%s，父类：%s\n", mo->className(), mo->superClass()->className());
    for (int i = mo->propertyOffset(); i < mo->propertyCount(); ++i)
        std::printf("  属性：%s（类型 %s）\n", mo->property(i).name(), mo->property(i).typeName());

    s.setProperty("temperature", 36.5);                       // 按名字写属性：走 WRITE 函数
    std::printf("  setProperty 之后 temperature() = %.1f\n", s.temperature());
    QMetaObject::invokeMethod(&s, "calibrate");               // 按名字调用函数
    std::printf("  inherits(\"QObject\") = %s\n", s.inherits("QObject") ? "true" : "false");
    // [endregion]
}

static void threads()
{
    std::printf("\n==== 3. 父子对象必须在同一个线程 ====\n");
    // [region thread]
    QThread worker;
    worker.start();
    QObject parent;                                           // 属于主线程
    auto *child = new QObject;
    child->moveToThread(&worker);                             // 让它属于工作线程
    child->setParent(&parent);                                // 不允许：Qt 打印警告，什么也不做
    std::printf("  setParent 之后 child->parent() %s\n", child->parent() ? "已设置" : "仍为空");
    worker.quit();
    worker.wait();
    delete child;
    // [endregion]
}

int main(int argc, char *argv[])
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);              // 不缓冲：和 Qt 的警告按真实顺序交错
    qputenv("QT_FORCE_STDERR_LOGGING", "1");                 // 让 Qt 的警告打印到终端
    qSetMessagePattern(QStringLiteral("  [Qt] %{message}"));
    QCoreApplication app(argc, argv);
    tree();
    introspect();
    threads();
    std::fflush(stdout);
    return 0;
}

#include "main.moc"
