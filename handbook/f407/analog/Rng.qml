import QtQuick
import VisionCraft

Section {
    title: qsTr("真随机数 RNG")
    lead: qsTr("F407 里有一个硬件随机数发生器：用芯片内部模拟电路的噪声产生随机位，每次读 32 位。和软件伪随机数不同，它不能由一个种子推算出来。")

    Why {
        text: qsTr("单片机上的 rand() 是伪随机数：同样的种子，每次上电得到同样的序列。抽检工件、生成通信里的随机数、给加密算法做种子，都需要不可预测的数。"
                 + "单片机上电时没有时钟、没有用户输入，几乎找不到靠得住的种子，硬件 RNG 就是为此准备的。"
                 + "本节先在 CubeMX 的副本里打开 RNG 看生成了什么代码，再用调试器直接操作板子上的 RNG 寄存器取一批数，最后在电脑上做几项统计。")
    }

    KeyPoints {
        label: qsTr("CubeMX 配置")
        points: [
            qsTr("RNG 没有引脚，也没有可调参数，只有「打开」一个开关。在 .ioc 里它是一个虚拟引脚 VP_RNG_VS_RNG，模式 RNG_Activate（下面的副本就是这样手工加的）。"),
            qsTr("RNG 要一个 48 MHz 的时钟，来自主 PLL 的 Q 输出（PLL48CLK，和 USB 共用）。工位固件的 station.ioc 里 PLLQ=7，正好 336 MHz ÷ 7 = 48 MHz；板子上读回的 RCC_PLLCFGR 是 0x07405408，最高那位 7 就是 PLLQ。"),
            qsTr("为了不改动真正的固件，示例把 station.ioc 复制一份、在副本里打开 RNG，用命令行 CubeMX 重新生成（见「.ioc 文件与代码生成」），然后看它改了什么。")
        ]
    }

    CodeRef { file: "handbook/f407/analog/rng-cubemx.txt"; caption: qsTr("CubeMX 生成的结果（副本）") }

    Para {
        text: qsTr("和其他外设一样：新增 rng.c / rng.h，main.c 里多一行 MX_RNG_Init()，hal_conf.h 打开 HAL_RNG_MODULE_ENABLED，HAL 的 RNG 驱动被拷进 Drivers/，cmake/stm32cubemx/CMakeLists.txt 多了两个源文件。"
                 + "CubeMX 还把 .ioc 里外设和虚拟引脚的顺序按字母重新排了一遍，所以 .ioc 的 diff 比手工改的那几行大。"
                 + "编译出来 Flash 多 136 字节、RAM 多 16 字节（hrng 句柄）。HAL 里取数的函数是 HAL_RNG_GenerateRandomNumber(&hrng, &value)。")
    }

    CodeRef { file: "tools/rng_probe.tcl"; region: "enable" }
    CodeRef { file: "tools/rng_probe.tcl"; region: "read" }
    CodeRef { file: "handbook/f407/analog/rng-probe.txt"; caption: qsTr("在板子上运行（固件照常运行，RNG 用完即关）") }

    KeyPoints {
        label: qsTr("寄存器")
        points: [
            qsTr("先给外设送时钟：RCC_AHB2ENR 第 6 位（RNGEN）。读回从 0x0 变成 0x40。这一步就是生成代码里的 __HAL_RCC_RNG_CLK_ENABLE()。"),
            qsTr("RNG_CR 第 2 位启动它；RNG_SR 第 0 位 DRDY=1 表示 DR 里有一个新数；第 1、2 位（CECS、SECS）是时钟错误和种子错误，第 5、6 位是对应的中断标志。"),
            qsTr("4096 次读取，一次也没有出错，也一次都没有遇到 DRDY=0：经调试器读一次寄存器要将近 1 毫秒（4096 个数约 4 秒），而按参考手册，RNG 每 40 个 PLL48 时钟周期（不到 1 微秒）就产生一个新数。程序里直接读时，就必须先等 DRDY。"),
            qsTr("用完后把 RNG_CR 和 RCC_AHB2ENR 恢复成原值，固件不受影响。")
        ]
    }

    Pitfall {
        text: qsTr("调试脚本中途出错，「恢复原状」的那几行就不会执行。第一次运行时 echo 里的变量名紧挨着中文字符，Tcl 把「$notReady；错误标志出现」当成了一个变量名，脚本在恢复之前就停了，RNG 的时钟一直开着。"
                 + "第二次运行读到的「原值」已经是 0x40，恢复了等于没恢复，最后只好手工写回 0。在 Tcl 里变量后面紧跟非 ASCII 字符时，要写成 ${notReady}。"
                 + "直接改寄存器的脚本，先记下原值，再改，最好把恢复写在出错也会执行的地方。")
    }

    CodeRef { file: "tools/rng_stats.py"; region: "checks" }
    CodeRef { file: "handbook/f407/analog/rng-stats.txt"; caption: qsTr("统计结果：硬件 RNG 和两个软件生成器对照") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("1 的比例：三者都接近 0.5。这一项连「计数器 ^ 常数」这种完全可预测的序列都能通过。"),
            qsTr("字节分布的卡方值（256 个桶，自由度 255，真随机时应在 255 附近，大约 212–302 之间都算正常（95% 的范围））：硬件 RNG 两次运行分别是 263.5 和 246.4，计数器序列是一百多万，一眼就露馅。"),
            qsTr("压缩比：随机数据压缩不了（1.001），计数器序列能压到 35%。"),
            qsTr("教科书线性同余发生器（x = x × 1103515245 + 12345）在前面几项都过关，卡方 209.5 甚至「太均匀」；但它的最低位是 0101 交替——任何「取模 2」的用法都会得到一个完全可预测的结果。硬件 RNG 的最低位没有这种规律。")
        ]
    }

    Pitfall {
        text: qsTr("统计检查只能发现「明显不随机」，不能证明「随机」。上面这几项连一个设计很差的发生器都没能全部识破。要认真评估随机数质量，用专门的测试套件（如 NIST SP 800-22、dieharder），样本量要大得多，本节只读了 16 KB，没有做这类测试。"
                 + "芯片的 RNG 也会出错：时钟不对或噪声源异常时 SR 的 CECS / SECS 置位，这时读到的数不能用。HAL_RNG_GenerateRandomNumber 会检查并返回错误，自己读寄存器时要自己检查。")
    }

    Try {
        task: qsTr("用 tools/rng_stats.py 对比时，把 LCG 的结果右移 16 位再用（x >> 16，很多 C 库的 rand() 就是这么做的），最低位还会 0101 交替吗？")
        answerNote: qsTr("用 Python 按同样的公式算了一下：右移 16 位后，最低位前 16 个是 0010111001010110，看不出交替；它的周期是 131072 = 2^17。线性同余发生器模 2^32 时，越低的位周期越短，所以很多 C 库的 rand() 只返回高位。但这只是让规律不那么明显，序列仍然完全由种子决定。")
    }

    InSystem {
        text: qsTr("工位固件目前没有用到 RNG。上位机生成测试工件时用的是带固定种子的伪随机数（src/vision/PartGenerator 的构造函数接收一个 seed），这样每次生成的工件都一样，检测结果才能复现——这种场合要的恰恰是「可预测」。"
                 + "以后如果固件需要不可预测的数（比如随机抽检、通信会话号），就在 CubeMX 里打开 RNG，按本节生成的代码调用 HAL_RNG_GenerateRandomNumber。")
    }
}
