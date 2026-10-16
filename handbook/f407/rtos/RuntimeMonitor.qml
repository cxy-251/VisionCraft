import QtQuick
import VisionCraft

Section {
    title: qsTr("Qt运行监控与RTT背压")
    lead: qsTr("把板上设备页和 ps/free 的观察扩展到上位机：任务、内存、响应计时和复位现场用结构化协议传给Qt，既能看当前值，也能比较一段时间内的变化。")
    Why {
        text: qsTr("不需要添加外设或连线。已有的任务快照、计时计数器和故障记录足以提供运行监控；关键是让数据所属任务生成快照，避免把LVGL或终端搬进通信任务，也避免靠解析终端文字维持界面。读取监控不会启动自检、访问SD或写入EEPROM/Flash。")
    }
    KeyPoints {
        label: qsTr("请求和所有者")
        points: [
            qsTr("GET_MONITOR（0x10）是空请求。LinkTask只交接一个序号，槽满立即返回BUSY；StationTask每轮检查请求，采集任务、LVGL内存及其他计数器，编码后回应。没有新增任务或动态分配；应答缓冲680字节放在外部静态区，受链接器的暂存区隔离检查保护。"),
            qsTr("任务快照复用设备页的实现：名称先复制再排序，最多十任务。容量不足时任务状态标记失败、行数为零，其他内存和故障字段仍可读；不能把旧任务行留在屏幕上当作当前值。"),
            qsTr("schema 1：112字节头部、18×16字节计时、7×4字节流量、每任务25字节；六任务数据578字节，最大678字节，前面另加一字节应答状态。多字节字段按小端逐项编码，不直接发送C结构体。长度、schema、容量、名称终止、任务状态与排序都校验。"),
            qsTr("监控本身也有开销：任务栈扫描、快照编码、发送锁及RTT等待都占时间；后台工作计时包含该路径。数值是已执行路径的观察，不是零开销探针，也不保证最坏时延。")
        ]
    }
    CodeRef { file: "protocol/vc_protocol.h"; region: "monitor" }
    CodeRef { file: "protocol/vc_protocol.c"; region: "monitor" }
    CodeRef { file: "firmware/station/App/monitor.c"; region: "handoff" }
    KeyPoints {
        label: qsTr("Qt如何处理快照")
        points: [
            qsTr("设备页可每秒刷新、暂停或手动刷新；离页停止轮询，未完成请求不重复排队。断开或收到设备HELLO会清空旧快照和历史，旧连接/旧启动的迟到回调不能重新发布旧数据。"),
            qsTr("超时、格式错误或读取失败会清空当前快照；旧固件返回UNKNOWN时明确提示不支持并停止自动尝试，连接与其他功能继续可用。任务快照失败只清任务行，保留同包中有效的资源信息。"),
            qsTr("历史最多保留120次成功快照，按电脑收到数据的时刻画2分钟趋势：青色是可用FreeRTOS堆，黄色是LVGL使用量。失败不填造样本，长间隔不连线；板子uptime回退或计时窗口epoch改变则清历史。暂停后仍显示数据年龄。模拟通道明确标为模拟器。"),
            qsTr("任务剩栈是运行以来的最小余量；计时包含等待和抢占，所以不是CPU占用率。尚未发生的计时项显示尚无样本；启动和换页也可能进入累计峰值，不能把峰值都归因于稳定采集负载。")
        ]
    }
    CodeRef { file: "src/device/DeviceLink.cpp"; region: "monitor" }
    CodeRef { file: "qml/device/RuntimePanel.qml" }
    Figure {
        files: ["handbook/f407/rtos/figures/runtime-monitor.png"]
        captions: [qsTr("Qt原生监控控件通过RTT连接实板，已暂停刷新；累计丢弃2492字节保留了主动背压实验的证据")]
    }
    Pitfall {
        text: qsTr("停止主机应用的socket读取不一定形成板端背压：OpenOCD和TCP可能继续替它排空环形区。本机实测 rtt stop 后，板上读写指针仍同步推进，丢弃计数没有增加，因此没有把这次尝试算作满缓冲验证。最终短暂停住OpenOCD进程，让MCU继续运行；环形区真正满后，整帧等待超时并增加丢弃字节，恢复调试器后再检查帧同步和命令应答。")
    }
    CodeRef { file: "firmware/station/App/vc_rtt.c"; region: "write" }
    CodeRef { file: "tools/runtime_probe.py" }
    CodeRef { file: "handbook/f407/rtos/runtime-monitor-test.txt"; from: "==== 构建及固件资源"; to: "==== 桌面协议"; caption: qsTr("构建、烧录与资源") }
    CodeRef { file: "handbook/f407/rtos/runtime-monitor-test.txt"; from: "==== 10分钟稳定性"; to: "以下原始输出"; caption: qsTr("600秒实板窗口摘要；完整逐次快照保留在同一记录文件") }
    CodeRef { file: "handbook/f407/rtos/runtime-monitor-test.txt"; from: "==== 验证边界"; to: "==== 最终相关回归"; caption: qsTr("验证范围和测量边界") }
    CodeRef { file: "handbook/f407/rtos/runtime-monitor-test.txt"; from: "==== 最终相关回归"; to: "==== 归档结束"; caption: qsTr("主机回归及实板Qt控件测试") }
    CodeRef { file: "tests/tst_runtime_ui.cpp" }
    Try {
        task: qsTr("为什么RTT丢弃计数增加以后，恢复测试要比较后续增量，而不能要求计数重新为零？")
        answerNote: qsTr("这是本次启动以来的累计字节数，暂停调试器时预期会增加；恢复不应清掉证据。记录压力前、恢复后及稳定窗口末尾三个值，分别检查诱发阶段确实丢弃、后续正常负载没有继续丢弃。不能为了显示零错误而复位板子。")
    }
    InSystem {
        text: qsTr("这项把下位机FreeRTOS实践接到Qt主线。保留现有六任务，新增只读监控通道和有界历史；不扩展已取消的串口、RS232/485、红外或Flash文件系统，也不触碰SD卡。")
    }
}
