// 拷贝、移动与隐式共享：把一个对象「给」出去时，数据到底有没有被复制
//
// 运行：./example_cpp_copy_move     输出见 output.txt
// 判断有没有复制的办法：比较数据所在的地址。地址相同 = 共用同一块内存。

#include <QByteArray>
#include <QElapsedTimer>
#include <QList>
#include <cstdio>
#include <utility>
#include <vector>

static const char *same(const void *a, const void *b) { return a == b ? "同一块内存" : "不同的内存"; }

static void stdCopy()
{
    std::printf("==== 1. std::vector：拷贝就是复制全部数据 ====\n");
    // [region std-copy]
    std::vector<char> a(8 * 1024 * 1024, 'a');   // 8 MB
    std::vector<char> b = a;                     // 拷贝构造：分配新内存，逐字节复制
    std::printf("  b = a 之后：%s\n", same(a.data(), b.data()));
    // [endregion]
}

static void move()
{
    std::printf("\n==== 2. std::move：把数据「交出去」，不复制 ====\n");
    // [region move]
    std::vector<char> a(8 * 1024 * 1024, 'a');
    const char *before = a.data();
    std::vector<char> b = std::move(a);          // 移动构造：b 接管 a 的内存，a 变空
    std::printf("  b 用的是 a 原来的内存吗？%s\n", b.data() == before ? "是" : "不是");
    std::printf("  a.size() = %zu（被移走后是有效但未指定的状态，标准库容器实际是空的）\n", a.size());

    const std::vector<char> c(1024, 'c');
    std::vector<char> d = std::move(c);          // c 是 const，不能被修改，于是悄悄退化成拷贝
    std::printf("  std::move 一个 const 对象：%s\n", same(c.data(), d.data()));
    // [endregion]
}

static void sharing()
{
    std::printf("\n==== 3. QByteArray：拷贝时共享，写入时才复制 ====\n");
    // [region sharing]
    QByteArray a(8 * 1024 * 1024, 'a');
    QByteArray b = a;                            // 只把引用计数 +1
    std::printf("  b = a 之后：%s\n", same(a.constData(), b.constData()));
    b[0] = 'x';                                  // 第一次写：b 发现数据被别人共用，先复制一份再写（detach）
    std::printf("  b[0] = 'x' 之后：%s，a[0] 仍是 '%c'\n", same(a.constData(), b.constData()), a[0]);
    // [endregion]
}

static void timing()
{
    std::printf("\n==== 4. 代价：拷贝 1000 次 8 MB ====\n");
    // [region timing]
    const std::vector<char> v(8 * 1024 * 1024, 'a');
    const QByteArray q(8 * 1024 * 1024, 'a');
    QElapsedTimer t;
    long touched = 0;                            // 用一下拷贝，防止整个循环被编译器优化掉

    t.start();
    for (int i = 0; i < 1000; ++i) {
        std::vector<char> copy = v;
        touched += copy[0];
    }
    const double stdMs = t.nsecsElapsed() / 1e6;

    t.restart();
    for (int i = 0; i < 1000; ++i) {
        QByteArray copy = q;
        touched += copy.at(0);                   // at() 是 const 函数：只读，不 detach
    }
    const double atMs = t.nsecsElapsed() / 1e6;

    t.restart();
    for (int i = 0; i < 1000; ++i) {
        QByteArray copy = q;
        touched += copy[0];                      // copy 不是 const：调用非 const 的 operator[]，它返回可写的引用，必须先 detach
    }
    const double indexMs = t.nsecsElapsed() / 1e6;

    std::printf("  std::vector 拷贝：            %8.1f ms\n", stdMs);
    std::printf("  QByteArray 拷贝 + at(0)：     %8.3f ms\n", atMs);
    std::printf("  QByteArray 拷贝 + copy[0]：   %8.1f ms\n", indexMs);
    if (touched == 0) std::printf("\n");
    // [endregion]
}

static void rangeFor()
{
    std::printf("\n==== 5. 坑：遍历共享的 Qt 容器也可能触发复制 ====\n");
    // [region range-for]
    QList<int> a(1000000, 1);
    QList<int> b = a;                            // 共享
    long sum = 0;
    for (const int &x : b)                       // b 不是 const：调用的是非 const 的 begin()，先 detach
        sum += x;
    std::printf("  非 const 遍历之后：%s\n", same(a.constData(), b.constData()));

    QList<int> c = a;
    for (const int &x : std::as_const(c))        // 当成 const 遍历：调用 const 的 begin()，不复制
        sum += x;
    std::printf("  std::as_const 遍历之后：%s\n", same(a.constData(), c.constData()));
    // [endregion]
    if (sum == 0) std::printf("\n");
}

int main()
{
    stdCopy();
    move();
    sharing();
    timing();
    rangeFor();
    return 0;
}
