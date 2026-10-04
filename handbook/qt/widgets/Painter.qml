import QtQuick
import VisionCraft

Section {
    title: qsTr("QPainter 绘图与双缓冲")
    lead: qsTr("QPainter 往任何「画布」上画：控件、QImage、QPixmap、打印机都可以。画到 QImage 上的好处是能逐个像素核对画得对不对。")

    Why {
        text: qsTr("仪表盘、波形、检测结果的框和标注，标准控件里都没有，要自己画。"
                 + "示例全部画在 QImage 上，再读回像素：抗锯齿到底改了哪些像素、一像素宽的线落在哪一行、旋转之后东西画到了哪里，都用数字说话。")
    }

    CodeRef { file: "examples/qt/painter/main.cpp"; region: "aa" }
    CodeRef { file: "examples/qt/painter/output.txt"; from: "==== 1"; to: "97 种灰度"; caption: qsTr("实际输出") }
    Figure {
        files: ["handbook/qt/figures/painter-aa-off.png", "handbook/qt/figures/painter-aa-on.png"]
        captions: [qsTr("关闭抗锯齿（放大 8 倍）"), qsTr("打开抗锯齿（放大 8 倍）")]
    }

    Para {
        text: qsTr("不开抗锯齿时，每一列只涂一个像素，全是纯黑，81 个像素（线横跨 x=10 到 90）；斜线看起来是台阶。"
                 + "打开后，线经过的像素按被覆盖的比例涂成不同深浅的灰，改动的像素翻倍到 163 个，用了 97 种灰度。边缘看起来平滑，代价是线变「虚」了一点。")
    }

    CodeRef { file: "examples/qt/painter/main.cpp"; region: "half-pixel" }
    CodeRef { file: "examples/qt/painter/output.txt"; from: "==== 2"; to: "y=20.5"; caption: qsTr("实际输出：第 20 列 y=18–22 五个像素的灰度") }

    Pitfall {
        text: qsTr("开着抗锯齿，在整数坐标 y=20 上画一像素宽的线，结果是 y=19、y=20 两行各一半灰（128、127），线又淡又粗。"
                 + "QPainter 的坐标是像素之间的网格线，像素 20 的中心在 20.5。一像素宽的线以 y=20 为中心，正好一半压在第 19 行、一半压在第 20 行。"
                 + "画在 y=20.5 上才是一整行纯黑（0）。画网格、表格线这种要清晰的细线，要么坐标加 0.5，要么对这些线关掉抗锯齿。")
    }

    CodeRef { file: "examples/qt/painter/main.cpp"; region: "transform" }
    CodeRef { file: "examples/qt/painter/output.txt"; from: "==== 3"; to: "蓝色方块左上角"; caption: qsTr("实际输出") }

    KeyPoints {
        label: qsTr("坐标变换")
        points: [
            qsTr("translate、rotate、scale 改的是之后所有绘制的坐标系，而不是已经画好的东西。"),
            qsTr("先平移到 (100,100) 再转 90°：局部 x 轴指向下方。局部的 (40,−2)–(44,2) 方块落到了 (98,140)–(102,144)，也就是中心正下方 40 像素处。rotate 的正方向是顺时针，因为屏幕坐标 y 轴向下。"),
            qsTr("resetTransform() 之后同样的矩形落在 (40,0)，和变换前的坐标一致。"),
            qsTr("变换会累积。画一组旋转的刻度时，每一格用 save() 保存、rotate、画、restore() 恢复，下面仪表盘的代码就是这样写的；否则每次旋转会叠加在上一次的结果上。")
        ]
    }

    CodeRef { file: "examples/qt/painter/main.cpp"; region: "gauge" }
    Figure {
        files: ["handbook/qt/figures/painter-dial.png"]
        captions: [qsTr("温度表，指针在 63")]
    }

    CodeRef { file: "examples/qt/painter/main.cpp"; region: "redraw-all"; caption: qsTr("每一帧都把表盘和指针全部重画") }
    CodeRef { file: "examples/qt/painter/main.cpp"; region: "cached"; caption: qsTr("表盘只画一次，存进 QPixmap；每帧贴上去再画指针") }
    CodeRef { file: "examples/qt/painter/output.txt"; from: "==== 4"; to: "缓存表盘"; caption: qsTr("实际输出：平均每帧耗时") }

    Para {
        text: qsTr("几次运行，全部重画 0.33–0.49 ms，贴缓存 0.031–0.052 ms，快了约 10 倍。51 条抗锯齿刻度加一个抗锯齿圆，比贴一张 300×300 的图贵得多。"
                 + "这就是常说的「双缓冲」在自绘控件里的用法：把不常变的部分画进一张离屏图，变了（比如窗口大小变了）才重画它。")
    }

    CodeRef { file: "examples/qt/painter/main.cpp"; region: "widget-buffer" }
    CodeRef { file: "examples/qt/painter/output.txt"; from: "==== 5"; caption: qsTr("实际输出（offscreen）") }

    Pitfall {
        text: qsTr("老教程里「为了防闪烁要自己做双缓冲」，说的是另一件事。Qt 的每个顶层窗口都有一个后台缓冲区（backingStore，示例里大小正是窗口的 200×100），"
                 + "paintEvent 里画的东西先进这块缓冲区，画完一起送到屏幕，用户看不到画了一半的画面。所以在 Qt 里自己再包一层「先画到 QPixmap 再整体画到控件」只为防闪烁，没有用；"
                 + "值得做的是上面那种「缓存不变的部分」，目的是省时间。")
    }

    Try {
        task: qsTr("把第 1 步那条线的终点从 (90,37) 改成 (90,10)，变成一条水平线。抗锯齿打开和关闭时，改动的像素数和灰度种数分别是多少？")
        answerNote: qsTr("实测：关闭时 81 个像素、1 种灰度；打开时 164 个像素、4 种灰度。线落在 y=9、y=10 两行之间，和第 2 步是同一个原因，所以行数翻倍；"
                       + "像素是 82 列而不是 81 列，是因为 QPen 默认的线端是方头（SquareCap），两端各多出半个线宽，盖住 x=9 和 x=90 各一半，于是又多出两种更浅的灰。"
                       + "把 y 改成 10.5 再试：82 个像素、2 种灰度——中间 80 个纯黑，两端各半个。")
    }

    InSystem {
        text: qsTr("本程序的界面是 QML，自绘的地方有两种做法。src/app/ImageView.cpp 继承 QQuickPaintedItem，在 paint() 里用的就是本节的 QPainter（drawImage 显示检测图像）；"
                 + "「数据」页的曲线 qml/data/TimeChart.qml 用的是 QML 的 Canvas，接口和浏览器的 canvas 一样，每秒和新数据到达时 requestPaint() 重画。")
    }
}
