import QtQuick
import VisionCraft

Section {
    title: qsTr("界面与场景联动：一份参数、两种表达")
    lead: qsTr("三维画面和数值投影只有共享同一套参数，才适合对读。")
    Why { text: qsTr("联动的价值是解释关系。两幅图看起来接近，不足以证明坐标、单位和投影模型相同。") }
    Para { text: qsTr("示例的旋转、距离、横移与焦距由四个滑块持有。场景绑定这些值，ProjectionBridge同时把它们交给共享ProjectionMath，生成右侧点列。") }
    Para { text: qsTr("数值链使用物体旋转、相机位置、显式坐标方向转换和cv::projectPoints。右侧只显示中间平面的四个点；左侧是有厚度、光照和不同视口比例的三维模型，不能拿两边窗口像素逐点比较。") }
    Para { text: qsTr("点击重置回到固定初始参数；不使用后台定时器轮询滑块，也不从GPU读回图片当作几何真值。应用内的系统投影演示会继续展示每个点在哪个坐标阶段变化。") }
    KeyPoints { points: [qsTr("状态归属明确，QML绑定负责传播变化。"), qsTr("模型不同的模式隐藏不适用的对照，而不是继续显示旧结果。"), qsTr("源码示例为选配目标，主安装包的阅读功能不要求Quick3D。")] }
    CodeRef { file: "src/vision/teaching/ProjectionMath.cpp"; region: "coordinate-chain"; caption: qsTr("共享数学代码中的坐标转换") }
    Pitfall { text: qsTr("本轮必要编译与实际交互是不同的证据。三维外观、拾取体验和GPU驱动表现需要在运行环境中观察，不由构建成功推导。") }
    Try { task: qsTr("为了让左右两幅图按像素对比，除了共享焦距与姿态，还需要统一什么？"); answerNote: qsTr("图像/视口尺寸、主点和像素中心、投影类型、物体几何、裁剪/畸变约定；若比较颜色，还要统一渲染与显示语义。") }
    InSystem { text: qsTr("构建：VC_BUILD_EXAMPLES=ON、VC_BUILD_SPATIAL_EXAMPLE=ON，再构建example_spatial_lessons。需要与Qt版本匹配的Quick3D组件；程序独立运行，不连接设备。正文与源码在主应用内阅读。") }
}
