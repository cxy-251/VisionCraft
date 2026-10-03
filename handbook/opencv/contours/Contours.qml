import QtQuick
import VisionCraft

Section {
    title: qsTr("轮廓检索")
    lead: qsTr("findContours 把二值图里每块白色区域的边界描成一串点。检索模式决定返回哪些边界、以及它们之间「谁套着谁」的关系怎么记。")

    Why {
        text: qsTr("工位检测的第 2 步要从二值图里找出垫圈的外边缘和内孔。光有「所有边界」不够，还得知道哪条是外边缘、哪条是它里面的孔——这靠层级（hierarchy）。"
                 + "示例画一个垫圈，孔里掉了一个小颗粒，旁边还有一个小方块，用四种检索模式各找一次，把层级数组打印出来。")
    }

    CodeRef { file: "examples/opencv/contours/main.cpp"; region: "shape" }
    CodeRef { file: "examples/opencv/contours/main.cpp"; region: "find" }

    KeyPoints {
        label: qsTr("层级数组怎么读")
        points: [
            qsTr("hierarchy[i] 是 4 个整数：[下一个同级轮廓, 上一个同级轮廓, 第一个子轮廓, 父轮廓]，都是轮廓的编号，没有就是 -1。"),
            qsTr("「父」= 套在外面的那条边界。父为 -1 的是最外层。顺着「下一个同级」可以遍历同一层的所有轮廓，顺着「第一个子」进入下一层。"),
            qsTr("白色区域的外边界和区域里的洞（黑色）的边界都是轮廓。洞的边界是那块白色区域外边界的子轮廓。")
        ]
    }

    CodeRef { file: "examples/opencv/contours/output.txt"; to: "^\\s*$"; caption: qsTr("实际输出（轮廓编号的顺序由 OpenCV 内部扫描顺序决定）") }

    KeyPoints {
        label: qsTr("四种模式的区别（面积：垫圈 25190，孔 5138，方块 841，颗粒 288）")
        points: [
            qsTr("RETR_EXTERNAL：只要最外层，2 个——方块和垫圈。孔和颗粒都不返回。只关心「有几个物体」时最快最省事。"),
            qsTr("RETR_LIST：全部 4 个，但不记关系，父、子全是 -1。"),
            qsTr("RETR_CCOMP：分成两层。第一层是所有白色区域的外边界（方块、垫圈，以及颗粒！），第二层是它们里面的洞。孔（5138）的父是垫圈（2）。"),
            qsTr("RETR_TREE：完整的嵌套关系：垫圈 → 孔 → 颗粒，颗粒的父是孔。")
        ]
    }

    Pitfall {
        text: qsTr("在 RETR_CCOMP 里，孔里的颗粒是最外层，不是孔的子轮廓。CCOMP 只有两层：「白色区域的外边界」和「洞的边界」，颗粒是一块独立的白色区域，所以算第一层，不管它在哪个洞里面。"
                 + "需要知道「颗粒在孔里」这种多层关系，要用 RETR_TREE。")
    }

    Figure {
        files: ["handbook/opencv/figures/contours-input.png", "handbook/opencv/figures/contours-ccomp.png"]
        captions: [qsTr("输入二值图"), qsTr("RETR_CCOMP：第一层绿色，第二层（洞）红色，数字是轮廓编号")]
    }

    Para {
        text: qsTr("工位检测用的是 RETR_CCOMP：在第一层里挑面积最大的当垫圈外边缘，再沿它的子轮廓链表找面积最大的当内孔。"
                 + "上面的结果正好说明这样做是稳的：就算孔里掉进了颗粒，颗粒也在第一层、面积小，不会被当成外边缘，也不会混进孔的候选里：")
    }
    CodeRef { file: "src/vision/Inspector.cpp"; from: "---- 2. 轮廓"; to: "return finish\\(VC_DEFECT_MISSING\\);" }
    CodeRef { file: "src/vision/Inspector.cpp"; from: "for \\(int child = hierarchy\\[outer\\]\\[2\\]"; to: "^    \\}" }

    Para {
        text: qsTr("最后一个参数决定轮廓点怎么存。CHAIN_APPROX_NONE 存边界上的每一个像素；CHAIN_APPROX_SIMPLE 把水平、竖直、45° 方向上的直线段压缩成两个端点：")
    }
    CodeRef { file: "examples/opencv/contours/main.cpp"; region: "approx" }
    CodeRef { file: "examples/opencv/contours/output.txt"; from: "轮廓点的存法"; caption: qsTr("实际输出") }
    Para {
        text: qsTr("方块从 116 个点压成 4 个角点，圆只从 508 压到 260（圆周上真正连成直线的段很短），面积完全不变。"
                 + "工位用 NONE：后面要拿轮廓点去拟合椭圆、找凸缺陷，点越完整越准。")
    }

    Pitfall {
        text: qsTr("contourArea 算的是边界围起来的面积，不扣除里面的洞，而且边界线取在像素中心上：垫圈外圆半径 90，πr² ≈ 25447，轮廓面积是 25190，差在边缘的半个像素。"
                 + "要算「真正有多少白色像素」，用 countNonZero 或连通域统计。")
    }

    Try {
        task: qsTr("如果工位改用 RETR_LIST，Inspector 里「只看最外层」的那一行判断 hierarchy[i][3] != -1 会怎样？还能找到内孔吗？")
        answerNote: qsTr("RETR_LIST 里所有轮廓的父都是 -1，那一行不再过滤任何轮廓，挑面积最大的仍然会挑到垫圈外边缘（运气好）。"
                       + "但找内孔要用 hierarchy[outer][2]（第一个子轮廓），在 LIST 里它是 -1，于是一个孔都找不到，函数返回「未找到工件」。本机把 RETR_CCOMP 临时改成 RETR_LIST 实测：五种工件（含合格品）全部判成「未找到工件」。层级信息是这个算法必需的。")
    }

    InSystem {
        text: qsTr("src/vision/Inspector.cpp 第 2 步：RETR_CCOMP + CHAIN_APPROX_NONE；外边缘和内孔交给后面的椭圆拟合、凸缺陷、偏心计算。")
    }
}
