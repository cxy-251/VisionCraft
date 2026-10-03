// lambda 与捕获：lambda 什么时候「拍下」外面的变量，以及延后执行时那些变量还在不在
//
// 运行：./example_cpp_lambda_capture                  安全的写法，输出见 output.txt
//       ./example_cpp_lambda_capture dangling-ref     按引用捕获了一个已经销毁的参数（错误写法）
//       ./example_cpp_lambda_capture no-context       connect 没有传上下文对象，接收者删了还被调用（错误写法）
// 两种错误写法是未定义行为，用 AddressSanitizer 编译后运行的结果见 asan-*.txt。

#include <QCoreApplication>
#include <QObject>
#include <QTimer>
#include <cstdio>
#include <cstring>
#include <functional>
#include <vector>

// 模拟 DeviceLink 的「请求—应答」：先登记回调，应答到达（事件循环的下一轮）时才调用
static std::vector<std::function<void()>> g_pending;
static void request(std::function<void()> onReply) { g_pending.push_back(std::move(onReply)); }
static void deliverReplies()
{
    for (auto &f : g_pending)
        f();
    g_pending.clear();
}

static void basics()
{
    std::printf("==== 1. 按值捕获 vs 按引用捕获 ====\n");
    // [region basics]
    int threshold = 18;
    auto byValue = [threshold] { return threshold; };      // 创建时拷贝一份
    auto byRef = [&threshold] { return threshold; };       // 只记住「去哪里找」
    threshold = 40;
    std::printf("  改成 40 之后：按值 = %d，按引用 = %d\n", byValue(), byRef());
    // [endregion]

    // [region mutable]
    int calls = 0;
    auto counter = [calls]() mutable { return ++calls; };  // 改的是 lambda 自己那份拷贝
    counter();
    counter();
    std::printf("  第三次调用返回 %d，外面的 calls 仍是 %d\n", counter(), calls);
    // [endregion]
}

// [region good]
// 正确：label 按值捕获，回调里用的是 lambda 自己的拷贝
static void simpleCommandGood(const QString &label)
{
    request([label] { std::printf("  应答到达：%s 完成\n", qPrintable(label)); });
}
// [endregion]

// [region bad]
// 错误：按引用捕获参数。调用者传进来的是临时对象，simpleCommandBad 返回时它就销毁了
static void simpleCommandBad(const QString &label)
{
    request([&label] { std::printf("  应答到达：%s 完成\n", qPrintable(label)); });
}
// [endregion]

static void deferred()
{
    std::printf("\n==== 2. 回调延后执行：捕获的东西还在吗 ====\n");
    // [region deferred]
    simpleCommandGood(QStringLiteral("PING"));   // 传入的临时 QString 在这一行结束时销毁
    simpleCommandGood(QStringLiteral("BEEP"));
    deliverReplies();                            // 之后才「收到应答」，调用回调
    // [endregion]
}

// [region receiver]
class Panel : public QObject {
public:
    explicit Panel(const char *name) : m_name(name) {}
    void show(int v) { std::printf("  %s 显示 %d\n", m_name.constData(), v); }

private:
    QByteArray m_name;
};

class Sensor : public QObject {
    Q_OBJECT
signals:
    void measured(int value);
};
// [endregion]

static void contextObject()
{
    std::printf("\n==== 3. connect 的上下文对象：接收者销毁后自动断开 ====\n");
    // [region context]
    Sensor sensor;
    auto *panel = new Panel("面板");
    QObject::connect(&sensor, &Sensor::measured, panel, [panel](int v) { panel->show(v); });
    //                                           ^^^^^ 第三个参数：上下文对象
    emit sensor.measured(1);
    delete panel;
    emit sensor.measured(2);                      // 连接已随 panel 一起断开，什么都不发生
    std::printf("  删除面板后再发一次信号：没有调用\n");
    // [endregion]
}

static void noContext()
{
    // [region no-context]
    Sensor sensor;
    auto *panel = new Panel("面板");
    QObject::connect(&sensor, &Sensor::measured, [panel](int v) { panel->show(v); });   // 没有上下文对象
    emit sensor.measured(1);
    delete panel;
    emit sensor.measured(2);                      // 连接还在，lambda 拿着悬空的 panel 指针
    // [endregion]
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    if (argc > 1 && std::strcmp(argv[1], "dangling-ref") == 0) {
        simpleCommandBad(QStringLiteral("PING"));
        simpleCommandBad(QStringLiteral("BEEP"));
        deliverReplies();
        return 0;
    }
    if (argc > 1 && std::strcmp(argv[1], "no-context") == 0) {
        noContext();
        return 0;
    }
    basics();
    deferred();
    contextObject();
    return 0;
}

#include "main.moc"
