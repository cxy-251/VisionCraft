import QtQuick
import VisionCraft

Section {
    title: qsTr("lambda 与捕获")
    lead: qsTr("lambda 是一个可以带着「外面的变量」到处走的小函数。它带走的是变量的拷贝还是地址，决定了它稍后被调用时会不会出事。")

    Why {
        text: qsTr("Qt 代码里 lambda 无处不在：connect 的槽、QTimer::singleShot 的回调、QtConcurrent 的任务、本项目「发请求、等应答」的回调。"
                 + "这些 lambda 的共同点是：写下来的时候不执行，过一会儿才执行。那时它捕获的局部变量可能已经销毁了。"
                 + "本项目写 DeviceLink 时就踩过一次：回调按引用捕获了一个参数，结果日志里的命令名偶尔是错的。")
    }

    KeyPoints {
        label: qsTr("语法")
        points: [
            qsTr("[捕获列表](参数) { 函数体 }。捕获列表写函数体要用到的外部变量。"),
            qsTr("[x]：按值捕获，创建 lambda 时拷贝一份 x，之后外面的 x 怎么变都和它无关。"),
            qsTr("[&x]：按引用捕获，只记住 x 在哪里。调用时去读那个地方——如果 x 已经销毁，读到的就是垃圾。"),
            qsTr("[this]：捕获当前对象的指针，函数体里可以直接用成员。它是指针，对象没了照样悬空。"),
            qsTr("[=] 和 [&]：按值或按引用捕获函数体用到的所有变量。省事，但看不出到底捕获了什么，本项目不用。")
        ]
    }

    CodeRef { file: "examples/cpp/lambda_capture/main.cpp"; region: "basics" }
    CodeRef { file: "examples/cpp/lambda_capture/main.cpp"; region: "mutable" }
    CodeRef { file: "examples/cpp/lambda_capture/output.txt"; from: "==== 1"; to: "第三次调用"; caption: qsTr("实际输出") }
    Para {
        text: qsTr("按值捕获的变量在 lambda 里默认是只读的；加 mutable 才能改，改的也只是 lambda 自己那份。"
                 + "可以把 lambda 想成一个编译器自动生成的类：捕获的变量是它的成员，函数体是它的 operator()。")
    }

    Para {
        text: qsTr("下面用一个缩小版的「请求—应答」重现本项目的那个 bug。request 只把回调存起来，deliverReplies 模拟稍后应答到达：")
    }
    CodeRef { file: "examples/cpp/lambda_capture/main.cpp"; region: "bad" }
    CodeRef { file: "examples/cpp/lambda_capture/main.cpp"; region: "good" }
    CodeRef { file: "examples/cpp/lambda_capture/main.cpp"; region: "deferred" }
    CodeRef { file: "examples/cpp/lambda_capture/output.txt"; from: "==== 2"; to: "BEEP 完成"; caption: qsTr("按值捕获：实际输出") }
    Pitfall {
        text: qsTr("换成按引用捕获的版本，普通编译不崩溃、不报错，只是两次都打印 BEEP：PING 那个临时字符串销毁后，那块栈内存被 BEEP 的临时字符串重新用了，"
                 + "第一个回调读到的是别人的数据。这就是当初日志里命令名「偶尔不对」的原因。AddressSanitizer 能当场抓住：")
        CodeRef { file: "examples/cpp/lambda_capture/asan-dangling-ref.txt" }
    }
    Para { text: qsTr("DeviceLink 里现在的写法：type 和 label 按值捕获，this 用来访问成员和发信号：") }
    CodeRef { file: "src/device/DeviceLink.cpp"; from: "^int DeviceLink::simpleCommand"; to: "^}" }

    Para {
        text: qsTr("[this] 也会悬空：lambda 连到信号上以后，可能活得比捕获的对象还久。connect 有一个专门的参数解决这个问题——"
                 + "在信号和 lambda 之间传一个「上下文对象」，这个对象被销毁时，连接自动断开：")
    }
    CodeRef { file: "examples/cpp/lambda_capture/main.cpp"; region: "receiver" }
    CodeRef { file: "examples/cpp/lambda_capture/main.cpp"; region: "context" }
    CodeRef { file: "examples/cpp/lambda_capture/output.txt"; from: "==== 3"; to: "没有调用"; caption: qsTr("实际输出") }
    Pitfall {
        text: qsTr("漏掉上下文对象，连接会一直存在，面板删除后信号照样调用 lambda。普通编译时它甚至还「正常地」打印了「面板 显示 2」：")
        CodeRef { file: "examples/cpp/lambda_capture/main.cpp"; region: "no-context" }
        CodeRef { file: "examples/cpp/lambda_capture/asan-no-context.txt" }
    }
    Para {
        text: qsTr("所以本项目里凡是 connect 到 lambda，第三个参数都是 this（或者别的上下文对象）。DeviceLink 连接传输层的信号：")
    }
    CodeRef { file: "src/device/DeviceLink.cpp"; from: "connect\\(transport, &Transport::progress"; to: "\\}\\);" }

    Pitfall {
        text: qsTr("[=] 不会拷贝成员变量。在成员函数里写 [=] 再用成员 n，实际捕获的是 this 指针，n 是 this->n。对象销毁后照样悬空。"
                 + "本项目用 C++17 编译，这种写法连警告都没有；C++20 才把它标为「已弃用」：")
        CodeRef { file: "examples/cpp/lambda_capture/implicit_this.cpp" }
        CodeRef { file: "examples/cpp/lambda_capture/implicit-this.txt" }
    }

    Try {
        task: qsTr("QtConcurrent::run 会把 lambda 放到别的线程执行。工位检测时，如果写成 QtConcurrent::run([&image] { return inspect(image); })，"
                 + "其中 image 是发起检测的函数里的局部变量，会出什么问题？应该怎样捕获？为什么按值捕获一张 cv::Mat 并不会拷贝整张图？")
        answerNote: qsTr("发起检测的函数很快就返回了，局部变量 image 随之销毁，而工作线程可能还没开始读它——和 dangling-ref 示例是同一个问题，只是多了线程，更难复现。"
                       + "应该按值捕获：[image]。cv::Mat 的拷贝只复制头部、给像素数据的引用计数加 1（见 OpenCV 卷「cv::Mat 内存模型」），"
                       + "所以按值捕获很便宜，而且引用计数保证像素在工作线程用完之前不会被释放。前提是之后没人原地修改这张图。"
                       + "工位的实际写法就是这样：")
        answerFile: "src/app/StationController.cpp"
        answerRegion: "thread"
    }

    InSystem {
        text: qsTr("src/device/DeviceLink.cpp 的每个 request 都带一个应答回调，按值捕获需要的参数；所有 connect 到 lambda 的地方都传了上下文对象。"
                 + "src/app/StationController.cpp 用 QtConcurrent 在线程池里跑检测。")
    }
}
