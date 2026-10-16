import QtQuick
import VisionCraft

Section {
    title: qsTr("界面响应：测量阻塞与任务协作")
    lead: qsTr("使用设备页、RTT 传图和非存储终端命令建立基线，再让 StationTask 做五次 180 ms 可抢占工作，分别观察界面与通信。全程不操作 SD 卡。")

    Why {
        text: qsTr("高优先级的通信任务还在回包，并不能说明界面任务及时处理了输入。先把通信往返、等待输入轮询、界面刷新间隔、命令等待和执行耗时分开，才能决定该把哪段工作搬到其他任务。")
    }
    Para {
        text: qsTr("本轮没有新建任务或业务队列。计时使用 168 MHz DWT 周期差；统计固定放在 RAM，每项记录次数、最近值、峰值和超过 20 ms 的次数，不在热路径打印日志。每项由固定任务写，StationTask 在暂停调度期间清零新窗口。")
    }
    CodeRef { file: "firmware/station/App/perf.h"; region: "metrics" }
    KeyPoints {
        label: qsTr("几个容易混淆的时间")
        points: [
            qsTr("loop/touch/gui gap 是两次处理开始之间的间隔；gui_work 是一次 gui_tick 调用的耗时。间隔变长不等于这一帧绘制花了同样久。"),
            qsTr("input_mark 从 LinkTask 发布标记到 StationTask 的输入轮询处消费，用来观察输入处理机会；不包含触摸芯片扫描、真实按压和像素呈现，不能称为端到端触摸延迟。"),
            qsTr("shell_wait 从命令入槽到取出；shell_exec 包含命令函数及其输出收集，后台阶段还包括回应答。单槽满时立即回 BUSY，拒绝的命令不进入等待统计。"),
            qsTr("image_wait 从最后一块接收完成到 StationTask 取图；image_work 覆盖缩略图复制/应用至缓冲归还。LVGL 随后刷新到屏幕的时间另计。"),
            qsTr("tx_wait 是取得发送互斥锁前的等待，tx_hold 覆盖编码和 RTT 写入；按三个应用任务分别统计。它们都包含可能的抢占，不是纯 CPU 时间。")
        ]
    }
    CodeRef { file: "firmware/station/App/app.c"; region: "send" }
    Para {
        text: qsTr("受控工作只有调试器写入请求才执行，一次最长 250 ms，执行前清掉请求，默认关闭。实验用 180 ms，不关闭中断和任务调度，也不访问外设；LinkTask 和 EnvTask 是否还能运行由回包和遥测验证。")
    }
    CodeRef { file: "firmware/station/App/station.c"; region: "controlled-work" }
    Para {
        text: qsTr("2026-10-15历史窗口分为空闲、终端、图片和受控工作四阶段，每阶段清统计，均并发50 ms遥测；分别执行30次短PING、24条诊断命令、五张38400字节图片、五次180 ms工作。五图等待峰值7.9555 ms、消费峰值3.3356 ms；StationTask最小剩栈3996/8192字节、LVGL峰值31080/45056字节、FreeRTOS最小剩堆18904字节。原始逐项统计保留在仓库，不与后来RTT满缓冲窗口合并。")
    }
    CodeRef { file: "handbook/f407/rtos/response-test.txt"; match: "^PASS: four non-storage"; caption: qsTr("四阶段历史窗口的原始汇总") }
    KeyPoints {
        label: qsTr("本轮能得出的结论")
        points: [
            qsTr("空闲、诊断命令和五张 RTT 图片的负载下，输入轮询标记峰值不超过 9.02 ms，界面调用间隔峰值不超过 28.74 ms。它们是本次有限窗口的观测值，不是实时上界。"),
            qsTr("180 ms 受控工作让输入标记等了 188.06 ms，界面调用间隔达到 201.11 ms，命令入槽后等待达 170.31 ms；同时短 PING 最大往返只有 8.20 ms。这直接展示了任务间的职责边界。"),
            qsTr("五轮各连发三条命令，均一条接收、两条 BUSY，累计拒绝十条，随后继续执行正常。BUSY 是有界单槽的背压，不是损坏的回包。"),
            qsTr("本轮发送锁等待峰值约 0.0041 ms，没有 RTT 丢字节。这个负载下没有把发送改成独立任务的证据；主机停止读取、RTT 满缓冲等情况仍未覆盖。"),
            qsTr("暂时保留当前任务划分。后续确有慢计算/等待的功能应采用分步执行或工作任务交付结果，由 StationTask 更新 LVGL；不能据此把所有终端命令都移到另一个任务，因为有的命令直接操作界面或硬件。")
        ]
    }
    Try {
        task: qsTr("如果 PING 只有 6 ms，而 input_mark 达到 188 ms，能否通过调低 LinkTask 优先级解决界面卡顿？")
        answerNote: qsTr("本次卡顿来自 StationTask 自己执行 180 ms 工作；调低通信任务优先级不会让它同时轮询输入。应先让慢工作分段让出控制，或放到有界工作任务，明确输入/结果/缓冲所有权与队列满时的处理方式。")
    }
    Pitfall {
        text: qsTr("主机 PING 时间含 TCP、OpenOCD 轮询、传输队列和 Python 调度；传图阶段的 PING 还排在四个图片分块之后，不可拿它推算板内的互斥锁等待。超过 20 ms 的次数只是严格阈值计数，周期略高于 20 ms 也会计入，不能当成丢帧数。鼠标、蜂鸣、结果槽的计数为零也不表示这些路径经过了满队列压力测试。")
    }
    InSystem {
        text: qsTr("tools/response_probe.py 独占自己启动的 OpenOCD，解析当前 ELF 的符号，协议编码和 CRC 直接复用 protocol/vc_protocol.c。它只进入 HOME、DEVICE、STATION，命令白名单为 uptime/sensors/ps/free；实验后关遥测，留在设备页。")
    }
}
