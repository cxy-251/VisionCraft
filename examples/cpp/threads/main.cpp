// 线程基础：多个线程同时改一个计数器，三种写法的结果和代价
//
// 运行：./example_cpp_threads      输出见 output.txt
// 只用标准库，方便用 ThreadSanitizer（-fsanitize=thread）检查，见 tsan-race.txt。

#include <atomic>
#include <chrono>
#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>

constexpr int kThreads = 4;
constexpr int kPerThread = 1'000'000;

// 开 kThreads 个线程，每个都执行 body，等它们全部结束，返回耗时（毫秒）
template <typename F>
static double runThreads(F body)
{
    const auto t0 = std::chrono::steady_clock::now();
    // [region spawn]
    std::vector<std::thread> workers;
    for (int i = 0; i < kThreads; ++i)
        workers.emplace_back(body);        // 创建线程，马上开始执行 body
    for (std::thread &t : workers)
        t.join();                          // 等这个线程执行完；不 join 就销毁 std::thread 会直接终止程序
    // [endregion]
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

// [region race]
volatile int g_plain = 0;                  // volatile 只保证每次都真的读写内存，不保证「读-加-写」不被打断

static void addPlain()
{
    for (int i = 0; i < kPerThread; ++i)
        g_plain = g_plain + 1;             // 三步：读出来、加 1、写回去。两个线程可能读到同一个旧值
}
// [endregion]

// [region atomic]
std::atomic<int> g_atomic{0};

static void addAtomic()
{
    for (int i = 0; i < kPerThread; ++i)
        g_atomic.fetch_add(1);             // 一条不可分割的「读-加-写」，CPU 保证别的线程插不进来
}
// [endregion]

// [region mutex]
int g_locked = 0;
std::mutex g_mutex;

static void addLocked()
{
    for (int i = 0; i < kPerThread; ++i) {
        std::lock_guard<std::mutex> lock(g_mutex);   // 构造时加锁，离开这个 {} 时自动解锁
        ++g_locked;                                  // 同一时刻只有一个线程能执行到这里
    }
}
// [endregion]

// [region deadlock]
// 两个线程各自需要两把锁，但拿锁的顺序相反
std::timed_mutex g_a, g_b;

static void lockAB(const char *who)
{
    std::lock_guard<std::timed_mutex> first(g_a);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));   // 让另一个线程有时间拿走 b
    if (g_b.try_lock_for(std::chrono::seconds(1))) {              // 真实代码里是 lock()，会永远等下去
        std::printf("  %s 拿到了两把锁\n", who);
        g_b.unlock();
    } else {
        std::printf("  %s 拿着 a 等 b，等了 1 秒还没等到：死锁\n", who);
    }
}

static void lockBA(const char *who)
{
    std::lock_guard<std::timed_mutex> first(g_b);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (g_a.try_lock_for(std::chrono::seconds(1))) {
        std::printf("  %s 拿到了两把锁\n", who);
        g_a.unlock();
    } else {
        std::printf("  %s 拿着 b 等 a，等了 1 秒还没等到：死锁\n", who);
    }
}

static void lockBoth(const char *who)
{
    std::scoped_lock both(g_a, g_b);   // 一次要两把：内部用「拿不到就全部放下重来」的算法，不会死锁
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::printf("  %s 拿到了两把锁\n", who);
}
// [endregion]

int main()
{
    const int expected = kThreads * kPerThread;
    std::printf("%d 个线程，每个加 %d 次，正确结果是 %d\n\n", kThreads, kPerThread, expected);

    const double plainMs = runThreads(addPlain);
    std::printf("volatile int ：结果 %8d，丢了 %8d 次，%6.1f ms\n", g_plain, expected - g_plain, plainMs);
    const double atomicMs = runThreads(addAtomic);
    std::printf("std::atomic  ：结果 %8d，丢了 %8d 次，%6.1f ms\n", g_atomic.load(), expected - g_atomic.load(), atomicMs);
    const double lockedMs = runThreads(addLocked);
    std::printf("std::mutex   ：结果 %8d，丢了 %8d 次，%6.1f ms\n", g_locked, expected - g_locked, lockedMs);

    std::printf("\n两个线程按相反的顺序拿两把锁：\n");
    {
        std::thread t1(lockAB, "线程 1");
        std::thread t2(lockBA, "线程 2");
        t1.join();
        t2.join();
    }
    std::printf("\n改用 std::scoped_lock 一次拿两把：\n");
    {
        std::thread t1(lockBoth, "线程 1");
        std::thread t2(lockBoth, "线程 2");
        t1.join();
        t2.join();
    }
    return 0;
}
