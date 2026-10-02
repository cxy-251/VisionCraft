import QtQuick
import VisionCraft

Section {
    title: qsTr("工位固件的任务划分")
    lead: qsTr("三个任务、一个队列、一把锁。这一节讲工位固件为什么这样拆分，任务之间怎么传东西。")

    Why {
        text: qsTr("板子要同时做好几件节奏不同的事：每毫秒看一次有没有上位机发来的数据；每 10 毫秒扫一次按键；"
                 + "每 250 毫秒刷新屏幕；每 500 毫秒采一次传感器，而 ADC 转换本身要等。全写在一个死循环里，"
                 + "最慢的那件事会拖住所有事：采 ADC 的那几毫秒里，上位机的命令就没人理。"
                 + "FreeRTOS 把它们拆成几个各自循环的任务，由调度器按优先级轮流执行，谁在等就让出 CPU。")
    }

    Para { text: qsTr("任务在 .ioc 里声明（名字、优先级、栈大小、入口函数），CubeMX 生成创建代码。入口函数设成「外部」，由 App/ 里的代码实现：") }
    CodeRef { file: "firmware/station/station.ioc"; match: "^FREERTOS\\.Tasks01" }
    CodeRef { file: "firmware/station/Core/Src/freertos.c"; from: "^const osThreadAttr_t linkTask_attributes"; to: "^};" }

    KeyPoints {
        label: qsTr("三个任务")
        points: [
            qsTr("LinkTask，优先级「高于普通」(32)：收上位机的帧、执行命令、回应答。优先级最高，保证命令来了马上处理；没数据时 osDelay(1) 让出 CPU。"),
            qsTr("StationTask，「普通」(24)：扫按键、控制蜂鸣器、画屏幕。屏幕只有它一个任务画。"),
            qsTr("EnvTask，「低于普通」(16)：采温度、光照、电压，按订阅周期发遥测。ADC 转换慢，放最低，被谁打断都没关系。"),
            qsTr("栈大小 512 是「字」不是字节：生成的代码里写的是 512 * 4。")
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

    Pitfall {
        text: qsTr("旧版固件在串口中断里直接执行终端命令。命令里有忙等的蜂鸣（几百毫秒）、HAL_Delay、甚至 vTaskDelay，"
                 + "而串口中断的优先级（6）比系统节拍 SysTick（15，数字越大越低）高：中断执行期间系统时间停止前进，"
                 + "HAL_Delay 永远等不到头，vTaskDelay 根本不允许在中断里调用。原则是：中断里只收数据、放进队列，"
                 + "处理交给任务。工位固件干脆不用通信中断——RTT 是轮询的。")
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

    Try {
        task: qsTr("现在栈溢出没有检查：任务的栈用超了会悄悄踩坏别的内存。在 .ioc 里把 FreeRTOS 的 configCHECK_FOR_STACK_OVERFLOW 设成 2，"
                 + "重新生成，实现 vApplicationStackOverflowHook，在里面让蜂鸣器长响。然后故意把 envTask 的栈改成 64 试试。")
        answerNote: qsTr("方法 2 会在每次任务切换时检查栈末尾的 16 个字节有没有被改写。钩子函数在调度器上下文里执行，"
                       + "不能再调用会阻塞的 FreeRTOS 函数，直接操作 GPIO 拉高蜂鸣器引脚、然后死循环即可。"
                       + "EnvTask 调用 HAL 的 ADC 函数和 vc_tel_env_write，64 个字（256 字节）的栈很快就会溢出。")
    }

    InSystem {
        text: qsTr("这三个任务的实现分别在 App/link.c、App/station.c、App/env.c。上位机「设备」页的往返时间（5~7 ms）"
                 + "主要取决于 OpenOCD 的轮询间隔，而 LinkTask 高优先级、空闲时每毫秒查一次缓冲区，保证它不是瓶颈。")
    }
}
