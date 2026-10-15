import QtQuick
import VisionCraft

Section {
    title: qsTr("工位固件的任务划分")
    lead: qsTr("三个应用任务，加 USB 主机库任务和 FreeRTOS 系统任务。按当前固件说明任务职责、数据交接，以及哪些慢操作仍会阻塞界面。")

    Why {
        text: qsTr("板子要同时做好几件节奏不同的事：每毫秒看一次有没有上位机发来的数据；每 10 毫秒扫一次按键；"
                 + "目标每 20 毫秒处理界面刷新；每 500 毫秒采一次传感器，而 ADC 转换本身要等。全写在一个死循环里，"
                 + "最慢的那件事会拖住所有事：采 ADC 的那几毫秒里，上位机的命令就没人理。"
                 + "FreeRTOS 把它们拆成几个各自循环的任务，由调度器按优先级轮流执行，谁在等就让出 CPU。")
    }

    Para { text: qsTr("任务在 .ioc 里声明（名字、优先级、栈大小、入口函数），CubeMX 生成创建代码。入口函数设成「外部」，由 App/ 里的代码实现：") }
    CodeRef { file: "firmware/station/station.ioc"; match: "^FREERTOS\\.Tasks01" }
    CodeRef { file: "firmware/station/Core/Src/freertos.c"; from: "^const osThreadAttr_t linkTask_attributes"; to: "^};" }

    KeyPoints {
        label: qsTr("应用任务与库任务")
        points: [
            qsTr("LinkTask，优先级「高于普通」(32)，栈 2048 字节：收帧、执行协议命令、回应答。应用任务中优先级最高；无数据时 osDelay(1)。EEPROM 操作和发送锁等待仍可能耗时，不能只凭优先级推断响应延迟。"),
            qsTr("StationTask，「普通」(24)，栈 8192 字节：按键、触摸、鼠标、蜂鸣器、页面、读卡和图片解码、终端命令、健康检查和喂狗。LVGL 控件只有它操作。"),
            qsTr("EnvTask，「低于普通」(16)，栈 1536 字节：采温度、光照、电压，更新快照并按订阅周期发遥测。高优先级任务可以抢占它，采样与上报时序仍需测量。"),
            qsTr(".ioc 中的栈深 512 经 CubeMX 转成 .stack_size = 512 * 4。CMSIS 属性是字节，不要和 FreeRTOS 原生接口的 word 数混用。"),
            qsTr("USBH_Queue 是 USB 库创建的普通优先级任务，栈 1024 字节；另有 IDLE 和 Tmr Svc。实际 LinkTask 在 App/link.c；USB 主机由 StationTask 调用 mouse_init 启动，不能把生成文件末尾的弱 LinkTask 当成实际入口。")
        ]
    }

    Para {
        text: qsTr("任务之间要传东西时，用了三种办法，各对应一种场景。第一种是消息队列：LinkTask 收到「蜂鸣 300 ms」的命令，"
                 + "不能自己去等 300 毫秒（那段时间就收不了数据了），而是把时长放进队列立刻返回，由 StationTask 去开关蜂鸣器：")
    }
    CodeRef { file: "firmware/station/App/app.c"; region: "beep" }

    Para {
        text: qsTr("StationTask 的主循环把「等蜂鸣请求」和「定时扫按键」合在一起：从队列取请求时最多等 10 毫秒，"
                 + "等到了就开蜂鸣器，等不到也正好到了扫键的时间。蜂鸣器用定时器 PWM 发声，到时间关掉就行，不用忙等。")
    }
    CodeRef { file: "firmware/station/App/station.c"; region: "loop" }

    Para {
        text: qsTr("第二种是互斥锁：三个任务都要给上位机发帧（应答、按键事件、遥测），而发送缓冲区只有一个。"
                 + "两个任务同时往里写，两帧的字节会交错在一起，两帧都废了。所以发送前先拿锁：")
    }
    CodeRef { file: "firmware/station/App/app.c"; region: "send" }

    Para {
        text: qsTr("第三种是短暂锁住调度器：最新的传感器读数是一个只有几个字节的结构体，EnvTask 写、StationTask 读。"
                 + "复制它只要几十个时钟周期，用不着一把锁，在复制期间暂停任务切换就够了：")
    }
    CodeRef { file: "firmware/station/App/env.c"; region: "latest" }

    KeyPoints {
        label: qsTr("后续功能增加的数据交接")
        points: [
            qsTr("结果使用只保留最新值的单槽；图片通过接收、就绪、绘制等状态交接同一个缓冲；远程命令使用一个待执行槽，StationTask 取出执行并按原序号回包。这些不全是逐条排队的消息队列。"),
            qsTr("鼠标报告从 USB 库任务通过 32 项队列交给 StationTask，满时计数并丢弃。鼠标回调属于任务上下文，不是硬件中断。"),
            qsTr("SD 卡读操作由 StationTask 发起，DMA 完成或出错中断释放信号量。CPU 可以执行其他任务，但等待的 StationTask 自身不能同时刷新界面、处理触摸或喂狗。JPEG 解码和长终端命令也仍在这里执行。"),
            qsTr("三个应用任务报到，由 StationTask 检查并喂狗。它自己卡住时可能来不及写故障记录，只留下看门狗复位标志。是否需要工作任务，先测输入延迟、最长阻塞、队列积压和内存，再决定如何拆分。")
        ]
    }
    CodeRef { file: "firmware/station/App/app.c"; region: "shell" }
    CodeRef { file: "firmware/station/App/mouse.c"; region: "callback" }
    CodeRef { file: "firmware/station/App/sdcard.c"; region: "data-complete" }

    Pitfall {
        text: qsTr("旧版固件在串口中断里直接执行终端命令。命令里有忙等的蜂鸣（几百毫秒）、HAL_Delay、甚至 vTaskDelay，"
                 + "而串口中断的优先级（6）比系统节拍 SysTick（15，数字越大越低）高：中断执行期间系统时间停止前进，"
                 + "HAL_Delay 永远等不到头，vTaskDelay 根本不允许在中断里调用。原则是：中断里只收数据、放进队列，"
                 + "处理交给任务。当前 RTT 接收采用轮询；SDIO、DMA、USB 等仍有中断，调用 RTOS 接口时需要分别检查优先级。")
    }

    Pitfall {
        text: qsTr("调度器启动之前调用 FreeRTOS 的函数（例如创建、获取互斥锁），会把 BASEPRI 寄存器设成屏蔽低优先级中断，"
                 + "SysTick 和 HAL 的时基中断都进不来，随后的 HAL_Delay 就卡死。旧版固件因此开机死锁过。"
                 + "工位固件的顺序是：外设初始化 → 屏幕初始化（要用 HAL_Delay）→ 初始化内核 → 创建任务和锁 → 启动调度器：")
        CodeRef { file: "firmware/station/Core/Src/main.c"; from: "USER CODE BEGIN 2"; to: "osKernelStart" }
    }

    Pitfall {
        text: qsTr("在中断里调用 FreeRTOS 的 ...FromISR 函数时，这个中断的优先级数值必须大于等于 configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY"
                 + "（这里是 5，也就是 5~15 这些较低的优先级；configMAX_SYSCALL_INTERRUPT_PRIORITY 是同一个值移到寄存器对应位置后的结果），"
                 + "否则会破坏内核的数据结构，表现为偶发的、很难复现的死机。")
        CodeRef { file: "firmware/station/Core/Inc/FreeRTOSConfig.h"; match: "define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY" }
    }

    Para {
        text: qsTr("设备页现在和终端 ps 共用任务快照：显示任务名、状态、优先级和运行以来最小剩余栈（字节）。快照最多十任务，名称先复制再按名字排序，界面每秒更新；容量不足或采集失败时隐藏旧行，明确显示错误。")
    }
    CodeRef { file: "firmware/station/App/app.h"; region: "task-snapshot" }
    CodeRef { file: "firmware/station/App/app.c"; region: "task-snapshot" }
    CodeRef { file: "firmware/station/App/page_device.c"; region: "tasks" }
    Figure {
        files: ["handbook/f407/rtos/figures/device-tasks.png"]
        columns: 1
        captions: [qsTr("桌面模拟器的设备页，使用假任务数据；实板测量见下方记录")]
    }
    CodeRef { file: "handbook/f407/rtos/task-monitor-test.txt"; from: "==== 0"; to: "==== 5"; caption: qsTr("实板：快照、刷新频率、重复进入、ps/free 与内存") }
    CodeRef { file: "handbook/f407/rtos/task-monitor-test.txt"; from: "==== 5"; caption: qsTr("桌面错误分支回归与测量边界") }

    Try {
        task: qsTr("当前 configCHECK_FOR_STACK_OVERFLOW=2 已启用。先用 ps、free 记录栈余量和内存池，再反复进入设备页、执行不涉及存储的终端命令进行比较。为什么不能只凭本轮最小余量就宣布所有路径都不会溢出？")
        answerNote: qsTr("高水位只覆盖已执行过的路径，更深调用、错误处理和并发时序可能使用更多栈。历史文件页验证最少剩余 2668 字节；本次设备页验证最少剩余 4012 字节，分配 8192 字节，各自只覆盖对应路径。本次未重跑读卡测试。当前溢出钩子会记录任务并复位，故障注入见「看门狗、栈溢出与 HardFault 现场」，无需重新开启检查。")
    }
    CodeRef { file: "firmware/station/Core/Inc/FreeRTOSConfig.h"; match: "define configCHECK_FOR_STACK_OVERFLOW" }
    CodeRef { file: "handbook/f407/gui/files-browser-test.txt"; from: "==== 8"; to: "==== 9"; caption: qsTr("历史文件页测量，保留作路径比较，本项未重跑") }

    InSystem {
        text: qsTr("三个应用任务分别在 App/link.c、App/station.c、App/env.c。上位机「设备」页历史测得的往返时间（5~7 ms）与当时 OpenOCD 轮询间隔有关；当前仍需结合命令耗时、发送锁等待和实际负载评估，不能作为所有场景的延迟保证。")
    }
}
