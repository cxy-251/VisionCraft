import QtQuick
import VisionCraft

Section {
    title: qsTr("自定义控件")
    lead: qsTr("一个好用的自定义控件，要像标准控件一样：放进布局知道自己该多大，数值变了自己重画，能通过属性名设置，样式表也管得到它。")

    Why {
        text: qsTr("上一节把仪表盘画在 QImage 上。真正放进界面，还得回答几个问题：布局给它多大？数值一秒变几十次时画几次？能不能在 QML、动画、样式表里用？"
                 + "示例把温度表做成控件 Gauge，逐条测出来。")
    }

    CodeRef { file: "examples/qt/custom_widget/main.cpp"; region: "class" }

    KeyPoints {
        label: qsTr("一个控件的「接口」")
        points: [
            qsTr("Q_PROPERTY(double value READ … WRITE … NOTIFY …)：把数值登记到元对象系统。之后 setProperty(\"value\", 75)、QML 绑定、属性动画、样式表的 qproperty- 都能用它（见「动态属性」）。"),
            qsTr("setValue 先判断值有没有变：没变就直接返回，不发信号、不重画。否则两个控件互相绑定时，很容易变成无限循环。"),
            qsTr("sizeHint / minimumSizeHint：告诉布局「希望多大、最小多大」（见「布局与伸缩因子」）。"),
            qsTr("改了值调用 update()，而不是 repaint()，原因见下面的测量。")
        ]
    }

    CodeRef { file: "examples/qt/custom_widget/main.cpp"; region: "paint" }
    Figure {
        files: ["handbook/qt/figures/custom-gauge.png"]
        captions: [qsTr("两个 Gauge 放在一个水平布局里；右边那个被样式表改了指针颜色和背景")]
    }

    CodeRef { file: "examples/qt/custom_widget/output.txt"; from: "==== 1"; to: "每个表"; caption: qsTr("实际输出") }
    Para {
        text: qsTr("窗口没有设大小，布局按两个表的 sizeHint 算出来：宽 160 × 2 + 两边边距 11 × 2 + 间隔 6 = 348，高 160 + 22 = 182。")
    }

    CodeRef { file: "examples/qt/custom_widget/main.cpp"; region: "update" }
    CodeRef { file: "examples/qt/custom_widget/main.cpp"; region: "repaint" }
    CodeRef { file: "examples/qt/custom_widget/output.txt"; from: "==== 2"; to: "repaint"; caption: qsTr("实际输出") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("update() 只是登记「这块需要重画」，真正的 paintEvent 要等回到事件循环才执行，多次登记会合并：改了 100 次，只画了 1 次，画的是最后的值。"),
            qsTr("repaint() 立刻同步调用 paintEvent：100 次就画 100 次，而屏幕刷新率一秒只有 60 次左右，多出来的都白画了。只有必须马上看到结果、等不到回事件循环的场合才用它。"),
            qsTr("第二轮设的值都是 50，和当前值一样，于是信号和重画都是 0 次。")
        ]
    }

    CodeRef { file: "examples/qt/custom_widget/output.txt"; from: "==== 3"; to: "窗口变大一次"; caption: qsTr("实际输出") }
    Para {
        text: qsTr("表盘缓存在 QPixmap 里（做法见「QPainter 绘图与双缓冲」）。数值变 50 次，表盘一次也没重画，只重画指针；窗口变大时 resizeEvent 把缓存清空，下一次 paintEvent 按新尺寸重画一次。")
    }

    CodeRef { file: "examples/qt/custom_widget/main.cpp"; region: "property" }
    CodeRef { file: "examples/qt/custom_widget/output.txt"; from: "==== 4"; caption: qsTr("实际输出") }

    KeyPoints {
        label: qsTr("属性和样式表")
        points: [
            qsTr("setProperty(\"value\", 75) 走的是 Q_PROPERTY 登记的 setValue，值被设成了 75。"),
            qsTr("样式表里 qproperty-needleColor: #22c55e 把 needleColor 属性设成绿色。选择器 Gauge#b 里的类名取自元对象系统（metaObject()->className()），这个名字是 Q_OBJECT 宏生成的。"),
            qsTr("背景 #0f172a 和圆角是 paintEvent 开头 QStyleOption + drawPrimitive(PE_Widget) 画出来的。圆角外面的 (0,0) 是窗口底色 #efefef。")
        ]
    }

    Pitfall {
        text: qsTr("核对像素时，不要单独对子控件调用 grab()。一开始示例用的是 b->grab()，结果读回的像素在任何情况下都是 #0f172a，连圆角外面也是，把 drawPrimitive 删掉也还是。"
                 + "原因是单独 grab 一个子控件时，Qt 会先用它调色板里的背景色把整块填满，而样式表恰好把调色板的背景色也改成了 #0f172a。"
                 + "改成截整个窗口 w.grab() 再取 b 所在位置的像素，才和屏幕上看到的一致。")
    }

    Try {
        task: qsTr("把 paintEvent 开头的 style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this) 这一行删掉，再运行。b 中间偏左的 (6,110) 还是 #0f172a 吗？")
        answerNote: qsTr("实测变成 #efefef，也就是窗口的底色：样式表里的 background 没人画了。这和「QSS 样式表与换肤」里 Panel 的情况一样，"
                       + "那里的解决办法是设置 WA_StyledBackground，这里是在 paintEvent 里自己调用 drawPrimitive，两种都可以。")
    }

    InSystem {
        text: qsTr("本程序的自定义显示控件在 QML 那边：src/app/ImageView.h 继承 QQuickPaintedItem，用 Q_PROPERTY 声明 image、smooth 两个属性，setImage 里改完值调用 update()——和本节 Gauge 的写法一一对应，"
                 + "只是基类从 QWidget 换成了 QQuickPaintedItem，paintEvent 换成了 paint(QPainter *)。")
    }
}
