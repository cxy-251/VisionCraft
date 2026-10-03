import QtQuick
import VisionCraft

Section {
    title: qsTr("文件监视与热重载")
    lead: qsTr("QFileSystemWatcher 在文件变化时发信号。难点不在用法，而在「保存」这件事在不同编辑器里根本不是同一种操作。")

    Why {
        text: qsTr("配置文件改了自动生效、模板改了自动刷新、手册保存后立刻看到效果——都靠监视文件。本程序开发模式下手册「保存即刷新」就是这么做的。"
                 + "示例在临时目录里建一个文件，用几种常见的保存方式去改它，记录收到几次信号、文件还在不在监视列表里。")
    }

    CodeRef { file: "examples/qt/file_watch/main.cpp"; region: "watch" }
    CodeRef { file: "examples/qt/file_watch/main.cpp"; region: "saves" }
    CodeRef { file: "examples/qt/file_watch/main.cpp"; region: "readd" }
    CodeRef { file: "examples/qt/file_watch/output.txt"; caption: qsTr("本机实测（Linux，底层是 inotify）") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("原地改写、追加，都收到 fileChanged，文件仍在监视。连着追加两次只收到 1 次信号：短时间内的多次变化会被合并，不能拿信号次数当修改次数。"),
            qsTr("「写临时文件再改名覆盖」也收到 1 次信号，但之后文件就不在监视列表里了。监视的其实是原来那个文件本身（inode），它被删掉了；同名的新文件是另一个文件。"),
            qsTr("于是第 4 步再改，一次信号都没有——热重载「只灵一次」的典型症状。重新 addPath 之后恢复正常。")
        ]
    }

    Para {
        text: qsTr("很多编辑器（以及 sed -i、不少 IDE）保存时就是先写临时文件再改名，这样即使保存到一半断电，原文件也是完整的。"
                 + "所以监视代码必须假设文件会「消失再出现」。SourceProvider 收到变化后等 100 ms（改名可能还没完成），文件存在而不在列表里就重新加入，再通知手册页重新加载：")
    }
    CodeRef { file: "src/handbook/SourceProvider.cpp"; from: "connect\\(&m_watcher, &QFileSystemWatcher::fileChanged"; to: "^    \\}\\);" }

    Pitfall {
        text: qsTr("收到变化信号只说明文件变了，不代表它已经写完。程序在信号里立刻去读，可能读到写了一半的内容，或者（改名方式）读的时候文件暂时不存在。"
                 + "稍等一下再读（上面的 100 ms），或者读失败时重试。")
    }

    Pitfall {
        text: qsTr("文件监视只是通知「该重新加载了」，重新加载本身也要做对。本程序的手册页就出过问题：监视、信号都正常，重新加载却拿到了 QML 引擎缓存里的旧版本，"
                 + "详见「解剖本程序的外壳」。排查这类问题时，先确认是哪一步断了：本程序可以用 QT_LOGGING_RULES=\"vc.source.debug=true\" 打印监视、变化、通知的每一步。")
    }

    Try {
        task: qsTr("如果要监视的是一个目录（比如「配方目录里新增了文件就自动出现在列表里」），该用什么信号？目录里某个已有文件的内容被改了，目录会不会发信号？"
                 + "在示例里加一句 watcher.addPath(dir.path()) 并连接 directoryChanged 试一试。")
        answerNote: qsTr("用 directoryChanged：目录里新增、删除、改名文件时触发。只改已有文件的内容，目录本身没变，不会触发 directoryChanged。"
                       + "注意第 3 种保存方式会触发它：临时文件的创建和改名都改变了目录的内容。")
    }

    InSystem {
        text: qsTr("src/handbook/SourceProvider.cpp 的 watch() 和变化处理；qml/handbook/HandbookPage.qml 的 Connections 收到通知后重新加载。只在开发模式（VC_DEV=1）启用。")
    }
}
