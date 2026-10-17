import QtQuick
import VisionCraft

Section {
    title: qsTr("表面外观：材质、光照与透明度")
    lead: qsTr("颜色变化可能来自照明，而不是物体几何或图像算法。")
    Why { text: qsTr("三维渲染输出是几何、材质、光源和显示链共同作用的结果。教学时应一次只改变一种因素。") }
    Para { text: qsTr("选择“材质与光照”，保持相机和薄板位置不动，改变粗糙度。PrincipledMaterial用材质参数描述光照响应；这里的metalness固定为0.15，用来观察反射外观。") }
    Para { text: qsTr("勾选“棋盘纹理”把一个128×128的QML棋盘交给Texture.sourceItem，再作为baseColorMap。切换“不受光照影响”可以把基础颜色与受光照着色对照。这个开关适合教学对比，不代表实际物体失去了反射规律。") }
    Para { text: qsTr("透明度改变混合关系，但不自动提供物理正确的折射、吸收或相机曝光。透明表面的排序、遮挡和深度写入也不能按不透明表面简单类推。") }
    KeyPoints { points: [qsTr("几何体、相机与颜色结果各有不同的因果关系。"), qsTr("薄板厚度为5毫米，边缘和投影点所描述的中间平面存在差别。"), qsTr("本例不把渲染颜色用作光度标定真值。")] }
    CodeRef { file: "examples/qml/spatial/Main.qml"; region: "hierarchy"; caption: qsTr("同一个模型的材质参数") }
    Pitfall { text: qsTr("不要把GPU预览的一个RGB值直接当成物理辐照度。色彩空间、曝光、色调映射与显示还需要各自的定义。") }
    Try { task: qsTr("改变灯光后边缘位置没变、表面亮度改变，能否说几何投影公式出错？"); answerNote: qsTr("不能。几何决定位置，外观还受到材质和光照影响；应分别检查位置与颜色。") }
    InSystem { text: qsTr("构建：VC_BUILD_EXAMPLES=ON、VC_BUILD_SPATIAL_EXAMPLE=ON，再构建example_spatial_lessons。需要与Qt版本匹配的Quick3D组件；程序独立运行，不连接设备。正文与源码在主应用内阅读。") }
}
