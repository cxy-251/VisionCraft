import QtQuick
import VisionCraft

Section {
    title: qsTr("三维交互：拾取结果与拖动约束")
    lead: qsTr("鼠标落在二维视口上，需要通过相机才能找到三维对象。")
    Why { text: qsTr("屏幕像素本身不携带唯一的深度。选中对象与决定如何拖动对象，是两个不同的步骤。") }
    Para { text: qsTr("示例把plate设为pickable。按下时通过View3D.pick获得命中的对象，只有命中薄板才开始改变yaw，选中后材质颜色提供反馈。") }
    Para { text: qsTr("这里明确把水平拖动解释为绕Y轴旋转，每像素0.25度，并在滑块范围内截断。它是一种交互映射，不是在估计真实世界里的毫米位移。") }
    Para { text: qsTr("如果要让模型跟着鼠标平移，应先选择约束平面，再把相机射线与平面求交；仅知道鼠标x/y不能确定三维点。示例没有把这一高级操作伪装成已经实现。") }
    KeyPoints { points: [qsTr("拾取用View3D本地坐标，不能直接传整个窗口的鼠标坐标。"), qsTr("拖动起点保存原始角度，避免每次移动都重复累计完整位移。"), qsTr("未命中时清除选择；旋转动作与普通控件输入分别处理。")] }
    CodeRef { file: "examples/qml/spatial/Main.qml"; region: "picking"; caption: qsTr("由命中对象进入受约束的旋转操作") }
    Pitfall { text: qsTr("不要在每次positionChanged里把累计位移再加到上一次角度，否则拖动量会越积越大。") }
    Try { task: qsTr("为什么本例使用dragYaw+(当前x−起点x)×0.25，而不是yaw+(当前x−起点x)×0.25？"); answerNote: qsTr("dragYaw是固定的起始角度，当前位移也是相对起点的总位移；如果用持续变化的yaw会重复累加。") }
    InSystem { text: qsTr("构建：VC_BUILD_EXAMPLES=ON、VC_BUILD_SPATIAL_EXAMPLE=ON，再构建example_spatial_lessons。需要与Qt版本匹配的Quick3D组件；程序独立运行，不连接设备。正文与源码在主应用内阅读。") }
}
