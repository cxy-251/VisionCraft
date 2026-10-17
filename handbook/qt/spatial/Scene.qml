import QtQuick
import VisionCraft

Section {
    title: qsTr("三维场景：Item、Node与View3D")
    lead: qsTr("把三维对象放进QML，不等于给Rectangle加一个z。")
    Why { text: qsTr("二维Item参与布局和输入；三维Node描述空间层次，最终通过View3D成为一块二维画面。两套对象关系相连，却使用不同坐标。") }
    Para { text: qsTr("先读QML组件、布局和GPU渲染。独立示例example_spatial_lessons有六个教学模式；安装版提供这六节正文和代码，不自带Quick3D示例可执行文件。") }
    Para { text: qsTr("示例在View3D内创建相机、光源、父节点和一个薄立方体。#Cube是内置几何体，scale=(2,1.5,0.05)把边长100的立方体变成约200×150×5的薄板。这里约定场景单位为毫米。") }
    Para { text: qsTr("场景本身不决定它占据窗口多少空间，Layout分配View3D的二维尺寸；相机再决定哪些空间对象投影到这块区域。缩放窗口与移动相机是两种不同操作。") }
    KeyPoints { points: [qsTr("模型的source提供几何，materials决定外观，父节点组合变换。"), qsTr("普通QML控件仍放在二维布局中，负责参数输入和说明。"), qsTr("本例默认没有定时器、自动旋转或外部模型下载。")] }
    CodeRef { file: "examples/qml/spatial/Main.qml"; region: "scene"; caption: qsTr("在场景树中找到相机、光源和模型") }
    Pitfall { text: qsTr("场景出现黑色或空白时，要区分模型未在相机前方、裁剪、材质/光源、模块和图形后端问题；编译成功不能回答画面是否正确。") }
    Try { task: qsTr("把薄板的父节点移到相机后方，为什么二维布局仍然保留同样大的区域？"); answerNote: qsTr("布局管理View3D这个Item；模型是否在视锥内是空间渲染问题，二者并不共同决定布局尺寸。") }
    InSystem { text: qsTr("构建：VC_BUILD_EXAMPLES=ON、VC_BUILD_SPATIAL_EXAMPLE=ON，再构建example_spatial_lessons。需要与Qt版本匹配的Quick3D组件；程序独立运行，不连接设备。正文与源码在主应用内阅读。") }
}
