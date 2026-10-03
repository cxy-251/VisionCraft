import QtQuick
import VisionCraft

Section {
    title: qsTr("拷贝、移动与隐式共享")
    lead: qsTr("把一个对象赋值给另一个变量、传给函数、放进容器时，背后的数据可能被完整复制，可能被转交，也可能只是多了一个人共用。三种情况的代价差几个数量级。")

    Why {
        text: qsTr("工位每次检测都要在几个地方之间传递一张截图、一份检测结果、一串要发给板子的字节。如果每传一次都完整复制一遍，"
                 + "图像大一点就会明显变慢；反过来，如果以为「复制了」其实是「共用」，改一处就会意外地影响另一处。"
                 + "C++ 标准库、Qt、OpenCV 在这件事上的规矩各不相同，这一节用地址比较和计时把它们摆在一起看。")
    }

    KeyPoints {
        label: qsTr("三种传递方式")
        points: [
            qsTr("拷贝（copy）：新对象有自己的一份完整数据。std::vector、std::string 的赋值就是这样，数据越大越慢。"),
            qsTr("移动（move）：新对象直接接管旧对象的数据，旧对象被掏空。只转交一个指针，和数据大小无关。用 std::move 显式要求，或者从函数返回局部变量时自动发生。"),
            qsTr("隐式共享（Qt 叫 implicit sharing，也叫写时复制）：赋值时只让两个对象指向同一块数据并把引用计数加 1；等到某一方要修改时，它才先复制一份。QString、QByteArray、QList、QImage 都是这样。"),
            qsTr("cv::Mat 是第四种：赋值后共用数据，但修改时不会复制——两边都能看到修改。要真正复制得调用 clone()（见 OpenCV 卷「cv::Mat 内存模型」）。")
        ]
    }

    CodeRef { file: "examples/cpp/copy_move/main.cpp"; region: "std-copy" }
    CodeRef { file: "examples/cpp/copy_move/main.cpp"; region: "move" }
    CodeRef { file: "examples/cpp/copy_move/main.cpp"; region: "sharing" }
    CodeRef { file: "examples/cpp/copy_move/output.txt"; from: "==== 1"; to: "a\\[0\\] 仍是"; caption: qsTr("实际输出") }

    Pitfall {
        text: qsTr("std::move 本身什么都不移动，它只是把对象标记成「可以被掏空」。如果对象是 const 的，没法被掏空，编译器就安静地改用拷贝——"
                 + "没有警告，只是慢。上面「std::move 一个 const 对象」那一行就是这样。")
    }

    Para {
        text: qsTr("隐式共享省下了多少？把 8 MB 的数据拷贝 1000 次。第三种写法看起来和第二种一样，只是把 at(0) 换成了 [0]：")
    }
    CodeRef { file: "examples/cpp/copy_move/main.cpp"; region: "timing" }
    CodeRef { file: "examples/cpp/copy_move/output.txt"; from: "==== 4"; to: "copy\\[0\\]"; caption: qsTr("本机实测（每次运行数字略有不同）") }

    Pitfall {
        text: qsTr("非 const 对象上的 [] 会触发复制，即使你只是读。operator[] 有 const 和非 const 两个版本，编译器按对象是否 const 来挑；"
                 + "非 const 版本返回可写的引用，Qt 不知道你接下来会不会写，只能先复制。写这个示例时，计时代码最初用的就是 copy[0]，"
                 + "结果 QByteArray 和 std::vector 一样慢——这一行是从真实的失误里留下来的。只读时用 at()、constData()，或者把对象声明成 const。")
    }

    Pitfall {
        text: qsTr("同样的道理，用范围 for 遍历一个正在共享的 Qt 容器，即使循环变量写成 const int &，也会先复制整个容器：for 调用的是容器的 begin()，"
                 + "容器本身不是 const，就调用非 const 的 begin()。用 std::as_const 包一下就不会：")
        CodeRef { file: "examples/cpp/copy_move/main.cpp"; region: "range-for" }
        CodeRef { file: "examples/cpp/copy_move/output.txt"; from: "==== 5"; to: "as_const 遍历之后"; caption: qsTr("实际输出") }
    }

    Para {
        text: qsTr("本项目里用到移动的一个关键地方：DeviceLink 收到应答后，先把对应的待处理请求从哈希表里「移出来」，再删掉表项，最后才调用回调。"
                 + "回调是别人写的代码，它可能马上发起一个新请求，往同一个哈希表里插入；如果这时还拿着指向表项的迭代器，插入可能让它失效。"
                 + "移出来之后，回调和表就没有关系了，而且移动 std::function 不会复制它捕获的东西：")
    }
    CodeRef { file: "src/device/DeviceLink.cpp"; region: "match" }

    Para {
        text: qsTr("另一处是 QImage 和 cv::Mat 之间的转换。QImage 可以「借用」一块别人的内存，但它不知道这块内存什么时候会被释放，"
                 + "所以把局部的 cv::Mat 包成 QImage 后必须 copy() 一份再返回：")
    }
    CodeRef { file: "src/app/StationController.cpp"; region: "toqimage" }

    Para { text: qsTr("最后看发缩略图时切块的代码。每一块是「4 字节偏移 + 最多 1000 字节像素」：") }
    CodeRef { file: "src/device/DeviceLink.cpp"; from: "QByteArray p\\(4, 0\\);"; to: "p \\+= m_img\\.pixels\\.mid" }

    Try {
        task: qsTr("m_img.pixels.mid(offset, len) 会和 m_img.pixels 共享内存，还是复制出 len 个字节？接着 p += 又做了什么？"
                 + "如果想让每块像素只被复制一次，可以怎么改？写一个小程序，用比较 constData() 地址的办法验证你的判断。")
        answerNote: qsTr("本机实测：mid(offset, len) 和 sliced(offset, len) 都会复制出新的 len 个字节（只有 mid(0) 这种取整个数组的情况才共享）；"
                       + "接着 p += 再把这段追加进 p，又复制一次，所以每块像素被复制了两次。"
                       + "QByteArrayView 只是「看」别人的内存，不拥有也不复制：p.append(QByteArrayView(m_img.pixels).sliced(offset, len)) 只在追加时复制一次。"
                       + "不过这里每块只有 1000 字节，复制的开销和一次 RTT 往返的几毫秒相比可以忽略，所以项目里没有改——先测量，再决定值不值得优化。")
    }

    InSystem {
        text: qsTr("src/device/DeviceLink.cpp 用 std::move 把请求的回调交给哈希表、再从哈希表里取出；字节数据一律用 QByteArray 按值传递，靠隐式共享避免复制。"
                 + "src/app/StationController.cpp 在 QImage 和 cv::Mat 之间转换时明确地复制一次，之后都靠共享传递。")
    }
}
