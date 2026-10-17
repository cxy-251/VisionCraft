import QtQuick
import VisionCraft

Section {
    title: qsTr("相机与投影：视野、距离与裁剪")
    lead: qsTr("透视改变远近尺度，正交保留平行投影的尺度关系。")
    Why { text: qsTr("图像里物体变大可能来自焦距增加，也可能来自物体靠近。投影模型把这些变化分开表达。") }
    Para { text: qsTr("示例透视相机位于(cameraX,0,distance)，保持朝局部-Z，不额外lookAt物体。横移相机因此会把物体推向画面一侧，而不是自动把它重新放回中心。") }
    Para { text: qsTr("固定参考图像为640×480。垂直视野由fovY=2×atan(240/focalPx)计算。改变实际View3D的宽高比会改变水平视野，右侧320×240的小面板则始终表示同一张640×480图。") }
    Para { text: qsTr("clipNear/clipFar按相机距离限制可渲染范围。正交模式使用独立相机：距离变化不产生透视缩小，示例同时隐藏右侧透视点，防止混用两个模型。") }
    KeyPoints { points: [qsTr("更大的focalPx对应更小的视野。"), qsTr("相机在世界中的位置，不等于OpenCV外参里的t向量。"), qsTr("投影在画面内部也不代表没有被别的物体遮挡。")] }
    CodeRef { file: "examples/qml/spatial/Main.qml"; region: "camera"; caption: qsTr("明确视野方向和裁剪范围") }
    Pitfall { text: qsTr("Qt场景相机朝-Z、上方向+Y；OpenCV相机采用Z向前、Y向下。坐标适配必须显式写出，不靠翻转最终图片补救。") }
    Try { task: qsTr("距离从500变成1000，焦距也从500变成1000，正对相机的平面尺寸会怎样？"); answerNote: qsTr("理想针孔模型中f/Z不变，平面相对主点的投影尺度保持；深度不同的点以及裁剪情况需要分别判断。") }
    InSystem { text: qsTr("构建：VC_BUILD_EXAMPLES=ON、VC_BUILD_SPATIAL_EXAMPLE=ON，再构建example_spatial_lessons。需要与Qt版本匹配的Quick3D组件；程序独立运行，不连接设备。正文与源码在主应用内阅读。") }
}
