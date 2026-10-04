import QtQuick
import VisionCraft

Section {
    title: qsTr("QNetworkAccessManager")
    lead: qsTr("QNetworkAccessManager 发 HTTP 请求：get、post 立刻返回一个 QNetworkReply，结果在 finished 信号里拿。整个过程不阻塞界面。")

    Why {
        text: qsTr("工位常常要和工厂的 MES 系统打交道：开工前取配方，检测完上报结果。这些服务大多是 HTTP + JSON 接口。"
                 + "示例在本机用 QTcpServer 写了一个极简的假 MES 服务器（只监听 127.0.0.1），可以随意让它慢、让它出错，看客户端各种情况下拿到什么。")
    }

    CodeRef { file: "examples/qt/network/main.cpp"; region: "get" }
    CodeRef { file: "examples/qt/network/output.txt"; from: "==== 1"; to: "曝光"; caption: qsTr("实际输出") }

    KeyPoints {
        label: qsTr("基本用法")
        points: [
            qsTr("get() 返回时请求还没完成（isFinished = 0），后面的代码照常往下走。结果在 finished 信号里处理：先打印的是「get() 已经返回」，后打印配方。"),
            qsTr("QNetworkReply 由调用者负责释放，在 finished 的处理函数里 deleteLater()。忘了的话，每个请求泄漏一个对象。"),
            qsTr("一个程序通常只建一个 QNetworkAccessManager，所有请求共用：它内部维护连接池，同一个服务器的连接可以复用。")
        ]
    }

    CodeRef { file: "examples/qt/network/main.cpp"; region: "post" }
    CodeRef { file: "examples/qt/network/output.txt"; from: "==== 2"; to: "服务器回答"; caption: qsTr("实际输出") }

    CodeRef { file: "examples/qt/network/main.cpp"; region: "errors" }
    CodeRef { file: "examples/qt/network/output.txt"; from: "==== 3"; to: "端口没人听"; caption: qsTr("实际输出") }

    KeyPoints {
        label: qsTr("三种失败")
        points: [
            qsTr("服务器回了 500：error() 是 InternalServerError（401），HTTP 状态码 500，正文照样能读到，里面是服务器说的原因「database locked」。"),
            qsTr("路径不存在：ContentNotFoundError（203），状态码 404，也有正文。"),
            qsTr("端口没人听：ConnectionRefusedError（1），没有状态码，也没有正文——请求根本没到达 HTTP 这一层。"),
            qsTr("所以判断成功要同时看两样：error() == NoError，以及状态码是不是 2xx。出错时把状态码和正文记进日志，排查时最有用的往往是服务器正文里的那句话。")
        ]
    }

    CodeRef { file: "examples/qt/network/main.cpp"; region: "timeout" }
    CodeRef { file: "examples/qt/network/output.txt"; from: "==== 4"; to: "不设超时"; caption: qsTr("实际输出") }

    Pitfall {
        text: qsTr("超时报的不是「超时」：setTransferTimeout(500) 之后，约 0.5 秒结束（几次运行 475–526 ms），error() 是 OperationCanceledError（5），errorString 是「Operation canceled」。"
                 + "QNetworkReply::TimeoutError 这个枚举值存在，但这里拿到的不是它。代码里如果只判断 TimeoutError，超时就会被当成「用户自己取消」处理掉。"
                 + "不设超时，请求就一直等到服务器回答为止（这里约 3 秒）；服务器要是永远不回，就永远等。生产代码要么设 transferTimeout，要么给整个 QNetworkAccessManager 设 setTransferTimeout。")
    }

    CodeRef { file: "examples/qt/network/main.cpp"; region: "parallel" }
    CodeRef { file: "examples/qt/network/output.txt"; from: "==== 5"; caption: qsTr("实际输出") }

    Para {
        text: qsTr("20 个请求一口气发出，服务器那边同时最多只有 6 个连接：对同一个服务器，QNetworkAccessManager 最多并行 6 个 HTTP/1.1 连接，其余排队。"
                 + "每个请求服务器处理 100 ms，20 个分 4 批（6+6+6+2），总共约 410 ms，几次运行都在 406–411 ms。"
                 + "所以「同时上报 1000 条结果」并不会真的开 1000 个连接；要更快，应该让服务器支持一次接收多条（批量接口）。")
    }

    Try {
        task: qsTr("第 4 步把 setTransferTimeout(500) 改成 setTransferTimeout(5000)。结果会是什么？")
        answerNote: qsTr("实测 2992 ms 后成功结束，error() = 0（NoError；没有出错时 errorString 显示的是「Unknown error」，不要拿它判断成败）。服务器 3 秒后回答，没碰到 5 秒的限制。按 Qt 文档，transferTimeout 是「这么久没有任何数据传输就放弃」，而不是「整个请求必须在这么久内完成」；一个持续有数据、但总共传了很久的下载不会被它打断（本节没有测这种情况）。")
    }

    InSystem {
        text: qsTr("本程序目前不连接 MES，也不访问网络。手册里外链的资料都是静态文字。"
                 + "以后要把每班的良率上报给服务器，就在 StationController 检测完成的地方，用本节的 post 发一个 JSON；记得设超时，并把失败的结果先存在本地，等网络恢复后补传。")
    }
}
