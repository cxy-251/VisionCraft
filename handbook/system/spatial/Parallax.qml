import QtQuick
import VisionCraft

Section {
    id: section
    title: qsTr("平面模型边界：两个深度层为何不能一起对齐")
    lead: qsTr("一张单应性矩阵能描述一个平面的视图关系，不能随意推广到任意深度。")
    Why { text: qsTr("多图融合中的局部重影，需要从几何来源理解。本课用两个有明确深度的平面，而不是仅靠一张处理后的图片猜测。") }
    Para { text: qsTr("点击“视差预设”：物体旋转为0，相机位于世界Z=500并向X移动80，焦距500像素；两层物体点分别在世界Z=0与Z=100，因此相机深度为500和400。") }
    Para { text: qsTr("相对于cameraX=0的原视图，水平移动引起像素变化Δu=−f×ΔC/Z。两层分别移动−80与−100像素。若用远平面的80像素平移补偿另一层，仍留下20像素残差。") }
    Para { text: qsTr("这是假设零旋转、相机平行移动的标量推导。实际转动或任意场景要重新使用完整投影，不把20像素公式套到所有姿态。示例即使旋转改变，仍把这个量明确标成“零旋转参考值”。") }
    KeyPoints { points: [qsTr("同一平面满足单应性；一般有平移且含不同深度的场景不满足同一平面模型。"), qsTr("纯相机旋转在理想针孔模型中是另一个可由单应性描述的特殊情况。"), qsTr("联合覆盖、边缘平滑和低像素差都不能单独证明几何模型正确。")] }
    LessonDemo { title: qsTr("坐标与投影"); pageActive: section.studyActive; activity: Component { ProjectionView { } } }
    CodeRef { file: "src/vision/teaching/ProjectionMath.cpp"; region: "depth-parallax"; caption: qsTr("独立示例、三维场景与应用演示共用的数学约定") }
    CodeRef { file: "examples/integration/projection/output.txt"; match: "^zero-yaw"; caption: qsTr("零旋转、横移80mm时的实际计算") }
    Figure { files: ["handbook/assets/projection/two-depths.png"]; captions: [qsTr("共享投影函数生成的两个深度层；不是GPU截图")] }
    Pitfall { text: qsTr("屏幕里两层边框的间距包含物体本身位置与深度缩放，不能直接当成两次相机运动之间的残差。20像素比较的是两幅视图的位移差。") }
    Try { task: qsTr("把相机横移改成0，残差参考值为0，能否据此断言两个平面处于同一深度？"); answerNote: qsTr("不能。这个运动没有提供区分深度的横向视差；零残差也可能来自观测条件，而非相同深度。") }
    InSystem { text: qsTr("先读Qt/三维场景与OpenCV/标定、PnP和单应性。应用演示使用普通Qt图像查看器，不要求Quick3D；example_projection_lesson提供固定参数的数值输出。绿色为第一层，橙色为第二层；只判断画面范围，不做遮挡分析。") }
}
