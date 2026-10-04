import QtQuick
import VisionCraft

Section {
    title: qsTr("图形视图 QGraphicsView")
    lead: qsTr("场景（QGraphicsScene）里放图元，视图（QGraphicsView）是看场景的一个窗口，可以缩放、平移。和 Model / View 一样，数据和显示是分开的。")

    Why {
        text: qsTr("检测结果要叠在工件照片上显示：缺陷框、标签、测量线。用户还要放大看细节、点一下某个框看详情。"
                 + "如果全靠 QPainter 自己画，缩放之后「鼠标点在哪个框上」这种换算都要自己写。"
                 + "图形视图把这些都管了：图元有自己的坐标，场景负责查找，视图负责缩放和坐标换算。")
    }

    CodeRef { file: "examples/qt/graphicsview/main.cpp"; region: "scene" }
    CodeRef { file: "examples/qt/graphicsview/main.cpp"; region: "view" }
    CodeRef { file: "examples/qt/graphicsview/output.txt"; from: "==== 1"; to: "场景坐标 \\(520.0,158.0\\)"; caption: qsTr("实际输出") }

    KeyPoints {
        label: qsTr("三套坐标")
        points: [
            qsTr("图元坐标：每个图元自己的。缺陷框的矩形是 (0,0)–(60,40)，不管它放在哪里。"),
            qsTr("场景坐标：所有图元共用的「世界」。这里让它和照片的像素坐标一致，setPos(520, 180) 就是把框放在照片的 (520,180)。检测算法给出的缺陷位置可以直接用。"),
            qsTr("视图坐标：窗口上的像素。fitInView 把 800×600 的场景缩到 0.618 倍放进窗口，场景 (520,180) 就落在窗口 (322,124)。"),
            qsTr("标签是框的子图元：它的 pos (0,−22) 是相对父图元说的，换到场景里是 (520,158)。移动框，标签跟着走。")
        ]
    }

    Figure {
        files: ["handbook/qt/figures/graphicsview-fit.png", "handbook/qt/figures/graphicsview-zoom.png", "handbook/qt/figures/graphicsview-zoom-label.png"]
        captions: [qsTr("fitInView：整张图缩进窗口"), qsTr("放大 4 倍：线和字都跟着变大"), qsTr("标签设 ItemIgnoresTransformations：字恢复原大小")]
    }

    CodeRef { file: "examples/qt/graphicsview/main.cpp"; region: "pick" }
    CodeRef { file: "examples/qt/graphicsview/output.txt"; from: "在窗口"; to: "边线上"; caption: qsTr("实际输出") }

    Para {
        text: qsTr("用户点在窗口上，视图的 itemAt 自动把窗口坐标换回场景坐标，再找最上面的图元。点在框中间也算点中了框：没设填充的矩形，形状仍然是整个矩形。"
                 + "如果只想让边线可点，要重写图元的 shape()。")
    }

    CodeRef { file: "examples/qt/graphicsview/main.cpp"; region: "zoom" }
    CodeRef { file: "examples/qt/graphicsview/main.cpp"; region: "ignore" }
    CodeRef { file: "examples/qt/graphicsview/output.txt"; from: "==== 2"; to: "之后"; caption: qsTr("实际输出") }

    KeyPoints {
        label: qsTr("缩放")
        points: [
            qsTr("视图的变换作用于场景里的一切：放大 4 倍后，2 像素宽的画笔在截图里是 8 像素，18 像素高的标签变成 72 像素。照片也是按像素放大的，第二张图右上角能看到锯齿状的台阶。"),
            qsTr("标签设了 ItemIgnoresTransformations 后，字恢复到 18 像素，位置仍然跟着框走。显示测量值、编号这类文字，通常希望放大时字不变大。"),
            qsTr("线宽也想不随缩放变粗，可以把画笔设为 cosmetic（QPen::setCosmetic(true)），见下面的练习。")
        ]
    }

    CodeRef { file: "examples/qt/graphicsview/main.cpp"; region: "index" }
    CodeRef { file: "examples/qt/graphicsview/output.txt"; from: "==== 3" ; caption: qsTr("实际输出") }

    Para {
        text: qsTr("场景默认用 BSP 树给图元建索引，「这个区域里有哪些图元」不用逐个检查。10 万个图元、1000 次小区域查询：几次运行，有索引 0.13–0.30 秒，没有索引 4.9–6.7 秒，差 16 倍以上，而两者找到的东西完全一样。"
                 + "itemAt、碰撞检测、只重画看得见的部分，靠的都是这个索引。反过来，如果图元一直在移动（比如每帧都动的动画），每次移动都要更新索引，这时 NoIndex 反而可能更快——Qt 文档这样建议，本节没有测动态场景。")
    }

    Pitfall {
        text: qsTr("图元的 boundingRect() 必须包住它画的全部内容，包括画笔超出的那半个线宽。场景只按 boundingRect 决定哪里需要重画、哪些图元在查询区域里；画到外面的部分，移动后可能残留，点上去也可能点不中。"
                 + "自己写图元时，尺寸改变之前要先调用 prepareGeometryChange()。这两条是 QGraphicsItem 文档里的要求，示例用的都是现成图元，没有演示出错的样子。")
    }

    Try {
        task: qsTr("把第 1 步缺陷框的画笔改成 QPen(QColor(\"#ef4444\"), 2) 加 setCosmetic(true)（先构造 QPen 变量再设置），再看第 2 步截图里边线有多宽。")
        answerNote: qsTr("实测边线宽 2 像素（原来是 8 像素）：cosmetic 画笔的宽度按窗口像素算，不受视图缩放影响。框本身照样放大 4 倍，只是线不变粗。")
    }

    InSystem {
        text: qsTr("本程序工位页的检测结果，是检测代码用 OpenCV 直接画进结果图（src/app/StationController.cpp 里的 r.annotated），再整张交给 ImageView 显示，所以不能单独点选某个框。以后要做「点一个缺陷看详情、放大看细节」，就该换成本节的做法。「实验室」里旧版的节点式流程编辑器（src/modules/node_pipeline/NodeGraphPipelinePage.cpp）用的是本节的 QGraphicsView：节点和连线都是图元，可以拖动、缩放。")
    }
}
