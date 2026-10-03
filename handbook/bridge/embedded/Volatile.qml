import QtQuick
import VisionCraft

Section {
    title: qsTr("volatile 与寄存器访问")
    lead: qsTr("告诉编译器：这个变量随时会被「你看不见的人」改掉，每次都老老实实去内存里读、往内存里写。")

    Why {
        text: qsTr("写 PC 程序时，一个变量只有你的代码会改。单片机上不是这样：中断函数、另一个任务、DMA、调试器、"
                 + "甚至外设本身（寄存器）都会在你的代码不知情的时候改变内存里的值。而编译器做优化时假设的是"
                 + "「这段代码没改它，它就不会变」，于是会把读取挪到循环外面、合并、甚至删掉。volatile 就是关掉这类假设。")
    }

    Para { text: qsTr("一个最常见的场景：主循环等一个标志位，由中断把它置 1。两个版本只差一个 volatile：") }
    CodeRef { file: "handbook/bridge/embedded/volatile_demo.c"; region: "flags" }

    Para {
        text: qsTr("用本机的 arm-none-eabi-gcc 14.2、-O2 编译，看生成的汇编（每行左边是地址，右边是指令）：")
    }
    CodeRef { file: "handbook/bridge/embedded/volatile_demo.asm.txt"; caption: qsTr("实际编译结果") }

    KeyPoints {
        label: qsTr("读汇编")
        points: [
            qsTr("wait_plain：ldr 只读了一次 flag_plain。如果是 0，cbz 跳到地址 8，而地址 8 的指令是「b.n 8」——跳回自己。"
                 + "编译器认定「循环里没人改它，读一次就够了」，把等待变成了真正的死循环，中断把它改成 1 也没用。"),
            qsTr("wait_volatile：循环从地址 12 开始，每一圈都重新 ldr 读一次，再比较。中断一改，下一圈就能看到。"),
            qsTr("key0_pressed：读寄存器 0x40021010（GPIOE 的输入数据寄存器），取第 4 位再取反。寄存器指针必须是 volatile 的，"
                 + "否则连续读两次按键，编译器可能只读一次。")
        ]
    }

    CodeRef { file: "handbook/bridge/embedded/volatile_demo.c"; region: "register" }

    Para {
        text: qsTr("HAL 库里所有寄存器都是这样定义的（结构体成员带 __IO，也就是 volatile）。"
                 + "工位固件里自己用到 volatile 的地方，是和调试器共享的 RTT 缓冲区指针：调试器通过 SWD 改 rd_off，程序完全不知道：")
    }
    CodeRef { file: "firmware/station/App/vc_rtt.c"; region: "layout" }

    Pitfall {
        text: qsTr("volatile 只管「这一个变量的读写不被优化掉」，不管它和别的变量之间的先后顺序。"
                 + "工位固件收缩略图时先 memcpy 写像素、再把 volatile 的状态改成「收齐了」；编译器有权把像素写入挪到状态改动之后，"
                 + "另一个任务就可能画出半张图。写这一节时发现了这个隐患，修复方法是在两者之间加内存屏障：")
        CodeRef { file: "firmware/station/App/app.c"; region: "barrier" }
    }

    Pitfall {
        text: qsTr("volatile 也不是锁。i++ 对 volatile 变量仍然是「读、加、写」三步，中断插在中间就会丢一次计数。"
                 + "多个任务同时改的数据，要用互斥锁、关中断或原子操作（见「工位固件的任务划分」）。")
    }

    Try {
        task: qsTr("把 volatile_demo.c 里 wait_plain 的循环体改成 while (flag_plain == 0) { __asm volatile(\"\" ::: \"memory\"); }，"
                 + "重新编译、看汇编。它还是死循环吗？这句空的内联汇编起了什么作用？")
        answerNote: qsTr("不再是死循环：每一圈都会重新读 flag_plain。这句是「编译器屏障」，告诉编译器内存可能被改过，"
                       + "之前读到寄存器里的值都不能再信。CMSIS 的 __DMB() 除了这个作用，还会让 CPU 等前面的内存访问完成。"
                       + "它比 volatile 粗：影响的是屏障前后所有的内存访问，而不是一个变量。")
    }

    InSystem {
        text: qsTr("固件里所有外设寄存器访问都经过 HAL 的 volatile 定义；App/vc_rtt.c 的 RTT 指针、App/app.c 的缩略图状态是自己写的 volatile。")
    }
}
