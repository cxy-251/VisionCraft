// 内存顺序写错时，ThreadSanitizer 能不能发现
// 不依赖 Qt，单独编译：
//   g++ -std=c++17 -O1 -g -fsanitize=thread -DORDER_OK=1 ring_order.cpp -o ok  && ./ok
//   g++ -std=c++17 -O1 -g -fsanitize=thread -DORDER_OK=0 ring_order.cpp -o bad && ./bad
//   （本机用 scripts/steamdeck-env.sh 里的 gcc 时，加 --sysroot=$VC_SYSROOT）
#include <atomic>
#include <cstdio>
#include <thread>

#if ORDER_OK
constexpr auto kPublish = std::memory_order_release, kObserve = std::memory_order_acquire;
#else
constexpr auto kPublish = std::memory_order_relaxed, kObserve = std::memory_order_relaxed;   // 错误写法
#endif

constexpr size_t N = 64;
int buf[N];
std::atomic<size_t> head{0}, tail{0};

int main()
{
    const int count = 100000;
    std::thread producer([&] {
        for (int i = 0; i < count; ++i) {
            const size_t h = head.load(std::memory_order_relaxed);
            while (h - tail.load(kObserve) == N) {}
            buf[h % N] = i;
            head.store(h + 1, kPublish);
        }
    });
    long bad = 0;
    for (int expect = 0; expect < count; ++expect) {
        const size_t t = tail.load(std::memory_order_relaxed);
        while (head.load(kObserve) == t) {}
        if (buf[t % N] != expect) ++bad;
        tail.store(t + 1, kPublish);
    }
    producer.join();
    std::printf("顺序错误 %ld 个\n", bad);
}
