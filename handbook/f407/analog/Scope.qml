import QtQuick
import VisionCraft

Section {
    title: qsTr("DAC → ADC 回环示波器")
    lead: qsTr("让 DAC 由定时器和 DMA 驱动、不停地输出正弦，ADC 也由定时器和 DMA 驱动、按固定速率采样同一个引脚，就得到一台简陋的「信号发生器 + 示波器」。整个过程 CPU 一行代码也不用执行。")

    Why {
        text: qsTr("「DAC」一节是一个点一个点地写、一个点一个点地读，靠调试器每次几毫秒，测不了随时间变化的信号。真实的采集要求「每隔固定时间采一个点」，这件事靠程序循环是做不准的，要交给硬件：定时器决定节拍，DMA 负责搬数据。"
                 + "本节全部用调试器配置寄存器：TIM6 + DMA1 让 DAC 按一张 128 点的正弦表输出，TIM2 + DMA2 让 ADC2 采 2048 个点放进 RAM，采完读出来在电脑上画图。PA4 同时是 DAC 输出和 ADC 输入，不用接线。"
                 + "用到的定时器、DMA 流、DAC、ADC2 工位固件都没用，固件照常运行；正弦表和采样缓冲放在固件没用到的那段 RAM（0x2001A000 起，固件的变量到 0x200172C0 为止）。")
    }

    CodeRef { file: "tools/scope_probe.tcl"; region: "dac-dma" }

    KeyPoints {
        label: qsTr("DAC 这一路")
        points: [
            qsTr("TIM6 每次计满产生一个更新事件，配置成把它送到 TRGO 输出；DAC 选 TIM6 TRGO 作触发源。每触发一次，DAC 就向 DMA 要一个新值。"),
            qsTr("DMA1 流 5（通道 7）把正弦表一个个搬到 DAC 的数据寄存器，循环模式：搬到表尾自动回到表头，永远不停。"),
            qsTr("TIM6 的时钟 84 MHz，重装值 656 − 1，每秒触发 128000 次；表有 128 个点，所以正弦是 1000 Hz。"),
            qsTr("OpenOCD 的 Tcl 里没有 sin()，正弦表在电脑上算好存成 tools/scope_sine128.txt：2048 + 1600 × sin，留出两端，避开「DAC」一节测到的饱和区。")
        ]
    }

    CodeRef { file: "tools/scope_probe.tcl"; region: "adc-dma" }

    KeyPoints {
        label: qsTr("ADC 这一路")
        points: [
            qsTr("TIM2 的 TRGO 作 ADC2 的外部触发（上升沿），每触发一次转换一个点；DMA2 流 2（通道 1）把结果搬进 RAM，不循环，搬满 2048 个就停，并置「传输完成」标志。"),
            qsTr("采样时间设 28 个 ADC 时钟，加上 12 位转换的 12 个，每个点约 40 / 21 MHz ≈ 1.9 µs，所以最快约 52 万次/秒。下面试了 1.1 千、5 万、20 万、50 万次/秒。")
        ]
    }

    CodeRef { file: "handbook/f407/analog/scope-probe.txt"; from: "ADC_RATE 200000"; to: "1000.50 Hz"; caption: qsTr("采样 200 kHz") }
    Figure {
        files: ["handbook/f407/analog/scope-200k.png"]
        captions: [qsTr("2048 个点，约 10 个周期。上下限约 400 和 3700，对应表里的 448 和 3648，和「DAC」一节测到的偏差一致")]
    }

    Para {
        text: qsTr("频率按「带回差的过零」计算（tools/scope_analyze.py）：1000.5 Hz，和设计的 1000 Hz 差 0.05%，这个差别主要来自 TIM6 的分频取整：84 MHz / 656 = 128049 次/秒，正弦实际是 1000.4 Hz。")
    }

    Figure {
        files: ["handbook/f407/analog/scope-500k-zoom.png"]
        captions: [qsTr("采样 500 kHz，只看前 150 个点")]
    }

    KeyPoints {
        label: qsTr("看清台阶")
        points: [
            qsTr("把采样率提到 500 kHz、放大看开头一小段，正弦变成了台阶：DAC 每 1/128000 秒（7.8 µs）才变一次，ADC 在这期间采了约 4 个点，读到的都是同一级。真实的信号发生器会在 DAC 后面接一个低通滤波器把台阶抹平。"),
            qsTr("第一个点是 20：ADC 刚打开后的第一次转换不可信。四种采样率下第一个点都只有 20–47，其余点最低也有 395。分析时要扔掉它。")
        ]
    }

    CodeRef { file: "tools/scope_analyze.py"; region: "freq" }

    Pitfall {
        text: qsTr("一开始用「穿过平均值就算一次」来数周期，500 kHz 的数据算出 1668 Hz，明显不对。台阶的某一级正好落在平均值附近时，几个采样点在平均值上下抖动，一个周期被数成了好几次。"
                 + "改成带回差的检测（越过「平均值 + 200」才算一次，之后要先跌到「平均值 − 200」以下才能再算）就对了：1000.67 Hz。任何「数过零次数」的测频方法都要加回差。")
    }

    Figure {
        files: ["handbook/f407/analog/scope-1100.png"]
        captions: [qsTr("采样 1.1 kHz，只看前 60 个点")]
    }
    CodeRef { file: "handbook/f407/analog/scope-probe.txt"; from: "ADC_RATE 1100"; to: "99.61 Hz"; caption: qsTr("采样 1.1 kHz") }

    Pitfall {
        text: qsTr("信号还是那个 1000 Hz 的正弦，采样率降到 1100 次/秒后，采到的数据是一条干净的、约 100 Hz 的正弦（99.6 Hz），看不出任何异常。"
                 + "这就是混叠：采样率低于信号频率的两倍时，高频信号会「伪装」成一个低频信号，差值 1100 − 1000 = 100 Hz。数据本身没法告诉你发生了混叠，只能在采样之前用低通滤波器把高于采样率一半的成分滤掉。"
                 + "工位上用 ADC 采传感器时，传感器信号里如果混着电源的 50 Hz 干扰、电机的振动，低采样率就可能把它们变成看起来很真实的慢变化。")
    }

    CodeRef { file: "tools/scope_probe.tcl"; region: "clear-flags" }
    CodeRef { file: "tools/scope_probe.tcl"; region: "reset-counter" }

    Pitfall {
        text: qsTr("这个脚本第一次运行一切正常，第二次运行起就「什么也没发生」。查下来是两个上次留下的状态："
                 + "一是 DAC 的 DMA 欠载标志（DAC_SR = 0x2000，上次停止时 DMA 先停而 DAC 还在要数据），它不清掉，DAC 就不再请求 DMA，输出一直是 0 V；"
                 + "二是 TIM2 的计数器停在上次的 2500 万左右，比新设的重装值大得多，32 位计数器要数到 2^32（约 51 秒）才回绕产生第一个更新事件，ADC 一个点也没采。"
                 + "所以配置外设之前，先清它的状态标志、把计数器清零并用 UG 位装入新参数。固件里每次上电都是复位状态，碰不到这类问题；但只要「停下来再重新启动」，就要考虑。")
    }

    Try {
        task: qsTr("如果把 ADC_RATE 设成 2000（正好是信号频率的两倍），采到的会是什么样子？")
        answerNote: qsTr("实测（报告最后一段、下图）：相邻两个点一高一低交替（3628、512、3600、481……），像是采到了波峰和波谷；但幅度在 1 秒里慢慢缩到几乎为零，再慢慢变回来。"
                       + "原因是 DAC 的正弦实际是 1000.4 Hz，不是严格的 1000 Hz，两个采样点在正弦上的相位一点点漂移：漂到正好落在两个过零点附近时，采到的就几乎是一条直线。"
                       + "这时 scope_analyze.py 算出的「900.64 Hz」没有意义（幅度太小时回差判断失效）。「采样率至少要大于信号频率的两倍」，正好两倍是不够的。")
    }
    Figure {
        files: ["handbook/f407/analog/scope-2000.png"]
        captions: [qsTr("上面练习的实测：采样率正好是信号的两倍，一高一低交替，幅度随相位漂移慢慢变化")]
    }

    InSystem {
        text: qsTr("本节是调试器版本的示波器。做成工位系统的功能时，固件负责这些配置，并把 DMA 采满的数据经 RTT 送给上位机（吞吐量见「RTT：经调试口通信」，约 80 KB/s，足够每秒传几十帧 2048 点的波形），上位机在「数据」页实时画出来。"
                 + "这一步还没有做；本节的寄存器配置、要清的标志、要扔掉的第一个点，都是到时候要搬进固件的。")
    }
}
