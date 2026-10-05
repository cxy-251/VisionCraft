import QtQuick
import VisionCraft

Section {
    title: qsTr("事件与控件：一个最小的界面框架")
    lead: qsTr("图形界面库的核心其实只有三样东西：控件（屏上一块会画自己、会响应点击的矩形）、页面（一组控件）、事件（手指或鼠标的按下、移动、抬起）。工位固件的界面框架 gui.c 不到 200 行，把这三样都做了。")

    Why {
        text: qsTr("旧版固件 firmware/f407zg 的多页面界面靠方向键一页页切换。工位屏有触摸，更自然的做法是像手机那样：首页摆一排图标，点哪个进哪个页面，左上角返回。"
                 + "这一章先自己写一个最小框架，弄清楚任何图形库底下都在做的几件事；之后再换成成熟的 LVGL，就知道它替我们做了什么。"
                 + "框架只用 lcd.h 里的几个画图函数（填矩形、写字、贴图），不碰任何硬件，所以同一份代码也能在电脑上跑（见「在电脑上运行界面代码」）。")
    }

    CodeRef { file: "firmware/station/App/gui.h"; region: "event" }
    CodeRef { file: "firmware/station/App/gui.h"; region: "widget" }
    CodeRef { file: "firmware/station/App/gui.h"; region: "page" }

    KeyPoints {
        label: qsTr("三个结构")
        points: [
            qsTr("事件只有三种：按下、移动、抬起，带一个坐标。触摸屏、鼠标、甚至调试器注入，最后都变成这三种，框架不关心事件从哪来。"),
            qsTr("控件就是一个矩形加两个函数指针：draw（怎么画自己）、on_click（被点了做什么）。pressed 记录「正被按着」，dirty 记录「需要重画」。C 没有类和虚函数，函数指针就是它的多态。"),
            qsTr("页面是一组控件，加上 enter（进入时画背景）和 tick（显示期间周期刷新数据）。框架用一个小栈记录页面：进入新页面压栈，返回出栈，所以返回键总能回到来的地方。")
        ]
    }

    CodeRef { file: "firmware/station/App/gui.c"; region: "handle" }

    KeyPoints {
        label: qsTr("一次「点击」是怎么判定的")
        points: [
            qsTr("按下：找到这个坐标上的控件（从后往前找，后加的在上层），记下它，标成按下的样子。"),
            qsTr("按着移动：如果移出了那个控件，就取消——这次不算点击。手机上按住一个按钮再把手指滑走，按钮不会触发，就是这个规则。"),
            qsTr("抬起：只有在同一个控件上抬起，才调用它的 on_click。所以「点击」不是一个事件，而是按下和抬起两个事件组合出来的判断。"),
            qsTr("按下时控件立刻变亮，让手指知道「按到了」；真正的动作要等抬起。")
        ]
    }

    Figure {
        files: ["handbook/f407/gui/figures/gui-home.png", "handbook/f407/gui/figures/gui-home-pressed.png"]
        captions: [qsTr("首页"), qsTr("按住 STATION 图标、还没抬起：图标背景变亮")]
    }

    CodeRef { file: "firmware/station/App/gui.c"; region: "tick" }

    Para {
        text: qsTr("画的时机和事件分开：处理事件时只改状态、标记 dirty，真正画在 gui_tick 里统一做，而且只画标了 dirty 的控件。"
                 + "这样一次按下只重画一个图标，而不是整屏；几个事件挤在一起时也只画一次。「重画的代价」一节会量这个区别有多大。")
    }

    CodeRef { file: "firmware/station/App/station.c"; region: "pointer"; caption: qsTr("板子上：触摸芯片的读数变成指针事件；调试器还能往一个队列里注入事件") }

    KeyPoints {
        label: qsTr("接到板子上")
        points: [
            qsTr("StationTask 每 20 ms 读一次触摸芯片（驱动见「GT9147 电容触摸」）：手指刚接触发「按下」，之后发「移动」，读到手指离开时在最后的位置发「抬起」。坐标超出屏幕的读数直接丢掉——「触屏终端与软键盘」一节实测出现过 65535。"),
            qsTr("g_gui_inject 是调试用的事件队列：调试器写进事件，固件下一轮循环就当作真的触摸处理。自动测试不用人去点屏幕。")
        ]
    }

    CodeRef { file: "tools/gui_probe.tcl"; region: "inject" }
    CodeRef { file: "handbook/f407/gui/board-test.txt"; from: "第 1 次"; to: "08a5"; caption: qsTr("在板子上：用调试器注入点击，读当前页面和点击次数") }

    Pitfall {
        text: qsTr("调试器注入口最早只有一个槽：写进 type、x、y，再把序号加 1。第一次测试一切正常，第二次「返回键」那一下却丢了，后面的点击全部错位。"
                 + "原因是进入工位页要整屏重画，当时要两三百毫秒，任务这段时间顾不上读注入口；调试器紧接着写入的「抬起」覆盖了还没被处理的「按下」。"
                 + "改成能放 8 个事件的环形队列后，连续快速点三下（每下按住 100 ms，两次之间不等待）也一个不丢。真实的触摸也有同样的问题：任务忙着重画时手指的短按可能被漏掉，所以重画要快——这就是「重画的代价」一节的内容。")
    }

    Try {
        task: qsTr("把 gui_handle 里 GUI_UP 的判断改成「只要按下过某个控件，抬起时不管在哪都算点击」（去掉 inside 判断），再看测试脚本里「拖到图标外再抬起」那一步会怎样。")
        answerNote: qsTr("推测：拖出去的那次也会被当成点击，进入 SENSORS 页，点击次数加 1。这正是去掉这个判断会带来的误触：手指按到图标、想反悔滑走，界面却照样执行了。本题没有改固件实测，可以在电脑上的模拟器里改一行验证。")
    }

    InSystem {
        text: qsTr("这个框架现在就是工位固件的界面：开机是首页，点图标进各个页面，左上角返回。后来加的 LVGL 页面也挂在这个框架下面（见「移植 LVGL」）。和上位机的通信不受影响（vclink_probe 实测吞吐、缩略图都正常）。")
    }
}
