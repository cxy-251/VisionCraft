import QtQuick
import VisionCraft

Section {
    title: qsTr("波形文件：保存与离线回放")
    lead: qsTr("把一张完整的ADC采样帧保存在电脑上，断开开发板后仍能查看时域和频谱。实时与离线使用同样的原始样本、实际采样率和分析方法，方便复现实验。")
    Why {
        text: qsTr("屏幕截图能记录外观，不能重新计算频谱，也无法检查原始ADC码。保存原始帧和必要元数据，才能比较参数、检查算法，并在没有开发板时继续学习Qt与OpenCV。文件全部写在电脑上，不使用板载SD卡或Flash文件系统。")
    }
    KeyPoints {
        label: qsTr("在数据页使用")
        points: [
            qsTr("采一帧，或暂停连续采集后，点击保存这一帧，选择电脑上的目标文件。文件扩展名为.vcscope.json；保存的是点击按钮时那张完整帧，不随之后的实时更新改变。"),
            qsTr("打开采样文件无需连接设备。成功加载后停止当前连续采集，标题明确标记离线回放，并显示原始来源、电脑接收时间及文件采样参数。放大、平移和频率视口仍可使用；返回实时后再由用户启动采集。"),
            qsTr("加载失败显示原因，保留上一份有效回放；不会把不完整样本画成新帧。即使底层收到别的实时帧或连接断开，也不会覆盖正在查看的文件。")
        ]
    }
    KeyPoints {
        label: qsTr("文件里保留什么")
        points: [
            qsTr("格式标识VisionCraft.ScopeFrame与版本1；原始12位ADC码数组、点数、实际采样率、采集编号、发生器设定频率、VDDA快照及原回传耗时。来源明确区分模拟器与实板通道，不把回放伪装成新采样。"),
            qsTr("receivedAtUtc是电脑收到完整帧的UTC时间，savedAtUtc是写文件的UTC时间。二者都不是ADC第一个样本的硬件时间，也不借用未校准的板上RTC。重新保存回放时保留原接收时间和来源，只更新保存时间。"),
            qsTr("读取限制128 KiB；只接受本地文件、已知版本、32–2048点、10–200 kS/s实际采样率，ADC码必须是0–4095整数。点数与数组长度必须一致，时间及元数据也要通过检查。最小/最大/均值及过零频率由样本重新计算，频谱继续使用原有OpenCV DFT，不信任文件额外附带的统计值。")
        ]
    }
    Figure {
        files: ["handbook/system/data/scope-replay.png"]
        captions: [qsTr("USB_232实板采样保存后，断开连接离线回放；同一帧已归档为pa4-usb.vcscope.json")]
    }
    Para {
        text: qsTr("归档实板例子为PA4的2048点、实际50 kS/s，原始ADC码404–3704。原回传含采集约485.95 ms；过零估计约1000.5 Hz，频谱主峰栅格约1001.0 Hz，栅格间隔24.414 Hz。测试在Qt原生控件内保存、打开、暂停连续采集；随后到达的新实时帧和断连均未覆盖回放，缩放也没有改变频谱。这些是单帧结果，不是长期采样或精密频率校准。")
    }
    CodeRef { file: "handbook/system/data/pa4-usb.vcscope.json"; match: "format|version|count|rate|receivedAtUtc|source"; language: "text"; caption: qsTr("归档实板文件的元数据，原始样本保留在同一JSON文件") }
    CodeRef { file: "src/app/ScopeArchive.cpp"; region: "save" }
    Pitfall {
        text: qsTr("QSaveFile先写临时文件，再提交到目标位置；写入失败不能留下半份正式记录。文件选择器允许用户确认覆盖已有文件。这个版本只保存单帧，不是连续录像、数据库或无限采样流；原回传耗时不能当作本次打开文件的耗时，ADC码也不等于校准电压。")
    }
    CodeRef { file: "src/app/ScopeArchive.cpp"; region: "load" }
    CodeRef { file: "src/device/ScopeFrame.h" }
    KeyPoints {
        label: qsTr("验证范围")
        points: [
            qsTr("文件层QtTest汇总22项通过：往返后样本/元数据/时域统计/频谱一致，坏版本、越界/非整数/非数值样本、长度不符、非法时间、截断与超大文件均被拒绝；加载失败保留有效回放，保存失败保留旧文件内容。归档实板JSON也可在无设备的测试进程中分析。"),
            qsTr("模拟器和USB_232实板均通过原生QML保存/打开流程，测试触发按钮clicked与对话框accepted回调；覆盖打开时停止连续采集、后续实时帧不覆盖文件、断连后保留回放及返回实时。系统原生文件选择器的逐键交互未自动测试。"),
            qsTr("仓库handbook/system/data/pa4-usb.vcscope.json可以直接在数据页打开；手册仅引用其元数据，完整原始样本保存在同一文件中。当前只保存单帧，不包括连续采样记录、板上SD写入或新的下位机任务。")
        ]
    }
    CodeRef { file: "handbook/system/data/scope-archive-test.txt"; match: "^Totals:|^100% tests passed"; caption: qsTr("文件层、桌面及独立实板窗口的原始测试汇总") }
    Try {
        task: qsTr("打开一份记录后把时间轴放大四倍，主峰频率栅格会更细吗？")
        answerNote: qsTr("不会。时域只改变当前可见区间，频谱仍分析整帧，栅格间隔为实际采样率除以总点数。离线文件也走同一个ScopeAnalysis，所以加载、暂停或缩放不应改变该结论。")
    }
    InSystem {
        text: qsTr("ScopeArchive只依赖Qt文件与数据类型，不认识DeviceLink或板子。ScopeFrame统一实时与文件帧的校验和时域统计，ScopeAnalysis负责频谱，ScopePanel选择当前显示来源；这让通信、文件和分析可以分别测试。原始日志仅作证据，使用说明和测量边界写在本节QML里。")
    }
}
