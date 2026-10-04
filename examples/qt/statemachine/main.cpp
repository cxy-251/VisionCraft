// 状态机：工位一个检测周期的状态和转换，用一张表写清楚
//
// 运行：./example_qt_statemachine     输出见 output.txt

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QMetaEnum>
#include <QTimer>
#include <cstdio>
#include <map>

// [region states]
class Station : public QObject {
    Q_OBJECT
public:
    enum class State { Idle, WaitPart, Capturing, Inspecting, Fault };
    enum class Event { Start, Stop, PartArrived, Captured, Inspected, Timeout, Error, Reset };
    Q_ENUM(State)
    Q_ENUM(Event)
// [endregion]

    Station()
    {
        // [region table]
        // 转换表：(当前状态, 事件) → 下一个状态。表里没有的组合，一律拒绝
        m_table = {
            {{State::Idle,       Event::Start},       State::WaitPart},
            {{State::WaitPart,   Event::PartArrived}, State::Capturing},
            {{State::WaitPart,   Event::Stop},        State::Idle},
            {{State::Capturing,  Event::Captured},    State::Inspecting},
            {{State::Capturing,  Event::Timeout},     State::Fault},
            {{State::Inspecting, Event::Inspected},   State::WaitPart},
            {{State::Inspecting, Event::Timeout},     State::Fault},
            {{State::Fault,      Event::Reset},       State::Idle},
        };
        // 任何状态遇到 Error 都进 Fault（除了已经在 Fault）
        for (State s : {State::Idle, State::WaitPart, State::Capturing, State::Inspecting})
            m_table[{s, Event::Error}] = State::Fault;
        // [endregion]
        m_timer.setSingleShot(true);
        connect(&m_timer, &QTimer::timeout, this, [this] { post(Event::Timeout); });
    }

    // [region post]
    bool post(Event e)
    {
        const auto it = m_table.find({m_state, e});
        if (it == m_table.end()) {
            if (e == Event::Stop && m_state != State::Idle) {
                m_stopPending = true;                         // 周期中途要停：记下来，回到 WaitPart 时再停
                log("推迟", e, m_state);
                return false;
            }
            ++rejected;
            log("拒绝", e, m_state);
            return false;
        }
        const State from = m_state;
        m_state = it->second;
        log("转换", e, from);
        onEnter(m_state);
        return true;
    }
    // [endregion]

    State state() const { return m_state; }
    int rejected = 0;
    QElapsedTimer clock;

private:
    // [region enter]
    void onEnter(State s)                                     // 进入每个状态时要做的事集中在这里
    {
        m_timer.stop();
        switch (s) {
        case State::Capturing:  m_timer.start(200); break;    // 拍照 200 ms 内必须完成
        case State::Inspecting: m_timer.start(300); break;
        case State::WaitPart:
            if (m_stopPending) { m_stopPending = false; post(Event::Stop); }
            break;
        default: break;
        }
    }
    // [endregion]
    void log(const char *what, Event e, State from)
    {
        auto name = [](auto v) { return QMetaEnum::fromType<decltype(v)>().valueToKey(int(v)); };
        if (m_state == from)
            std::printf("  %5lld ms  %s：%-11s 在 %-10s 时%s\n", clock.elapsed(), what, name(e), name(from),
                        QByteArray(what) == "推迟" ? "先记下，这件做完再停" : "不允许");
        else
            std::printf("  %5lld ms  %s：%-10s ─%-11s→ %s\n", clock.elapsed(), what, name(from), name(e), name(m_state));
    }
    State m_state = State::Idle;
    bool m_stopPending = false;
    std::map<std::pair<State, Event>, State> m_table;
    QTimer m_timer;
};

// [region flags]
// 对照：用几个布尔变量记状态
struct FlagStation {
    bool running = false, busy = false, fault = false;
    void start() { running = true; }
    void partArrived() { if (running) busy = true; }
    void error() { fault = true; running = false; }
    void inspected() { busy = false; }
    void reset() { fault = false; }
};
// [endregion]

static void wait(int ms)
{
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < ms) QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    using E = Station::Event;

    std::printf("==== 1. 一个正常的周期 ====\n");
    {
        Station s;
        s.clock.start();
        for (E e : {E::Start, E::PartArrived, E::Captured, E::Inspected, E::Stop})
            s.post(e);
    }

    std::printf("\n==== 2. 不该出现的事件 ====\n");
    {
        Station s;
        s.clock.start();
        for (E e : {E::Captured, E::Start, E::Start, E::Inspected, E::Reset})
            s.post(e);
        std::printf("  共拒绝 %d 个事件，现在是 %s\n", s.rejected,
                    QMetaEnum::fromType<Station::State>().valueToKey(int(s.state())));
    }

    std::printf("\n==== 3. 拍照超时 ====\n");
    {
        Station s;
        s.clock.start();
        s.post(E::Start);
        s.post(E::PartArrived);
        wait(300);                                            // 相机一直没有回 Captured
        s.post(E::Captured);                                  // 迟到的结果
        s.post(E::Reset);
    }

    std::printf("\n==== 4. 周期中途按停止 ====\n");
    {
        Station s;
        s.clock.start();
        for (E e : {E::Start, E::PartArrived, E::Stop, E::Captured, E::Inspected})
            s.post(e);
    }

    std::printf("\n==== 5. 布尔变量版本 ====\n");
    {
        // [region flags-bug]
        FlagStation f;
        f.start();
        f.partArrived();
        f.error();                                            // 检测中出错
        f.reset();                                            // 复位
        f.partArrived();                                      // 没有重新启动，又来一件
        // [endregion]
        std::printf("  running=%d busy=%d fault=%d —— 没在运行，却「正在检测」\n", f.running, f.busy, f.fault);
        std::printf("  三个布尔量能表示 2×2×2 = 8 种组合，有意义的状态只有 5 种\n");
    }
    return 0;
}

#include "main.moc"
