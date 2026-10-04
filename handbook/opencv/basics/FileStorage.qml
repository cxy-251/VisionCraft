import QtQuick
import VisionCraft

Section {
    title: qsTr("FileStorage 持久化")
    lead: qsTr("cv::FileStorage 把数字、字符串、矩阵存成 YAML、XML 或 JSON 文本，再读回来。它最大的坑是：读一个不存在的键，不报错，得到 0。")

    Why {
        text: qsTr("检测参数、相机标定结果、模板这些东西要存盘。OpenCV 自带的 FileStorage 直接认识 cv::Mat，存矩阵最方便。"
                 + "示例把三个检测参数和一个矩阵写进 YAML，打印文件内容，再读回来——包括读一个文件里没有的键。")
    }

    CodeRef { file: "examples/opencv/basics2/main.cpp"; region: "storage" }
    CodeRef { file: "examples/opencv/basics2/output.txt"; from: "==== 5"; caption: qsTr("实际输出（中间是生成的 YAML 文件）") }

    KeyPoints {
        points: [
            qsTr("写：fs << \"键\" << 值，依次写下去。文件名的扩展名决定格式：.yml 是 YAML，.xml 是 XML，.json 是 JSON。"),
            qsTr("矩阵存成 !!opencv-matrix，带着行数、列数、元素类型（f 是 float），读回来就是同样的 Mat。"),
            qsTr("double 6.0 写成「6.」——这是 OpenCV 的写法，读回来没有问题，但用别的工具读这个 YAML 时要注意。")
        ]
    }

    Pitfall {
        text: qsTr("minDefectArea 根本不在文件里，读出来却是 0，没有任何报错。如果这是检测阈值，0 意味着「任何大小都算缺陷」，所有工件都会被判不合格。"
                 + "读之前用 fs[\"键\"].isNone() 检查，或者先给变量设好默认值、只有键存在时才覆盖。参数文件版本升级（新增了参数）时，旧文件里一定缺新键，这个坑很常见。")
    }

    Try {
        task: qsTr("本项目的检测参数存在哪里、怎么防止「读到不存在的值」？对比一下和 FileStorage 的做法。")
        answerNote: qsTr("检测配方存在板子的 EEPROM 里（F407 卷「EEPROM」一节），前面有 'V' 'C' 标识和版本号，后面有 CRC 校验，三样都对上才使用，否则用默认值。"
                       + "这比「缺了就是 0」安全得多。用 FileStorage 时也可以照做：文件里存一个 version 键，读取时先检查版本，缺键时保留默认值。")
    }

    InSystem {
        text: qsTr("工位没有用 FileStorage：检测参数作为配方存在板子上，界面设置用 QSettings（Qt 卷「QSettings 配置与配方」）。数据由 examples/opencv/basics2 测得。")
    }
}
