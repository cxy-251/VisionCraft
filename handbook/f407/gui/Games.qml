import QtQuick
import VisionCraft

Section {
    title: qsTr("小游戏：格子、节拍与只重画变了的格子")
    lead: qsTr("扫雷、贪吃蛇、俄罗斯方块。三个游戏没有用 LVGL，而是用手写框架直接往屏上画格子：每一步只重画变了的那几格，几乎不占内存，也很快。三个游戏用了三种决定「画哪几格」的办法。")

    Why {
        text: qsTr("小游戏把前面几节的东西都用上了：指针事件（点、长按、滑动）、板上按键、定时的节拍、「重画的代价」里只画变了的东西。"
                 + "这类画面就是一格格的色块，用 LVGL 的话每格一个控件，几百个控件内存放不下，也没有必要。手写框架画一个格子就是一次 lcd_fill。")
    }

    CodeRef { file: "firmware/station/App/game.c" }

    KeyPoints {
        label: qsTr("共用的部分")
        points: [
            qsTr("随机数用 xorshift32：三次移位异或，周期 2^32 − 1，足够游戏用。种子平时用当前的毫秒数（每局不同）；调试器往 g_game_seed 写一个数，就每局都一样——同一个种子，模拟器和板子布的雷完全相同，测试结果能复现。"),
            qsTr("整局的状态放在 game.h 里的结构体中，和画面无关：离开页面再回来局面还在，测试和模拟器也直接读这些结构体。"),
            qsTr("界面框架的页面多了一个 key 回调：当前页面是游戏时，板上四个按键当方向键用，而且不再发给上位机——否则上位机会把 KEY0 当成「检测一次」。四个键的位置：WK_UP 上、KEY1 下、KEY2 左、KEY0 右（看屏的人实测确认）。")
        ]
    }

    CodeRef { file: "firmware/station/App/page_mines.c"; region: "rules" }
    CodeRef { file: "firmware/station/App/page_mines.c"; region: "pointer" }

    KeyPoints {
        label: qsTr("扫雷：每格一个「要重画」标记")
        points: [
            qsTr("第一下点完才布雷，而且避开这一格和它周围一圈：第一下必定翻开一片空地。"),
            qsTr("翻到周围没有雷的格子，就把邻居也翻开，一直扩散。用一个 81 格的数组当栈，不用递归——任务栈只有 8 KB，递归深度不好估。"),
            qsTr("棋盘不是控件：没按在按钮上的指针事件交给页面，自己算是哪一格。在同一格按下又抬起才算一下；按住超过 0.4 秒算长按，插旗。"),
            qsTr("哪一格的状态变了就给它打一个「要重画」的标记，每轮只画打了标记的格子。")
        ]
    }

    CodeRef { file: "firmware/station/App/page_snake.c"; region: "step" }

    KeyPoints {
        label: qsTr("贪吃蛇：每一步只动三格")
        points: [
            qsTr("身体存在一个环形缓冲里，另有一张「哪些格子被占着」的表，撞自己、放食物都查它。"),
            qsTr("走一步只有三格会变：新蛇头、原来的蛇头（变成身体颜色）、离开的蛇尾。不打标记，直接在改状态的地方把这三格画掉。模拟器里自动玩 30 秒，平均每步写 2431 个像素，大约是几个格子，吃到食物时还要重画一次分数。"),
            qsTr("方向键改的是「下一步的方向」，走的时候才生效，并且不能直接掉头——否则一步之内连按两次方向，蛇会撞上自己的脖子。"),
            qsTr("滑动：按下和抬起的位置之差，水平、竖直哪个分量大就往哪走，至少滑 30 像素才算。")
        ]
    }

    CodeRef { file: "firmware/station/App/page_tetris.c"; region: "draw" }

    KeyPoints {
        label: qsTr("俄罗斯方块：和屏上的比，不一样才画")
        points: [
            qsTr("七种方块各是一个 n×n 小方框里的 4 个格子；转一下就是绕方框中心转 90°：(x, y) → (n−1−y, x)。贴着墙转不开时往旁边挪一两格再试。"),
            qsTr("画法换了一种：记住屏上每一格现在是什么颜色，每轮把「棋盘 + 正在落的这一块」算出来，逐格比较，只画不一样的。不用操心哪一步改了哪些格子，下落、旋转、消行都自动只画变化的部分。"),
            qsTr("消行：从下往上找满的行，把它上面的所有行整体往下挪一行（一次 memmove）。消 1、2、3、4 行分别得 100、300、500、800 分，乘以等级。")
        ]
    }

    Figure {
        files: ["handbook/f407/gui/figures/gui-mines-first.png", "handbook/f407/gui/figures/gui-snake-play.png", "handbook/f407/gui/figures/gui-tetris-play.png"]
        captions: [qsTr("扫雷：第一下点正中间"), qsTr("贪吃蛇：模拟器里自动玩了 30 秒"), qsTr("俄罗斯方块")]
    }

    CodeRef { file: "handbook/f407/gui/games-test.txt"; from: "==== 1"; to: "看屏的人亲手玩"; caption: qsTr("模拟器和板子") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("扫雷：种子 12345，第一下点正中间翻开 44 格；把剩下的安全格点完，翻开 71 格（81 − 10）判赢；新开一局故意点雷判输。板子上用调试器做同样的操作，结果完全一样。进入游戏换页 25.4 ms。"),
            qsTr("俄罗斯方块：摆一个最底下一行只空中间四格的残局，落一根横长条，消 1 行、得 100 分，最底下一行清空。"),
            qsTr("看屏的人把三个游戏都玩了一遍，操作、判定、画面都正常，板上按键的方向也对。")
        ]
    }

    Pitfall {
        text: qsTr("贪吃蛇的节拍判断写成了「now − 上一步的时刻」，而上一步的时刻可能是在按方向键时记的，比这一轮的 now 还晚——和看门狗监工、闲置调暗是同一个坑。这次写的时候就用了有符号差。"
                 + "俄罗斯方块第一版的「GAME OVER」那行字每一轮都重画（每 20 ms 写约 4600 个像素），违背了自己定的规矩，改成状态变了才画。")
    }

    Try {
        task: qsTr("俄罗斯方块现在没有「一按到底」（硬降）。加一个 DROP 按钮要写几行？")
        answerNote: qsTr("几行：while (move(0, 1)) {} 一直往下挪到挪不动，再 lock()。画面不用管——下一轮 draw() 比较时自然只重画变了的格子。本题没有实测。")
    }

    InSystem {
        text: qsTr("首页第三行的 GAMES 里。游戏页面的数据在 game.h 的结构体里，离开再回来局面还在；正在进行的贪吃蛇回来时先暂停，按方向键继续。")
    }
}
