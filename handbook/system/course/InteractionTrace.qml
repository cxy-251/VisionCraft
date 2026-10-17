import QtQuick
import VisionCraft
Section {
    title: qsTr("综合解剖：一次检测操作如何走完")
    lead: qsTr("沿现有工位代码追踪一次操作：输入动作、请求快照、异步取图、后台计算、结果归档、属性通知和下一件工件。每个箭头都有对应的代码。")
    Why { text: qsTr("前面的课程分别讲输入、绑定、图像交付与结果发布。本节把它们放回同一条真实调用链，区分谁发起请求、谁拥有数据，以及一步失败时谁负责收尾；不新增检测功能。") }
    CodeRef { file: "qml/station/StationPage.qml"; match: "onClicked: Station.inspectNow" }
    CodeRef { file: "src/app/StationController.cpp"; from: "^void StationController::inspectNow"; to: "^}" }
    KeyPoints {
        label: qsTr("① 动作成为有身份的请求")
        points: [
            qsTr("按钮调用inspectNow，不直接执行OpenCV。busy与记录会话的beginCapture先决定能否开始；不能因为按钮能点，就在后端允许多个请求重叠。"),
            qsTr("控制器保存当前工件真值、检测器参数、时间与来源，再发出带captureId的grabRequested。后续不能用已经改变的配方或下一件工件真值解释这次输入。"),
            qsTr("取图不是同步返回值。QML收到信号后延迟到事件循环，再从独立完整传送带图层获取480×360图像；查看器的缩放、工具栏不是算法输入。")
        ]
    }
    CodeRef { file: "qml/station/StationPage.qml"; region: "grab" }
    CodeRef { file: "src/app/StationController.cpp"; from: "^void StationController::inspectGrab"; to: "^}" }
    KeyPoints {
        label: qsTr("② 回调交付输入，后台交付结果")
        points: [
            qsTr("inspectGrab先检查请求仍有效、编号匹配、图像非空且尺寸正确。迟到或重复回调不会启动第二次检测；有效回调之后停止取图超时计时。"),
            qsTr("输入QImage留给记录，转换后的Mat和检测器快照按值交给工作线程。线程里运行Inspector，不读按钮或滑条。Mat的数据不再由界面并发修改，是这次按值交付安全的前提。"),
            qsTr("完成时onInspected使用先前保存的真值和配方，更新计数、混淆统计、标注图与记录。它采用这次请求的快照语义，不是把当前配方重新贴到旧结果上。")
        ]
    }
    CodeRef { file: "src/app/StationController.cpp"; from: "    InspectionSession::Record record;"; to: "    emit statsChanged" }
    KeyPoints {
        label: qsTr("③ 采用结果后，界面和设备各走自己的出口")
        points: [
            qsTr("resultChanged与statsChanged通知QML重新读取。ImageView负责绘制保存的QImage，显示布局和图像缩放仍只是视图职责。"),
            qsTr("连接DeviceLink时才发送判定与必要的图像；未连接时，本地算法、统计和记录仍可以完成。下位机不是执行OpenCV的地方。"),
            qsTr("清除busy后才nextPart，避免把下一件工件和本次在途请求混在一起。自动模式再启动间隔定时器，不是在工作线程里递归驱动界面。")
        ]
    }
    CodeRef { file: "src/app/StationController.cpp"; from: "    if [(]m_link && m_link->state"; to: "^}" }
    Pitfall { text: qsTr("不同控制器可以有不同发布策略：工坊参数变化后拒绝旧版本作为当前预览；工位则记录一次已接受请求的原始参数与答案。共同要求是归属明确，不能机械地说所有旧快照都应丢弃。") }
    Try { task: qsTr("取图回调迟到、后台处理失败、设备未连接，三者是否都应该记成一次NG？"); answerNote: qsTr("不是。迟到回调被编号与状态检查拒绝，后台失败走错误收尾而非生成有效判定，设备未连接只是没有远端发送出口。NG是算法对有效输入的判定，不能把基础设施错误混进缺陷统计。") }
    InSystem { text: qsTr("本节只沿现有源码阅读，不运行工位取图或操作开发板。结合三节Qt/OpenCV协作课，可逐个说明请求ID、配方快照、内存所有权、线程归属与通知链。") }
}
