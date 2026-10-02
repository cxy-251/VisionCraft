import QtQuick
import VisionCraft

Section {
    title: qsTr("cv::Mat 内存模型")
    lead: qsTr("cv::Mat 是「一个小小的头 + 一块可能被很多人共用的像素数据」。分清什么时候共用、什么时候复制，能避开 OpenCV 里大半的 bug。")

    Why {
        text: qsTr("一张 1920×1080 的彩色图有 6 MB。如果每次把 Mat 传给函数、赋值给变量都复制一遍像素，程序会慢得不能用。"
                 + "所以 OpenCV 的 Mat 默认不复制像素：赋值只复制一个几十字节的「头」（尺寸、类型、指向数据的指针），"
                 + "像素数据由几个 Mat 共用，并用引用计数记录有几个 Mat 在用它，最后一个放手时才释放。"
                 + "好处是快，代价是：你改一个 Mat，别的 Mat 可能跟着变。")
    }

    Demo {
        title: qsTr("谁和谁共用数据")
        MatView { }
        Para {
            font.pixelSize: Theme.fontSmall
            color: Theme.textMuted
            text: qsTr("边框颜色相同 = 共用同一块数据。依次点：B = A → C = A.clone() → R = A(ROI) → A 中间涂白（看 B 和 C）"
                     + "→ R 涂灰（看 A）→ A.release()（看 B 还在不在）。每一步的结论都有测试 tst_mat_demo 用真实的 OpenCV 验证过。")
        }
    }

    KeyPoints {
        points: [
            qsTr("B = A、按值传参、按值返回：只复制头，共用数据，引用计数 +1。改 B 的像素就是改 A 的像素。"),
            qsTr("A.clone()、A.copyTo(C)（C 尺寸类型不同时）：分配新内存、复制全部像素，从此无关。"),
            qsTr("A(cv::Rect(...))、A.row(i)、A.colRange(...)：取一个窗口（ROI），还是同一块数据，只是起点和步长不同。改 ROI 就是改原图对应的区域。"),
            qsTr("A.release() 或 A 离开作用域：只是 A 放手，引用计数 -1。别人还在用，数据就还在。"),
            qsTr("OpenCV 函数的输出参数（例如 cv::GaussianBlur(src, dst, ...) 里的 dst）用 create() 准备内存：尺寸或类型不对才重新分配；"
                 + "已经合适就直接写进 dst 原来的内存。如果那块内存还被别的 Mat 共用，别人也会被改——下面的例子就会遇到。")
        ]
    }

    Pitfall {
        text: qsTr("把 Mat 转成 Qt 的 QImage 时，QImage(mat.data, ...) 这个构造函数不复制像素，只是借用 Mat 的内存。"
                 + "Mat 一被释放，QImage 指向的就是一块已经还回去的内存，显示出来是花屏，或者直接崩溃。"
                 + "工位页把检测结果转成 QImage 时，最后的 .copy() 就是为此加的：")
        CodeRef { file: "src/app/StationController.cpp"; region: "toqimage" }
    }

    Pitfall {
        text: qsTr("想「在副本上画图、原图保持干净」时，要 clone()。检测算法在结果图上画圆、画框，开头就是 res.annotated = bgr.clone()；"
                 + "写成 res.annotated = bgr，标注就会画进调用者传进来的原图里。")
    }

    Pitfall {
        text: qsTr("Mat 交给另一个线程处理时，传过去的只是头，两个线程共用数据。只要原线程之后不再改这张图就没问题。"
                 + "工位页把截图转成 Mat，按值捕获进线程池里的 lambda；主线程之后不再碰它，所以不需要 clone：")
        CodeRef { file: "src/app/StationController.cpp"; region: "thread" }
    }

    Para {
        text: qsTr("按值传参最容易让人上当。下面三个函数都「按值」接收图像，在函数里把它调暗。"
                 + "调用 darken(photo) 之后，调用者手里的 photo 会不会变？先猜，再看实际运行结果：")
    }
    CodeRef { file: "examples/opencv/mat_sharing/main.cpp"; region: "functions" }
    CodeRef { file: "examples/opencv/mat_sharing/output.txt"; caption: qsTr("程序输出") }

    KeyPoints {
        label: qsTr("从输出里读出来的")
        points: [
            qsTr("darkenInPlace：photo 变暗了。按值传参只复制头，img 和 photo 共用像素，-= 是原地修改。"),
            qsTr("darkenExpr：photo 也变暗了。很多人以为 img = img - x 会「算出一张新图再赋给 img」，"
                 + "但 OpenCV 发现左边的 img 尺寸、类型都合适，就直接把结果写进它原来的内存——也就是 photo 的内存。"),
            qsTr("darkenInt：只有蓝色通道变了。整数 50 被当成 cv::Scalar(50, 0, 0, 0)，只减第一个通道（OpenCV 的通道顺序是 B G R）。"
                 + "三个通道都要减，得写 cv::Scalar::all(50)。"),
            qsTr("传 photo.clone() 进去：photo 不变，函数改的是一份独立的副本。")
        ]
    }

    Try {
        task: qsTr("改写 darkenExpr，让它不影响调用者，但不能改函数签名（参数仍然按值传 cv::Mat）。至少想出两种写法。")
        answerNote: qsTr("① 在函数里先 img = img.clone(); 让 img 指向自己的数据，再做减法；"
                       + "② 把结果写进一个新的局部变量：cv::Mat out = img - cv::Scalar::all(50);（out 原来是空的，必须新分配）。"
                       + "更好的做法是从接口上表明意图：只读的输入写成 const cv::Mat&，要改的输出单独作为参数或返回值。")
    }

    InSystem {
        text: qsTr("工位的一次检测里，Mat 的流向是：界面截图 QImage → toMat()（转换时复制）→ 线程池里检测（共用，只读）"
                 + "→ 结果图 annotated（clone 出来再画）→ toQImage()（转换后 copy）→ 界面显示。")
    }
}
