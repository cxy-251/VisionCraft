// 对象生命周期与所有权：谁创建、谁销毁、什么时候销毁
//
// 运行：./example_cpp_object_lifetime                 依次演示安全的写法，输出见 output.txt
//       ./example_cpp_object_lifetime delete-in-slot  在槽里直接 delete 发送者（错误写法）
//       ./example_cpp_object_lifetime stack-order     栈上的子对象先于父对象声明（错误写法）
// 两种错误写法是未定义行为，用 AddressSanitizer 编译后运行才能稳定地看到问题，见 asan-*.txt。

#include <QCoreApplication>
#include <QPointer>
#include <QTimer>
#include <cstdio>
#include <cstring>
#include <memory>

static void say(const char *fmt, const char *name)
{
    std::printf(fmt, name);
    std::printf("\n");
    std::fflush(stdout);
}

// [region tracer]
// 构造和析构时各打印一行，用来观察「什么时候被销毁」
struct Tracer {
    const char *name;
    explicit Tracer(const char *n) : name(n) { say("  构造 %s", name); }
    ~Tracer() { say("  析构 %s", name); }
};
// [endregion]

// [region node]
class Node : public QObject {
public:
    Node(const char *name, QObject *parent = nullptr)
        : QObject(parent), m_name(name), m_label(QByteArray(name) + " 的成员"), m_member(m_label.constData()) {}
    ~Node() override { say("  ~Node 函数体：%s", m_name); }

private:
    const char *m_name;
    QByteArray m_label;
    Tracer m_member;     // 普通成员变量：看它和子对象谁先被销毁
};
// [endregion]

// [region link]
// 模拟一个「传输层」：发出 closed 信号之后，还要用自己的成员变量
class Link : public QObject {
    Q_OBJECT
public:
    explicit Link(QObject *parent = nullptr) : QObject(parent), m_name(QStringLiteral("rtt")) {}
    void closeNow()
    {
        emit closed();
        // emit 返回后继续执行：如果槽里把 this 删掉了，下面这行读的就是已经释放的内存
        std::printf("  Link::closeNow 收尾：name = %s\n", qPrintable(m_name));
    }
signals:
    void closed();

private:
    QString m_name;
};
// [endregion]

static void scopes()
{
    std::printf("==== 1. 作用域：离开 {} 时按声明的反序析构 ====\n");
    // [region scopes]
    {
        Tracer a("a");
        Tracer b("b");
        say("  离开作用域%s", "");
    }
    // [endregion]
}

static void uniquePtr()
{
    std::printf("\n==== 2. unique_ptr：唯一的主人，可以转交 ====\n");
    // [region unique]
    std::unique_ptr<Tracer> owner = std::make_unique<Tracer>("堆上的 t");
    std::unique_ptr<Tracer> next = std::move(owner);       // 所有权转给 next，owner 变空
    std::printf("  转交后 owner %s，next %s\n", owner ? "非空" : "为空", next ? "非空" : "为空");
    next.reset();                                           // 主人放手：立即析构
    std::printf("  reset 之后\n");
    // [endregion]
}

static void parentChild()
{
    std::printf("\n==== 3. QObject 父子树：父对象析构时删除子对象 ====\n");
    // [region tree]
    auto *root = new Node("root");
    new Node("child", root);                    // 只交给父对象，不需要自己保存指针
    QPointer<Node> watch = new Node("grandchild", root->children().first());
    std::printf("  delete root 之前，watch %s\n", watch ? "非空" : "为空");
    delete root;
    std::printf("  delete root 之后，watch %s\n", watch ? "非空" : "为空");
    // [endregion]
}

static void deleteLaterDemo()
{
    std::printf("\n==== 4. 在槽里销毁发送者：用 deleteLater ====\n");
    // [region later]
    auto *link = new Link;
    QPointer<Link> watch = link;
    QObject::connect(link, &Link::closed, [link] {
        std::printf("  槽：收到 closed，deleteLater()\n");
        link->deleteLater();                  // 只是登记：回到事件循环后再删
    });
    link->closeNow();
    std::printf("  closeNow 返回，watch %s\n", watch ? "非空" : "为空");
    QCoreApplication::processEvents();
    std::printf("  processEvents 之后，watch %s\n", watch ? "非空" : "为空");
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    std::printf("  sendPostedEvents(DeferredDelete) 之后，watch %s\n", watch ? "非空" : "为空");
    // [endregion]
}

static void deleteInSlot()
{
    // [region delete-in-slot]
    auto *link = new Link;
    QObject::connect(link, &Link::closed, [link] {
        std::printf("  槽：收到 closed，直接 delete\n");
        delete link;                          // 错误：closeNow 还没执行完
    });
    link->closeNow();
    // [endregion]
}

static void stackOrder()
{
    // [region stack-order]
    QObject child;                            // 先声明，后析构
    QObject parent;
    child.setParent(&parent);
    // 离开作用域：parent 先析构，按父子关系 delete &child —— 而 child 在栈上，不是 new 出来的
    // [endregion]
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    if (argc > 1 && std::strcmp(argv[1], "delete-in-slot") == 0) {
        deleteInSlot();
        return 0;
    }
    if (argc > 1 && std::strcmp(argv[1], "stack-order") == 0) {
        stackOrder();
        return 0;
    }
    scopes();
    uniquePtr();
    parentChild();
    deleteLaterDemo();
    return 0;
}

#include "main.moc"
