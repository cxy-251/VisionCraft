import QtQuick
import VisionCraft

Section {
    id: section
    title: qsTr("同一个点：物体、世界、相机与图像")
    lead: qsTr("每一步都写出点属于哪个坐标系，再讨论它为什么移动。")
    Why { text: qsTr("坐标转换错误经常被误认为OpenCV算法错误。把中间值直接摆在一起，比只看最后图像更容易定位原因。") }
    Para { text: qsTr("本例物体点用毫米描述，先绕世界Y轴旋转，再减去相机在世界中的位置，最后用diag(1,-1,-1)转换Qt相机与OpenCV相机的方向。相机本身保持朝Qt局部-Z，不附加lookAt旋转。") }
    Para { text: qsTr("OpenCV相机采用X向右、Y向下、Z向前。内参以像素为单位，主点固定在((640−1)/2,(480−1)/2)=(319.5,239.5)。u=fx×Xc/Zc+cx，v=fy×Yc/Zc+cy。") }
    Para { text: qsTr("界面查看器再把640×480原图适应到控件区域。这个显示缩放不回写内参，也不改变下方表格里的原图像素坐标。系统原有图像交付课程解释这条显示边界。") }
    KeyPoints { points: [qsTr("长度与焦距分别使用mm和px，不能把两个数字直接相加。"), qsTr("本例使用零镜头畸变，画面坐标不是默认带真实镜头模型的结果。"), qsTr("相机后方与零深度在除法之前拒绝，画面外点另行标注。")] }
    LessonDemo { title: qsTr("坐标与投影"); pageActive: section.studyActive; activity: Component { ProjectionView { } } }
    CodeRef { file: "src/vision/teaching/ProjectionMath.cpp"; region: "coordinate-chain"; caption: qsTr("独立示例、三维场景与应用演示共用的数学约定") }
    CodeRef { file: "examples/integration/projection/output.txt"; match: "^point [04] "; caption: qsTr("两个深度层的实际数值输出") }
    Pitfall { text: qsTr("把世界点乘了外参后再加一次相机位置，会重复应用平移。相机在世界的pose与world_to_camera互为逆关系。") }
    Try { task: qsTr("相机向世界+X移动，物体固定，图像中的物体向哪边移动？"); answerNote: qsTr("本例相机不转动，Xc=Xworld−cameraX减小，所以u减小，物体向图像左侧移动。") }
    InSystem { text: qsTr("先读Qt/三维场景与OpenCV/标定、PnP和单应性。应用演示使用普通Qt图像查看器，不要求Quick3D；example_projection_lesson提供固定参数的数值输出。绿色为第一层，橙色为第二层；只判断画面范围，不做遮挡分析。") }
}
