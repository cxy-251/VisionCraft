import QtQuick
import VisionCraft

Section {
    title: qsTr("空间变换：父坐标、局部坐标与顺序")
    lead: qsTr("同样的旋转和平移，顺序一变，物体走的轨迹也会变。")
    Why { text: qsTr("二维界面中的x/y已经是相对于父项的值。三维层次把同样的思想扩展成矩阵组合，需要分清在哪个坐标系里移动。") }
    Para { text: qsTr("选择“父子变换”，勾选“平移受父旋转影响”。模型在父坐标里沿x移到100，再由父节点绕Y旋转。点的世界位置可写为R_parent×(p_local+t_local)。") }
    Para { text: qsTr("若想让物体只绕自身中心转动、同时把整个物体平移到固定世界位置，需要换节点层次：外层平移，内层旋转，对应t_world+R_local×p_local。两个式子对原点的结果就已经不同。") }
    Para { text: qsTr("本例演示第一种层次。右侧数值投影没有为该额外局部平移建模，因此开启此模式时隐藏投影点；不能让来自另一套参数的点继续冒充同一场景。") }
    KeyPoints { points: [qsTr("先确定点是物体坐标、父坐标还是世界坐标，再乘矩阵。"), qsTr("矩阵相乘通常不可交换，改变层次会改变组合顺序。"), qsTr("scale也参与父子变换，非均匀缩放还会影响表面法线的变换。")] }
    CodeRef { file: "examples/qml/spatial/Main.qml"; region: "hierarchy"; caption: qsTr("模型相对于旋转父节点的局部平移") }
    Pitfall { text: qsTr("eulerRotation展示方便，但三个旋转角不能直接当成OpenCV的Rodrigues向量。本例只绕一个Y轴，转换关系才可以明确写成一个轴角。") }
    Try { task: qsTr("先令yaw=0，再令yaw=90，局部平移(100,0,0)在世界坐标中指向哪里？"); answerNote: qsTr("按右手Y轴旋转，分别指向世界+X和-Z。示例滑块范围止于70度，90度可通过修改源码计算；不能把这个题当成已在界面显示过的结果。") }
    InSystem { text: qsTr("构建：VC_BUILD_EXAMPLES=ON、VC_BUILD_SPATIAL_EXAMPLE=ON，再构建example_spatial_lessons。需要与Qt版本匹配的Quick3D组件；程序独立运行，不连接设备。正文与源码在主应用内阅读。") }
}
