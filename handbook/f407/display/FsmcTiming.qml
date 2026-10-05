import QtQuick
import VisionCraft

Section {
    title: qsTr("FSMC 时序与屏幕识别")
    lead: qsTr("同一块屏，写可以很快，读却要慢得多。FSMC 的时序参数一档档调快，写和读分别在哪一档开始出错，用调试器一测便知；扩展模式让读写各用各的时序。")

    Why {
        text: qsTr("「FSMC 与 8080 并口」讲了屏幕怎么挂在 FSMC 上，那里提到 CubeMX 默认的时序（地址建立 15、数据建立 60 个 HCLK）很保守，刷满一屏要很久。"
                 + "把时序调快能让画图快好几倍，但快到什么程度会出错？只看芯片手册的时序表，换算起来容易出错，而且不同批次的屏也不完全一样。"
                 + "本节在板子上直接试：CPU 暂停，只动屏幕左上角一行 32 个像素（先存下、最后写回），每一档时序分别检验「写」和「读」。")
    }

    CodeRef { file: "tools/fsmc_timing_probe.tcl"; region: "id" }
    CodeRef { file: "handbook/f407/display/fsmc-timing.txt"; from: "==== 1"; to: "0xD300"; caption: qsTr("读 ID") }

    KeyPoints {
        label: qsTr("屏幕识别")
        points: [
            qsTr("驱动要先知道接的是哪种控制器，才能发对初始化序列。各家控制器读 ID 的命令不同：NT35510 是 0xDA00 / 0xDB00 / 0xDC00 三个寄存器，ILI9341 一类是 0xD3。"),
            qsTr("这块屏 0xDB00 读到 0x0080，正是 NT35510 的 ID2；ILI93xx 的 0xD3 读到全 0、通用的 RDDID（0x04）也是全 0。固件 App/lcd.c 的判断就是「ID2 低字节为 0x80」。"),
            qsTr("开发板厂家的例程常见的做法是依次发几种控制器的 ID 命令，看哪个读回的值对得上，以此支持多种屏。只用一种屏的项目，读一次核对就够了：读不到预期的 ID，多半是 FSMC 引脚、时序或复位有问题，比一上来就初始化、看到白屏再查省事。")
        ]
    }

    CodeRef { file: "tools/fsmc_timing_probe.tcl"; region: "sweep" }
    CodeRef { file: "handbook/f407/display/fsmc-timing.txt"; from: "==== 2"; to: "     0     1"; caption: qsTr("时序扫描") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("写：从最慢一直到 ADDSET=0、DATAST=1（每次访问粗算约 17 ns），32 个像素全部写对。"),
            qsTr("读：DATAST=15（约 190 ns）以上全对；8/8 开始偶尔出错（几次运行错 0–2 个），4/6 错 3–5 个，再快就几乎全错。"),
            qsTr("「每次访问约」一列按 (ADDSET + DATAST + 2) 个 HCLK 粗算，只用来比较快慢，不是精确的总线周期。"),
            qsTr("所以默认的 15/60 对写来说慢了二十多倍，对读来说也还有 2–3 倍的余量。")
        ]
    }

    Pitfall {
        text: qsTr("调试器发出的访问是一个一个分开的，两次访问之间隔着零点几到一毫秒；CPU 刷屏时却是连续不断地写。连续写时，屏的控制器还有「两次写之间最短间隔」之类的要求，这个脚本测不出来。"
                 + "所以「调试器测试写到 17 ns 都没问题」不代表固件里可以设这么快。实际调时序，还要在固件里连续刷屏，用读回或者眼睛确认，并留出余量。")
    }

    CodeRef { file: "tools/fsmc_timing_probe.tcl"; region: "extmod" }
    CodeRef { file: "handbook/f407/display/fsmc-timing.txt"; from: "==== 3"; caption: qsTr("扩展模式") }

    Para {
        text: qsTr("读写需要的时间差这么多，用同一套时序只能迁就慢的那个。FSMC 的扩展模式（BCR4 的 EXTMOD 位）让读继续用 BTR4、写改用 BWTR4："
                 + "这里读保持原来的 15/60，写用 1/2，写入再读回 32 个像素全对。CubeMX 的 FSMC 配置里有对应的扩展模式开关（选项名以界面为准），打开后会多出一组写时序参数。"
                 + "测完把 BCR4、BWTR4、BTR4 都写回了原值，那一行像素也写回了原样。")
    }

    Try {
        task: qsTr("工位固件刷新屏幕时几乎只写不读（只有 tools/lcd_readback.tcl 这类调试工具会读）。按本节结果，固件的时序该怎么改？")
        answerNote: qsTr("打开扩展模式：读时序保持慢的（比如 15/30，保证调试读回可靠），写时序调快，但不要直接用本节测到的极限 1/2，而是在固件里连续刷屏验证过、再留余量（比如 2/5 左右起步）。"
                       + "这是根据本节测量的建议，还没有改进固件里实际测刷屏速度。")
    }

    InSystem {
        text: qsTr("工位固件已经按本节改了：station.ioc 里打开扩展模式（ExtendedMode1），写时序 ADDSET=2、DATAST=5，读时序保持 15/60，由 CubeMX 生成进 Core/Src/fsmc.c。"
                 + "实测换一页整屏重画从 242–353 ms 降到 31–45 ms，读回显存仍然正确，看屏的人确认画面正常（见「事件与控件：一个最小的界面框架」）。")
    }
}
