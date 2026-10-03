import QtQuick
import VisionCraft

Section {
    title: qsTr("检测管线")
    lead: qsTr("一张截图进去，一个判定出来。中间五步，每一步都用到 OpenCV 卷里讲过的一种方法，每一步都可能提前给出结论。这一节把它们串起来，再量一量整条管线花多少时间、时间花在哪。")

    Why {
        text: qsTr("OpenCV 卷是一个方法一个方法地讲，并且每节都回到工位检测里那一步。这里反过来，从检测代码的角度把它们按顺序摆在一起，看清楚数据怎么一步步变成结论。"
                 + "代码在 src/vision/Inspector.cpp，一个函数从头读到尾就是整条管线。")
    }

    KeyPoints {
        label: qsTr("五步和六个出口")
        points: [
            qsTr("① 分割：转灰度，5×5 高斯模糊，大津法二值化。——「高斯滤波」「全局阈值与 OTSU」"),
            qsTr("② 轮廓：RETR_CCOMP 找轮廓，第一层面积最大的是外边缘，它的子轮廓里最大的是内孔。找不到或太小：出口「未找到工件」。——「轮廓检索」"),
            qsTr("③ 量圆：外边缘先求凸包再拟合椭圆，内孔直接拟合，得到两个圆心和半径。——「外接圆与拟合椭圆」"),
            qsTr("④ 缺口和偏心：外轮廓的凸缺陷深度超过 5 像素：出口「缺口」；两个圆心距离超过 6 像素：出口「内孔偏心」。——「凸包与凹缺陷」"),
            qsTr("⑤ 表面：黑帽变换找比周围暗的小东西，阈值 18，和环面掩膜相与，连通域取最大一块；不到 12 像素：出口「合格」；否则按最小外接旋转矩形的长宽比分「划痕」和「污点」。——「腐蚀、膨胀、开闭运算」「位运算与掩膜」「连通域」")
        ]
    }

    CodeRef { file: "src/vision/Inspector.cpp"; match: "// ---- \\d|return finish\\(" ; caption: qsTr("代码里的步骤标题和全部出口（按出现顺序）") }

    Para { text: qsTr("每一步的中间结果都可以导出成图。一件划痕工件从输入到判定：") }
    Figure {
        files: ["handbook/opencv/figures/scratch-input.jpg", "handbook/opencv/figures/scratch-binary.png", "handbook/opencv/figures/scratch-blackhat-x4.png",
                "handbook/opencv/figures/scratch-suspect.png", "handbook/opencv/figures/scratch-result.jpg"]
        captions: [qsTr("输入"), qsTr("① 大津二值化"), qsTr("⑤ 黑帽（×4）"), qsTr("⑤ 超过阈值的像素"), qsTr("判定：划痕")]
    }
    Figure {
        files: ["handbook/opencv/figures/chip-result.jpg", "handbook/opencv/figures/offset-result.jpg", "handbook/opencv/figures/spot-result.jpg", "handbook/opencv/figures/good-result.jpg"]
        captions: [qsTr("缺口"), qsTr("内孔偏心"), qsTr("污点"), qsTr("合格")]
        columns: 4
    }

    Para { text: qsTr("1000 件工件（缺陷率 50%、难度 0.5）逐件计时：") }
    CodeRef { file: "examples/opencv/pipeline_timing/main.cpp"; region: "timing" }
    CodeRef { file: "examples/opencv/pipeline_timing/output.txt"; caption: qsTr("本机实测（Steam Deck，每次运行略有不同）") }
    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("一件 3.5 ms 左右，99% 在 5.5 ms 以内。工位每次检测都放进线程池（见 Qt 卷「QtConcurrent 与 QFuture」），界面不会因此卡顿；就算放在主线程，这点时间也不会被察觉。"),
            qsTr("在第 ④ 步就结束的缺口、偏心件只要 1.2~1.3 ms；走完第 ⑤ 步的合格、划痕、污点要 3.6~3.9 ms。表面检测一步就占了整条管线三分之二的时间——21×21 的黑帽是最贵的运算。"),
            qsTr("所以先做便宜的判断、后做贵的判断，是管线排序的一个好原则：能提前确定不合格的，就不必再看表面。")
        ]
    }

    Pitfall {
        text: qsTr("提前结束意味着一件工件只报一种缺陷。一件既有缺口又有划痕的垫圈，会在第 ④ 步判成「缺口」，划痕根本没被检查。"
                 + "对「放不放行」来说没有影响（都是不合格），但如果要统计「哪种缺陷最多」来追溯生产问题，这样的统计会偏向排在前面的缺陷类型。")
    }

    Try {
        task: qsTr("如果把第 ④ 步和第 ⑤ 步的顺序反过来（先查表面，再查缺口和偏心），整体的平均耗时会变多还是变少？各类判定结果会不会变？")
        answerNote: qsTr("平均耗时会增加：现在缺口、偏心件（约 27%）在 1.2 ms 就结束了，换了顺序它们也要先做 2.5 ms 左右的表面检测。"
                       + "判定结果在模拟数据上基本不变，因为每件只有一种缺陷——但缺口处露出的暗色传送带可能会被黑帽当成「比周围暗的东西」，"
                       + "这就要靠环面掩膜（离外边缘留 7 像素）挡住。这是个可以改了代码实测的问题。")
    }

    InSystem {
        text: qsTr("src/vision/Inspector.cpp（参数在 Inspector.h）；工位控制器 src/app/StationController.cpp 在线程池里调用它，结果下发给板子；评价方法见 OpenCV 卷「怎样评价一个检测算法」。")
    }
}
