import QtQuick
import VisionCraft

Section {
    title: qsTr("凸包与凹缺陷")
    lead: qsTr("凸包和轮廓之间的空隙就是「凹进去的地方」。convexityDefects 找出每一处空隙最深的点有多深——工位就用它判断垫圈边上有没有缺口。")

    Why {
        text: qsTr("完美的圆是凸的，凸包和轮廓重合，没有空隙。边上被磕掉一块，轮廓就凹进去，凸包从缺口上方跨过，两者之间出现一个空隙，空隙的深度大约就是缺口的深度。"
                 + "这一节用上一节同样的示例量凸缺陷深度，再看一个检测算法没有考虑到的情况。")
    }

    CodeRef { file: "examples/opencv/fit_hull/main.cpp"; region: "defects" }
    KeyPoints {
        points: [
            qsTr("convexHull 第三个参数 returnPoints = false 时返回的是凸包点在轮廓数组里的下标，convexityDefects 要的就是下标。"),
            qsTr("每个缺陷 4 个数：空隙的起点、终点（都是凸包上的点）、空隙里离凸包最远的点、以及那个最远距离。距离是定点数，乘了 256，要除回去。")
        ]
    }
    CodeRef { file: "examples/opencv/fit_hull/output.txt"; to: "40  "; caption: qsTr("实际输出（最后一列）") }
    Para {
        text: qsTr("没有缺口时最深 0.9 像素——圆被画成像素后边缘有锯齿，这点凹凸免不了。挖 10、20、30、40 像素，量到 8、18、28、38：比挖的深度少 2 像素，"
                 + "因为缺口是平底的矩形，而凸包从圆弧上跨过去时，弦比圆弧本身稍低一点。工位的阈值是 5 像素，远高于锯齿的 0.9。")
    }
    CodeRef { file: "src/vision/Inspector.cpp"; region: "chip" }

    Pitfall {
        text: qsTr("凹缺陷只认「凹」，可是一根向外凸的毛刺也会制造凹陷：凸包从毛刺尖往两边拉直线，毛刺两侧的圆弧就落在凸包里面，成了两个空隙。"
                 + "示例里 8 像素和 16 像素的毛刺，最深凸缺陷分别是 8.0 和 15.5，都超过了 5。用真正的检测代码验证：一件合格品，外圆上加一根 10 像素的小毛刺：")
        CodeRef { file: "examples/opencv/fit_hull/output.txt"; from: "毛刺"; caption: qsTr("示例输出") }
        CodeRef { file: "examples/opencv/inspect_steps/main.cpp"; region: "burr" }
        CodeRef { file: "examples/opencv/inspect_steps/output.txt"; match: "^burr"; caption: qsTr("实际输出") }
        Figure {
            files: ["handbook/opencv/figures/burr-result.jpg"]
            captions: [qsTr("判成「缺口」，红圈标在毛刺旁边")]
        }
        Para {
            text: qsTr("判成了 NG，从「这件该不该放行」看没错——毛刺也是缺陷；但缺陷类型报错了。准确率测试发现不了这个问题，因为模拟产线从来不生成毛刺。"
                     + "这是评价算法时要记住的：测试只能证明算法在测过的情况下对。要区分缺口和毛刺，可以看最深点在凸包的哪一侧，或者比较缺陷点到圆心的距离和半径。")
        }
    }

    Try {
        task: qsTr("怎样不改检测阈值，就把毛刺和缺口区分开？提示：缺陷最深点到外圆圆心的距离，和外圆半径比一比。")
        answerNote: qsTr("缺口的最深点在圆里面，离圆心比半径小（缺口越深越小）；毛刺造成的凹陷，最深点是毛刺旁边那段正常的圆弧，离圆心约等于半径。"
                       + "所以「最深点到圆心的距离 < 半径 − 几个像素」才算缺口，否则可能是毛刺或别的外凸物。这需要先求出外圆（上一节），再判缺口——顺序和现在的代码一致，只多一个比较。")
    }

    InSystem {
        text: qsTr("src/vision/Inspector.cpp 第 4 步；阈值 minChipDepth 在 src/vision/Inspector.h。毛刺的例子在 examples/opencv/inspect_steps，算法目前没有改，留作练习。")
    }
}
