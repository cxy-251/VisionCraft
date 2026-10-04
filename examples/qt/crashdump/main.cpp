// 崩溃现场：程序崩溃时自己记下「死在哪」，事后还原成函数名和行号
//
// 运行：./example_qt_crashdump     输出见 output.txt
// 主进程用不同参数启动自己，让子进程去崩溃，再读它留下的记录。

#include <QCoreApplication>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <cstdio>
#include <cstring>
#include <execinfo.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

// [region handler]
static int g_crashFd = -1;                                    // 提前打开好：崩溃时不能再分配内存、打开文件

static void writeStr(const char *s) { ssize_t r = ::write(g_crashFd, s, strlen(s)); (void)r; }

static void onCrash(int sig, siginfo_t *info, void *)
{
    // 这里只能调用「异步信号安全」的函数：write、backtrace_symbols_fd 可以，printf、new、QString 不行
    writeStr("signal ");
    writeStr(sig == SIGSEGV ? "SIGSEGV" : sig == SIGABRT ? "SIGABRT" : sig == SIGFPE ? "SIGFPE" : "?");
    char addr[32];
    const unsigned long a = reinterpret_cast<unsigned long>(info->si_addr);
    int n = 0;
    for (int shift = 60; shift >= 0; shift -= 4) addr[n++] = "0123456789abcdef"[(a >> shift) & 0xf];
    addr[n] = 0;
    writeStr(" addr 0x");
    writeStr(addr);
    writeStr("\n");
    void *frames[32];
    const int count = backtrace(frames, 32);
    backtrace_symbols_fd(frames, count, g_crashFd);           // 直接写进文件，不经过 malloc
    signal(sig, SIG_DFL);                                     // 恢复默认处理，再发一次：照常生成 core、照常以信号退出
    raise(sig);
}

static void installCrashHandler(const char *path, bool altStack)
{
    g_crashFd = ::open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (altStack) {                                           // 给信号处理函数一块单独的栈
        static char stack[64 * 1024];
        stack_t ss{};
        ss.ss_sp = stack;
        ss.ss_size = sizeof stack;
        sigaltstack(&ss, nullptr);
    }
    struct sigaction sa{};
    sa.sa_sigaction = onCrash;
    sa.sa_flags = SA_SIGINFO | (altStack ? SA_ONSTACK : 0);
    for (int sig : {SIGSEGV, SIGABRT, SIGFPE})
        sigaction(sig, &sa, nullptr);
}
// [endregion]

// [region bugs]
struct Part { int id; double scratch; };

__attribute__((noinline)) double measureScratch(const Part *p) { return p->scratch * 2; }   // p 是空指针时崩溃
__attribute__((noinline)) double inspectPart(const Part *p) { return measureScratch(p) + 1; }
__attribute__((noinline)) int recurse(int depth) { volatile char pad[1024]; pad[0] = char(depth); return recurse(depth + 1) + pad[0]; }
// [endregion]

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    const QString dir = QCoreApplication::applicationDirPath();
    if (argc == 4 && QByteArray(argv[1]) == "crash") {
        installCrashHandler(argv[3], QByteArray(argv[2]) != "overflow-noalt");
        const QByteArray what(argv[2]);
        if (what == "null") {
            std::printf("%f\n", inspectPart(nullptr));
        } else if (what.startsWith("overflow")) {
            std::printf("%d\n", recurse(0));
        } else if (what == "abort") {
            std::abort();
        }
        return 0;
    }

    for (const char *what : {"null", "abort", "overflow", "overflow-noalt"}) {
        std::printf("==== 子进程：%s ====\n", what);
        const QString log = dir + "/crash-" + what + ".txt";
        QFile::remove(log);
        // [region run]
        QProcess p;
        p.start(QCoreApplication::applicationFilePath(), {"crash", what, log});
        const qint64 pid = p.processId();
        p.waitForFinished();
        // [endregion]
        std::printf("  exitStatus=%s，exitCode=%d，子进程 pid=%lld（十六进制 %llx）\n",
                    p.exitStatus() == QProcess::CrashExit ? "CrashExit" : "NormalExit", p.exitCode(), pid, pid);
        QFile f(log);
        f.open(QIODevice::ReadOnly);
        const QList<QByteArray> lines = f.readAll().split('\n');
        if (lines.value(0).isEmpty()) {
            std::printf("  崩溃记录：空（处理函数没有运行）\n\n");
            continue;
        }
        std::printf("  崩溃记录第一行：%s\n", lines.value(0).constData());
        // [region symbolize]
        // backtrace 记下的是运行时的绝对地址。程序每次加载的基址不同（地址随机化），
        // 用「(+偏移) [绝对地址]」这一行算出基址，再把每一帧换成文件内偏移，交给 addr2line 查函数名和行号
        static const QRegularExpression frameRe(R"(example_qt_crashdump\(([^)]*)\) \[0x([0-9a-f]+)\])");
        quint64 base = 0;
        QList<QPair<quint64, bool>> frames;                   // (绝对地址, 是否正好是出错的那条指令)
        bool prevWasLibc = false;
        for (const QByteArray &l : lines.mid(1)) {
            const auto m = frameRe.match(QString::fromUtf8(l));
            if (!m.hasMatch()) { prevWasLibc = l.contains("libc.so"); continue; }
            const quint64 abs = m.captured(2).toULongLong(nullptr, 16);
            if (!base && m.captured(1).startsWith("+0x"))
                base = abs - m.captured(1).mid(1).toULongLong(nullptr, 16);
            // 紧跟在 libc 信号跳板后面的那一帧，是出错的指令本身；其余帧都是返回地址
            frames << qMakePair(abs, prevWasLibc && frames.size() == 1);
            prevWasLibc = false;
        }
        int shown = 0;
        for (const auto &[abs, faultPc] : frames.mid(1)) {    // 第 0 帧是 onCrash 自己，跳过
            QProcess a2l;                                     // 返回地址指向 call 的下一条指令，减 1 才落在调用那一行
            a2l.start("addr2line", {"-f", "-C", "-e", QCoreApplication::applicationFilePath(),
                                    QString::number(abs - base - (faultPc ? 0 : 1), 16).prepend("0x")});
            a2l.waitForFinished();
            const QStringList out = QString::fromUtf8(a2l.readAllStandardOutput()).split('\n');
            std::printf("    %-28s %s\n", qPrintable(out.value(0)), qPrintable(out.value(1).section('/', -2)));
            if (++shown == 5) break;
        }
        // [endregion]
        std::printf("  （记录共 %lld 行）\n\n", qlonglong(lines.size() - 1));
    }
    return 0;
}
