import QtQuick
import VisionCraft

Section {
    title: qsTr("图像工坊：从Widgets到QML")
    lead: qsTr("实验室里的单图工坊已迁到QML：打开电脑图片、调整四种基础处理、比较原图和结果并导出。旧Widgets入口继续保留，两种界面调用同一份ImageOperations，没有另写一套算法。")
    Why {
        text: qsTr("旧ImageProcessorPage已提供灰度、高斯、固定阈值和Canny，迁移的重点是把界面、后台任务与运算职责分开。ImageOperations处理Mat，ImageWorkshop管理文件、原图和参数快照，ImageWorkshopPage负责交互，ImageViewer负责图片查看。算法由新旧页面共同复用。")
    }
    KeyPoints {
        label: qsTr("在实验室使用")
        points: [
            qsTr("点击打开图片，或使用固定种子的划痕工件示例。屏幕取图也保留为明确的手动按钮，不在进入页面时自动执行；桌面未返回图像时显示原因，可以继续使用本地图片。无需连接开发板。"),
            qsTr("选择灰度、高斯、固定阈值或Canny。自动预览默认开启，短时间连续调整参数以180 ms窗口合并；关闭后点击执行处理。高斯核直接输入奇数边长1–51，1表示不模糊；不沿用旧滑条除以10再转奇数的界面映射。"),
            qsTr("每次均从原图计算，不叠加上一次结果。原图与结果分别显示，可缩放、平移、查看原始像素；结果标题保留实际使用的参数和算法耗时，不把图片转换或文件时间算进算法耗时。"),
            qsTr("选择PNG或JPEG，再导出当前结果。PNG保留无损结果，JPEG会引入有损压缩。文件保存的是完整处理结果，不包含工具栏或当前缩放视口；旧版工具按钮仍可打开原Widgets工坊及其余工具。")
        ]
    }
    CodeRef { file: "src/vision/ImageOperations.h" }
    CodeRef { file: "src/vision/ImageOperations.cpp"; region: "operations" }
    KeyPoints {
        label: qsTr("后台任务与结果归属")
        points: [
            qsTr("打开文件、执行算法和编码导出均放在线程池；每个工坊控制器最多执行一个任务，不为每次拖动新增线程。原图及参数按值捕获，工作线程不读取QML控件，不操作设备。"),
            qsTr("原图或参数变化会增加版本号。后台结果只有匹配当前版本才发布；旧参数结果可继续供查看，但明确标成上次结果，不能作为当前结果导出。新图片读失败保留上一张有效图片；清空图像后，旧任务也不能重新填回画面。"),
            qsTr("查看器和运算状态分别保存：缩放不会重跑算法，平移不会改变图像。自动预览只合并参数请求，不表示实时视频或固定帧率，也不修改Qt全局线程池大小。")
        ]
    }
    CodeRef { file: "src/app/ImageWorkshop.cpp"; region: "job" }
    CodeRef { file: "src/app/ImageWorkshop.cpp"; region: "process" }
    Para {
        text: qsTr("图片只从电脑本地文件读取，使用Qt文件路径，支持中文路径；读取时应用图片方向信息，并转成8位RGB原图。单文件限制128 MiB、最多24×1024×1024像素，先检查尺寸再解码，避免在界面线程展开过大的图片。视觉适配层再转换为BGR供OpenCV处理；灰度、阈值和Canny输出单通道，高斯保留彩色通道。")
    }
    CodeRef { file: "src/app/ImageWorkshop.cpp"; region: "open" }
    Pitfall {
        text: qsTr("固定阈值与自适应阈值不是同一种算法。这里的二值化按用户设置的一个灰度阈值分割，没有使用OTSU或局部阈值；相应方法已有独立手册与示例。工坊处理结果也不等于工位的缺陷判定，不应给任意本地图片附上模拟工件的标准答案。")
    }
    Para {
        text: qsTr("导出先在后台编码，再由QSaveFile提交到目标文件，失败时显示原因。文件类型由实际扩展名决定，支持.png、.jpg、.jpeg。这一版导出的是结果图片，没有把参数工程或处理链存档，也不混用工位的.vcinspect.json或波形的.vcscope.json。")
    }
    CodeRef { file: "src/app/ImageWorkshop.cpp"; region: "save" }
    Try {
        task: qsTr("把Canny的结果放大到400%，再导出PNG，导出的宽高会变成原图的四倍吗？")
        answerNote: qsTr("不会。查看器只做显示变换；控制器导出的是算法产生的完整结果图，宽高仍与输入一致。要改变图像实际尺寸，需要单独执行缩放运算，而不是操作查看器。")
    }
    InSystem {
        text: qsTr("入口在实验室的QML图像工坊；保留旧版工具按钮。源码分别位于src/vision/ImageOperations、src/app/ImageWorkshop、qml/workshop/ImageWorkshopPage.qml与qml/data/ImageViewer.qml。设备通信和下位机均不参与这条单图处理路径。")
    }
}
