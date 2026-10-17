import QtQuick
import VisionCraft

Section {
    id: section
    title: qsTr("相机、物体与焦距：相对关系和投影尺度")
    lead: qsTr("看见物体变大，只能说明投影变化，不能直接断言物体靠近。")
    Why { text: qsTr("在针孔模型里，图像只观察相对位姿与内参共同产生的结果。很多不同操作都能造成相似的局部图像变化。") }
    Para { text: qsTr("保持物体朝向不变，减小相机距离，会增加f/Z，物体投影变大。增加焦距也会放大相对主点的像素距离，但两个操作对不同深度点的影响一般不相同。") }
    Para { text: qsTr("绕Y旋转物体时，同一块平面的不同角点获得不同深度，近的一侧和远的一侧出现透视差异。不要把旋转后的形状当成仅沿X压缩的一张二维图。") }
    Para { text: qsTr("已有calib3d示例根据已知投影做标定和PnP。本课只展示几何链和中间坐标，不重新做参数估计；已知真值、数值投影和从观测反求参数分别标注。") }
    KeyPoints { points: [qsTr("相机相对物体平移，与物体相对相机反向平移可以形成相同相对位姿。"), qsTr("没有额外尺度信息时，仅凭一张图不能恢复所有绝对尺度。"), qsTr("本例使用少量点即时更新，没有常驻计算线程或自动采集。")] }
    LessonDemo { title: qsTr("坐标与投影"); pageActive: section.studyActive; activity: Component { ProjectionView { } } }
    CodeRef { file: "src/vision/teaching/ProjectionMath.h"; region: "camera-contract"; caption: qsTr("独立示例、三维场景与应用演示共用的数学约定") }
    Pitfall { text: qsTr("把Rodrigues旋转向量的三个分量当成三个独立欧拉角，会在组合旋转时产生不同结果。本例只用单一Y轴旋转降低歧义。") }
    Try { task: qsTr("焦距与相机距离同时加倍，两个不同深度层会不会都保持原来的投影？"); answerNote: qsTr("一般不会。只有深度也严格按同一比例变化时f/Z才保持；固定世界中的第二深度层会有不同的分母变化。") }
    InSystem { text: qsTr("先读Qt/三维场景与OpenCV/标定、PnP和单应性。应用演示使用普通Qt图像查看器，不要求Quick3D；example_projection_lesson提供固定参数的数值输出。绿色为第一层，橙色为第二层；只判断画面范围，不做遮挡分析。") }
}
