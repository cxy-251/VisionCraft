import QtQuick
import VisionCraft

Section {
    title: qsTr("单应性配准")
    lead: qsTr("有了几十对匹配点，就能反求「标签是怎样被斜拍进场景的」那个 3×3 变换。匹配点里有错的，直接用全部点求会被拉偏很远；RANSAC 反复用少量点试，找出大多数点都同意的那个答案。")

    Why {
        text: qsTr("「透视变换」一节是已知四个角求变换；实际中四个角往往要靠特征匹配找出来，而匹配不可能全对。示例接着上一节，用比值检验后的 87 对匹配求变换，比较用不用 RANSAC。")
    }

    CodeRef { file: "examples/opencv/features/main.cpp"; region: "homography" }
    CodeRef { file: "examples/opencv/features/output.txt"; from: "==== 3"; to: "内点里"; caption: qsTr("实际输出（最后一行按标准答案核对 RANSAC 的判断）") }
    Figure { files: ["handbook/opencv/figures/features-matches.jpg"]; captions: [qsTr("RANSAC 判为内点的匹配（绿线）：左边是标签，右边是场景")] }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("RANSAC 判为内点的 74 对，正好就是上一节按标准答案判为正确的 74 对——13 对错误匹配全被剔除了。"),
            qsTr("用求出的变换把标签的四个角映射到场景里，和真实位置最多差 3.25 像素。四个角离大部分特征点（集中在文字上）比较远，是外推，误差会放大。"),
            qsTr("不用 RANSAC，13 对错误匹配参与最小二乘，四个角最多差 24 像素，结果基本不能用。")
        ]
    }

    Pitfall {
        text: qsTr("RANSAC 的第四个参数 3.0 是「离模型多远还算内点」（像素）。太小，正确的点也因为一两个像素的误差被踢出去；太大，错误的点也混进来。"
                 + "它还需要内点占多数：如果错误匹配超过一半，RANSAC 也可能找到一个错误的模型——所以前面的比值检验仍然重要。")
    }

    Try {
        task: qsTr("知道了 H，怎样把场景里斜着的标签「拉正」成正面视图？用哪个函数、传哪个矩阵？")
        answerNote: qsTr("用 warpPerspective(scene, out, H.inv(), Size(260, 160))：H 把标签坐标映射到场景，拉正要反过来，所以传它的逆；或者传 H 本身再加 WARP_INVERSE_MAP 标志。"
                       + "本机用真实的变换实测，拉正后和原标签平均差 7.65——比「透视变换」一节的 2.80 大，因为这次场景里加了噪声，而且背景 70 和标签边缘之间的过渡被插值进了画面。")
    }

    InSystem {
        text: qsTr("工位检测没有用单应性配准（相机固定、正对传送带）。数据由 examples/opencv/features 测得。")
    }
}
