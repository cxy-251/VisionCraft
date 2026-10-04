import QtQuick
import VisionCraft

Section {
    title: qsTr("图像金字塔")
    lead: qsTr("pyrDown 先模糊、再隔一个点取一个点，尺寸减半；反复做就得到一层层越来越小的「金字塔」。先模糊这一步不能省：直接隔点取，会凭空造出图里没有的图案。")

    Why {
        text: qsTr("在小图上找、在大图上精修，是很多算法加速的办法（模板匹配、光流、特征检测都这么做）。缩小图像看似简单，但做错了会出现「混叠」：太细的纹理在缩小后变成别的东西。"
                 + "模拟工件的传送带正好有一种周期约 18 行的横条纹，可以拿来试。")
    }

    CodeRef { file: "examples/opencv/basics2/main.cpp"; region: "pyramid" }
    CodeRef { file: "examples/opencv/basics2/output.txt"; from: "==== 3"; to: "细节层"; caption: qsTr("实际输出") }
    KeyPoints {
        points: [
            qsTr("pyrDown 一次，480×360 变成 240×180；pyrUp 再放大回来，尺寸对了，细节回不来，和原图平均差 2.56。"),
            qsTr("原图减去放大回来的图，就是这一层丢掉的细节（拉普拉斯金字塔的一层）。存下它，放大的图加上它就完全复原了（平均差 0.00）。图像压缩、图像融合用的就是这种分层表示。")
        ]
    }

    CodeRef { file: "examples/opencv/basics2/main.cpp"; region: "alias" }
    CodeRef { file: "examples/opencv/basics2/output.txt"; from: "缩到 1/16"; to: "pyrDown×4"; caption: qsTr("实际输出") }
    Pitfall {
        text: qsTr("缩到 1/16 后，每个点代表 16 行，而条纹一个周期只有 18 行——这么细的东西在小图里根本表示不出来，正确的结果应该是它们的平均值。"
                 + "pyrDown 四次得到的正是几乎平的一列 53。直接隔 16 行取一行却得到 47~60 之间的一个慢慢起伏的波，周期大约 9 个点（原图约 150 行）——原图里根本没有这么慢的起伏，它是取样取出来的假象，叫混叠。"
                 + "所以缩小前一定要先模糊：pyrDown 自带这一步；用 resize 缩小时选 INTER_AREA，它也是按区域平均。")
    }

    Try {
        task: qsTr("工位检测的参数是按 480×360 的图定的（比如最小缺陷面积 12 像素）。如果为了提速，先 pyrDown 一次再检测，哪些参数要改？改成多少？")
        answerNote: qsTr("长度类的参数减半：偏心阈值 6 → 3、缺口深度 5 → 2.5；面积类的参数变成四分之一：最小缺陷面积 12 → 3；长宽比不变。黑帽的结构元素 21×21 也要缩到约 11×11。"
                       + "但 3 个像素的缺陷已经很容易和噪声混在一起，淡缺陷的检出率预计会下降——提速的代价要用准确率测试来量，而不是只算参数。")
    }

    InSystem {
        text: qsTr("工位检测在原尺寸上做，没有用金字塔（一件 3.5 ms 已经足够快）。数据由 examples/opencv/basics2 测得。")
    }
}
