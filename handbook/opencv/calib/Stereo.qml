import QtQuick
import VisionCraft

Section {
    title: qsTr("双目深度")
    lead: qsTr("两台并排的相机拍同一个场景，同一个点在两张图里的水平位置差（视差）和它的距离成反比：距离 = 焦距 × 基线 ÷ 视差。近处视差大、测得准；远处视差小，差一个像素就差很多。")

    Why {
        text: qsTr("单个相机看不出远近，两个相机就可以，就像人的两只眼睛。示例造了一对理想的双目图像：随机纹理的平面，左半边离相机 600 毫米、右半边 1200 毫米，"
                 + "按公式算出每一列该有的视差把纹理挪过去，再用 SGBM 算法求视差、换算成距离。")
    }

    CodeRef { file: "examples/opencv/calib3d/main.cpp"; region: "stereo" }
    CodeRef { file: "examples/opencv/calib3d/output.txt"; from: "==== 3"; caption: qsTr("实际输出") }
    Figure { files: ["handbook/opencv/figures/stereo-disparity.png"]; captions: [qsTr("视差图：左边近（亮），右边远（暗）；最左侧的黑带宽 128 列：搜索范围（numDisparities = 128）伸到了图像外面，算不出视差")] }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("视差 80 和 40 像素都测准了，换算出的距离 600、1200 毫米完全正确。这是理想情况：两台相机完全平行、纹理丰富、没有噪声、视差正好是整数。"),
            qsTr("精度随距离变差：视差差 1 个像素，近处距离差 7.6 毫米，远处差 30.8 毫米。距离加倍，同样的视差误差带来约 4 倍的距离误差（距离误差和距离的平方成正比）。")
        ]
    }

    Pitfall {
        text: qsTr("SGBM 的输出是乘了 16 的定点数（CV_16S），要除以 16 才是像素；没算出来的点是负数，统计时要排除（示例用 disp > 0 做掩膜）。"
                 + "而且它要求两张图已经「校正」过：同一个点必须在两张图的同一行上。真实的两台相机不可能装得完全平行，要先用双目标定（stereoCalibrate）和校正（stereoRectify）把图变换好。")
    }

    Pitfall {
        text: qsTr("没有纹理的地方测不出视差：一面白墙在两张图里处处一样，不知道该和哪里对应。工位的垫圈表面是光滑均匀的金属，正是双目最难处理的那类表面；"
                 + "实际中会加一个投射随机光斑的投影仪，人为给它加上纹理（结构光）。")
    }

    Try {
        task: qsTr("如果想在 1200 毫米处也达到近处 7.6 毫米左右的精度，可以改哪些参数？各改多少？")
        answerNote: qsTr("距离误差 ≈ 距离² ÷ (焦距 × 基线) × 视差误差。距离加倍，误差变 4 倍；要抵消，焦距 × 基线要变成 4 倍：基线从 60 加到 240 毫米，或焦距加倍、基线也加倍。"
                       + "代价是基线越长，两台相机看到的画面差别越大，近处的物体可能只出现在一台相机里，匹配也更难。")
    }

    InSystem {
        text: qsTr("工位是单相机（截图），不需要深度。数据由 examples/opencv/calib3d 测得。")
    }
}
