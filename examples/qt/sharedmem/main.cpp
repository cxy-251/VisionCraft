// 共享内存：两个进程看同一块内存
//
// 运行：./example_qt_sharedmem     输出见 output.txt
// 程序会用不同的参数把自己再启动几次，扮演「另一个进程」。

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QProcess>
#include <QSharedMemory>
#include <cstdio>
#include <cstring>

static const char *kKey = "visioncraft-example-frame";
static constexpr int kW = 1280, kH = 1024;                    // 一帧 8 位灰度图：1.25 MB

// [region header]
struct FrameHeader {                                          // 共享内存开头放一个头，后面是像素
    quint32 frameNo;
    quint32 width, height;
    quint64 counter;                                          // 第 3 步用来演示加锁
};
// [endregion]

static QSharedMemory *attach(QSharedMemory::AccessMode mode = QSharedMemory::ReadWrite)
{
    auto *shm = new QSharedMemory(QSharedMemory::legacyNativeKey(kKey));
    if (!shm->attach(mode)) {
        std::fprintf(stderr, "attach 失败：%s\n", qPrintable(shm->errorString()));
        std::exit(1);
    }
    return shm;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    const QByteArray role = argc > 1 ? argv[1] : "";

    // ---------- 子进程的几种角色 ----------
    if (role == "reader") {
        // [region reader]
        QSharedMemory *shm = attach(QSharedMemory::ReadOnly);
        shm->lock();
        const auto *h = static_cast<const FrameHeader *>(shm->constData());
        const auto *px = reinterpret_cast<const uchar *>(h + 1);
        quint64 sum = 0;
        for (quint32 i = 0; i < h->width * h->height; ++i) sum += px[i];
        std::printf("  [子进程] 第 %u 帧 %ux%u，像素和 %llu\n", h->frameNo, h->width, h->height, (unsigned long long)sum);
        shm->unlock();
        // [endregion]
        return 0;
    }
    if (role == "add-nolock" || role == "add-lock") {
        QSharedMemory *shm = attach();
        auto *h = static_cast<FrameHeader *>(shm->data());
        // [region counter]
        volatile quint64 &counter = h->counter;               // volatile：不让编译器把 100 万次加法合并成一次
        for (int i = 0; i < 1000000; ++i) {
            if (role == "add-lock") shm->lock();
            counter = counter + 1;                            // 读、加一、写回：三步，不是一步
            if (role == "add-lock") shm->unlock();
        }
        // [endregion]
        return 0;
    }
    if (role == "crash-owner") {
        QSharedMemory shm(QSharedMemory::legacyNativeKey("visioncraft-example-crash"));
        shm.create(4096);
        std::abort();                                         // 创建者崩溃，没有机会释放
    }

    // ---------- 主进程 ----------
    std::printf("==== 1. 创建并写入一帧 ====\n");
    // [region create]
    QSharedMemory shm(QSharedMemory::legacyNativeKey(kKey));
    if (!shm.create(sizeof(FrameHeader) + kW * kH)) {         // 创建者决定大小
        std::printf("  create 失败：%s\n", qPrintable(shm.errorString()));
        return 1;
    }
    shm.lock();
    auto *h = static_cast<FrameHeader *>(shm.data());
    *h = {1, kW, kH, 0};
    auto *px = reinterpret_cast<uchar *>(h + 1);
    for (int i = 0; i < kW * kH; ++i) px[i] = uchar(i % 251);
    shm.unlock();
    // [endregion]
    quint64 sum = 0;
    for (int i = 0; i < kW * kH; ++i) sum += px[i];
    std::printf("  共享内存 %lld 字节，主进程算的像素和 %llu\n", qlonglong(shm.size()), (unsigned long long)sum);
    std::fflush(stdout);

    std::printf("\n==== 2. 另一个进程读 ====\n");
    std::fflush(stdout);
    QProcess::execute(QCoreApplication::applicationFilePath(), {"reader"});
    std::printf("\n==== 3. 两个进程同时改一个数 ====\n");
    for (const char *mode : {"add-nolock", "add-lock"}) {
        h->counter = 0;
        QElapsedTimer t;
        t.start();
        QProcess a, b;
        a.start(QCoreApplication::applicationFilePath(), {mode});
        b.start(QCoreApplication::applicationFilePath(), {mode});
        a.waitForFinished(-1);
        b.waitForFinished(-1);
        std::printf("  %-10s 两个进程各加 100 万次：结果 %llu（应为 2000000），%.0f ms\n", mode,
                    (unsigned long long)h->counter, t.nsecsElapsed() / 1e6);
    }

    std::printf("\n==== 4. 创建者崩溃之后 ====\n");
    for (bool attachFirst : {false, true}) {
        QProcess::execute(QCoreApplication::applicationFilePath(), {"crash-owner"});
        QSharedMemory again(QSharedMemory::legacyNativeKey("visioncraft-example-crash"));
        if (!attachFirst) {
            const bool created = again.create(4096);
            std::printf("  直接 create：%s；", created ? "成功" : qPrintable(again.errorString()));
            const bool attached = again.attach();
            std::printf("再 attach：%s\n", attached ? "成功" : qPrintable(again.errorString()));
        } else {
            // [region recover]
            if (again.attach())                               // 先试着连上残留的那块
                again.detach();                               // 最后一个 detach 的进程负责把它删掉
            const bool created = again.create(4096);
            // [endregion]
            std::printf("  先 attach 再 detach，然后 create：%s\n", created ? "成功" : qPrintable(again.errorString()));
        }
    }
    return 0;
}
