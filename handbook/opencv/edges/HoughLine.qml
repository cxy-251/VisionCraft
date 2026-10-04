import QtQuick
import VisionCraft

Section {
    title: qsTr("霍夫直线")
    lead: qsTr("和霍夫圆同样的投票思路：每个边缘点给经过它的所有直线投票。HoughLines 返回无限长的直线（用 ρ、θ 表示），HoughLinesP 返回带端点的线段。")

    Why {
        text: qsTr("找边缘、找导轨、校正图像的倾斜角，都要从一堆边缘点里找出直线。示例画三条已知的线——水平、竖直、斜的——看两种函数各返回什么。")
    }

    CodeRef { file: "examples/opencv/geometry/main.cpp"; region: "hough-lines" }
    CodeRef { file: "examples/opencv/geometry/output.txt"; from: "==== 2"; to: "\\(20,180\\)"; caption: qsTr("实际输出") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("HoughLines 用 (ρ, θ) 表示直线：θ 是直线的法线方向，ρ 是原点到直线的距离。水平线 y = 30：法线竖直向下，θ = 90°，ρ = 30。竖直线 x = 150：θ = 0°，ρ = 150。"),
            qsTr("斜线从 (20,180) 到 (200,40)，方向约 −37.9°，法线方向 52.1°，报告的 θ = 52° 是 1° 一格量化后的结果。"),
            qsTr("HoughLinesP 直接给出端点，和画线时用的坐标完全一致（竖直线的两个端点顺序反了，方向本来就不重要）。多数时候 HoughLinesP 更好用。")
        ]
    }

    Pitfall {
        text: qsTr("HoughLines 的票数阈值是「线上至少多少个边缘点」，和线的长度直接相关：短线永远达不到阈值。示例的阈值 100 正好让三条线（长度 140~260 像素）都通过。"
                 + "图里有长有短的线时，HoughLinesP 的 minLineLength（最短线段长度）和 maxLineGap（允许断开的最大间隔）比单一票数阈值更好控制。")
    }

    Try {
        task: qsTr("把斜线改成从 (20,180) 到 (60,150)（长度 50），HoughLines 阈值 100 还找得到它吗？HoughLinesP 呢？")
        answerNote: qsTr("本机实测：两个函数都只找到了水平线和竖直线。原因要数点：这条线虽然长 50，但画成 1 像素宽的线只有 41 个像素（斜线每走一步 x 和 y 一起变，点数 = 横、纵跨度中较大的那个 + 1）。"
                       + "41 个点凑不够 HoughLines 的 100 票，也凑不够 HoughLinesP 的 50 票；把 HoughLinesP 的阈值降到 40，就找到了。票数阈值要按「点数」想，不是按「长度」想。")
    }

    InSystem {
        text: qsTr("工位检测没有用霍夫直线（垫圈只有圆）。数据由 examples/opencv/geometry 测得。")
    }
}
