// 无锁环形队列（一个生产者、一个消费者）和异步日志
//
// 运行：./example_qt_ringbuffer               输出见 output.txt
//       ./example_qt_ringbuffer crash <sync|async> <文件>   写 1000 行日志后立刻 abort()，给第 3 部分用

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QDebug>
#include <QMutex>
#include <QProcess>
#include <QQueue>
#include <QWaitCondition>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>

// [region ring]
// 单生产者、单消费者的环形队列。容量是 2 的幂，下标用「& (N − 1)」代替取模
template <typename T, size_t N>
class SpscRing {
    static_assert((N & (N - 1)) == 0, "容量必须是 2 的幂");
public:
    bool push(const T &v)                                     // 只有生产者线程调用
    {
        const size_t head = m_head.load(std::memory_order_relaxed);
        if (head - m_tail.load(std::memory_order_acquire) == N)
            return false;                                     // 满了
        m_buf[head & (N - 1)] = v;
        m_head.store(head + 1, std::memory_order_release);    // release：保证上一行写的数据先于 head 被对方看到
        return true;
    }
    bool pop(T &v)                                            // 只有消费者线程调用
    {
        const size_t tail = m_tail.load(std::memory_order_relaxed);
        if (m_head.load(std::memory_order_acquire) == tail)   // acquire：看到新 head，就一定看到它之前写好的数据
            return false;                                     // 空的
        v = m_buf[tail & (N - 1)];
        m_tail.store(tail + 1, std::memory_order_release);
        return true;
    }
private:
    T m_buf[N];
    alignas(64) std::atomic<size_t> m_head{0};                // 两个下标放在不同的缓存行，免得两个核心互相抢
    alignas(64) std::atomic<size_t> m_tail{0};
};
// [endregion]

// [region locked]
// 对照：互斥锁 + QQueue
class LockedQueue {
public:
    void push(int v) { QMutexLocker l(&m); q.enqueue(v); }
    bool pop(int &v) { QMutexLocker l(&m); if (q.isEmpty()) return false; v = q.dequeue(); return true; }
private:
    QMutex m;
    QQueue<int> q;
};
// [endregion]

static constexpr int kCount = 5'000'000;
static SpscRing<int, 1024> g_ring;

template <typename Push, typename Pop>
static void transfer(const char *name, Push push, Pop pop)
{
    QElapsedTimer t;
    t.start();
    std::thread producer([&] { for (int i = 0; i < kCount; ++i) while (!push(i)) {} });
    long bad = 0;
    int expect = 0, v;
    while (expect < kCount)
        if (pop(v)) { if (v != expect) ++bad; ++expect; }
    producer.join();
    std::printf("  %-22s %d 个整数，顺序错误 %ld 个，%.0f ms\n", name, kCount, bad, t.nsecsElapsed() / 1e6);
}

// ---------------- 异步日志 ----------------
// [region async-log]
// 调用 log() 的线程只把字符串放进内存缓冲区就返回；后台线程每攒一批、或每 100 ms，统一写盘
class AsyncLog {
public:
    explicit AsyncLog(const char *path) : m_file(std::fopen(path, "w")), m_writer([this] { run(); }) {}
    ~AsyncLog()
    {
        { QMutexLocker l(&m_mutex); m_stop = true; }
        m_cond.wakeOne();
        m_writer.join();                                      // 析构时把剩下的全部写完
        std::fclose(m_file);
    }
    void log(const QByteArray &line)
    {
        QMutexLocker l(&m_mutex);
        m_front.append(line).append('\n');                   // 前台缓冲：只追加，不碰文件
        if (m_front.size() > 64 * 1024)
            m_cond.wakeOne();
    }
private:
    void run()
    {
        QByteArray back;
        for (;;) {
            {
                QMutexLocker l(&m_mutex);
                if (m_front.isEmpty() && !m_stop)
                    m_cond.wait(&m_mutex, 100);
                back.swap(m_front);                           // 双缓冲：交换，前台立刻可以继续写
                if (back.isEmpty() && m_stop) return;
            }
            std::fwrite(back.constData(), 1, size_t(back.size()), m_file);   // 写盘在锁外面
            std::fflush(m_file);
            back.clear();
        }
    }
    std::FILE *m_file;
    QMutex m_mutex;
    QWaitCondition m_cond;
    QByteArray m_front;
    bool m_stop = false;
    std::thread m_writer;
};
// [endregion]

// [region handler]
// 把 qDebug/qInfo/qWarning 全部转进异步日志
static AsyncLog *g_log = nullptr;
static void toAsyncLog(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    static const char *const kLevel[] = {"调试", "警告", "严重", "致命", "信息"};   // Qt 6 里下标就是 QtMsgType 的值（qlogging.h；Qt 7 会把 Info 挪到第 2 个）
    g_log->log(QByteArray("[") + kLevel[type] + "] " + msg.toUtf8());
}
// [endregion]

static void syncLog(std::FILE *f, const QByteArray &line)
{
    std::fwrite(line.constData(), 1, size_t(line.size()), f);
    std::fputc('\n', f);
    std::fflush(f);                                           // 每行都真正交给操作系统
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    if (argc == 4 && QByteArray(argv[1]) == "crash") {
        // [region crash]
        if (QByteArray(argv[2]) == "sync") {
            std::FILE *f = std::fopen(argv[3], "w");
            for (int i = 0; i < 1000; ++i) syncLog(f, "line " + QByteArray::number(i));
        } else {
            auto *log = new AsyncLog(argv[3]);                // 故意不析构：模拟程序崩溃
            for (int i = 0; i < 1000; ++i) log->log("line " + QByteArray::number(i));
        }
        std::abort();                                         // 崩溃：不会执行任何析构函数
        // [endregion]
    }

    std::printf("==== 1. 跨线程传递整数 ====\n");
    transfer("无锁环形队列（1024）", [](int v) { return g_ring.push(v); }, [](int &v) { return g_ring.pop(v); });
    LockedQueue lq;
    transfer("互斥锁 + QQueue", [&](int v) { lq.push(v); return true; }, [&](int &v) { return lq.pop(v); });

    std::printf("\n==== 2. 写日志的调用要多久 ====\n");
    const QString dir = QCoreApplication::applicationDirPath();
    {
        // [region latency]
        const int n = 200000;
        QElapsedTimer t;
        std::FILE *f = std::fopen(qPrintable(dir + "/sync.log"), "w");
        t.start();
        for (int i = 0; i < n; ++i) syncLog(f, "工位 A3 检测完成 #" + QByteArray::number(i));
        const double syncUs = t.nsecsElapsed() / 1e3 / n;
        std::fclose(f);
        double asyncUs;
        {
            AsyncLog log(qPrintable(dir + "/async.log"));
            t.restart();
            for (int i = 0; i < n; ++i) log.log("工位 A3 检测完成 #" + QByteArray::number(i));
            asyncUs = t.nsecsElapsed() / 1e3 / n;
        }   // 析构：等后台写完
        // [endregion]
        std::printf("  同步（每行 fflush）：每次调用 %.2f µs\n", syncUs);
        std::printf("  异步（只进内存缓冲）：每次调用 %.2f µs\n", asyncUs);
        QFile a(dir + "/async.log");
        a.open(QIODevice::ReadOnly);
        std::printf("  异步日志析构后，文件里 %lld 行（应为 %d）\n", qlonglong(a.readAll().count('\n')), n);
        QFile::remove(dir + "/sync.log");
        QFile::remove(dir + "/async.log");
    }

    std::printf("\n==== 3. 程序崩溃时，日志还剩多少 ====\n");
    for (const char *mode : {"sync", "async"}) {
        const QString file = dir + "/crash-" + mode + ".log";
        QProcess::execute(QCoreApplication::applicationFilePath(), {"crash", mode, file});
        QFile f(file);
        f.open(QIODevice::ReadOnly);
        std::printf("  %-5s：写了 1000 行后崩溃，文件里留下 %lld 行\n", mode, qlonglong(f.readAll().count('\n')));
        f.remove();
    }

    std::printf("\n==== 4. qInstallMessageHandler ====\n");
    {
        const QString file = dir + "/qt.log";
        // [region install]
        g_log = new AsyncLog(qPrintable(file));
        const QtMessageHandler old = qInstallMessageHandler(toAsyncLog);   // 返回原来的处理函数
        qInfo() << "相机已连接" << 1920 << "x" << 1080;
        qWarning("曝光 %d µs 超出范围", 90000);
        qDebug() << "这一行也进日志文件";
        qInstallMessageHandler(old);                          // 先恢复，再销毁日志对象
        delete g_log;
        g_log = nullptr;
        // [endregion]
        QFile f(file);
        f.open(QIODevice::ReadOnly);
        std::printf("%s", f.readAll().constData());
        f.remove();
    }
    return 0;
}
