import QtQuick
import VisionCraft
Section {
    title: qsTr("后台计算与界面刷新：结果还属于当前请求吗")
    lead: qsTr("把计算放到线程池，只解决了界面不被计算占住。任务完成时，还要判断它处理的是哪张图、哪组参数，以及当前界面是否仍需要它。")
    Why { text: qsTr("QtConcurrent章节已经讲过run、Watcher和线程池。本节只补结果归属：任务A按旧阈值开始，用户随后改图和阈值，A即使正常完成，也不能被标成当前结果。这个问题与函数有没有计算错误不同。") }
    Para { text: qsTr("示例用4个像素和固定阈值。它提交第1版后，先在主线程把输入和阈值改成第2版，再进入事件循环接收完成通知，不依赖sleep来碰巧制造先后顺序。程序只允许一个计算任务在途。") }
    CodeRef { file: "examples/integration/latest_result/main.cpp"; region: "snapshot" }
    KeyPoints {
        label: qsTr("提交时固定三样东西")
        points: [
            qsTr("输入图：任务拿到提交时的图像值。示例使用QImage按值捕获，主线程后来对另一个QImage实例fill会触发写时分离；这一点不能直接套到cv::Mat上，Mat按值仍共享像素。"),
            qsTr("参数：阈值按值捕获。工作线程不去读取滑条、不查询此刻控制器成员，更不在处理一半时切换到另一组参数。"),
            qsTr("请求版本：记录提交时的revision。输入或影响结果的参数变化，就增加当前版本。结果把自己的版本一并带回，发布时才有比较依据。")
        ]
    }
    CodeRef { file: "examples/integration/latest_result/main.cpp"; region: "change" }
    CodeRef { file: "examples/integration/latest_result/main.cpp"; region: "publish" }
    KeyPoints {
        label: qsTr("完成、发布、刷新是三件事")
        points: [
            qsTr("完成：线程池里的函数返回一个结果，表示计算结束。它不会因此自动更新界面，也不意味着结果一定符合当前选择。"),
            qsTr("发布：Watcher和连接的接收上下文都在主线程。完成回调比较版本；旧版本只丢弃结果，再按最新状态提交下一次计算，版本匹配才采用。"),
            qsTr("刷新：实际应用采用结果后更新属性并发NOTIFY，QML绑定重新读取；查看器再安排绘制。线程完成通知、属性通知和重绘请求各有自己的职责。")
        ]
    }
    CodeRef { file: "examples/integration/latest_result/output.txt"; caption: qsTr("相同执行顺序下的实际输出") }
    Para { text: qsTr("第1版仍按阈值128、原图[40,120,180,220]计算出[0,0,255,255]，说明任务没有混入新状态；但它返回时当前版本已经是2，所以被丢弃。第2版用全240的输入和阈值200，得到四个255并发布。workerOnGui=0、publishOnGui=1说明本例计算和采用结果发生在不同职责的线程中。") }
    Pitfall { text: qsTr("版本比较不能修复共享像素的数据竞争。如果主线程和工作线程同时改同一块Mat.data，即使最后丢弃结果，竞争也已经发生。先保证提交数据在任务期间不被并发修改，再用版本号处理结果归属。") }
    Pitfall { text: qsTr("丢弃旧结果不等于中断正在执行的OpenCV函数。停止显示、使版本失效、协作取消和等待线程退出是不同动作；不要为了界面上显示了停止，就假设工作线程已经释放所有资源。") }
    KeyPoints {
        label: qsTr("回到已有工坊控制器")
        points: [
            qsTr("ImageWorkshop的job携带revision，只有匹配当前版本才发布；参数变化或清空会让旧结果失效。resultCurrent还控制能否把结果作为当前参数的结果导出。"),
            qsTr("工坊把快速连续调整合并为180 ms预览窗口，并在忙时记录待处理状态。防抖减少提交次数，单任务控制限制在途工作，版本检查避免旧结果发布；三者不能互相替代。"),
            qsTr("工作线程不捕获控制器this，任务通过值交付数据；对象销毁时也必须妥善处理在途任务寿命。本例退出事件循环后等待自己的线程池收尾，没有把线程交给已经销毁的局部变量。")
        ]
    }
    CodeRef { file: "src/app/ImageWorkshop.h"; match: "bool resultCurrent" }
    CodeRef { file: "src/app/ImageWorkshop.cpp"; region: "process" }
    Try { task: qsTr("只做防抖、不比较版本，能否保证在途旧任务不会覆盖新状态？只比较版本、每次拖动都提交任务，又会怎样？"); answerNote: qsTr("防抖不能阻止已经提交的任务晚些完成，仍需检查结果归属。只比较版本虽能拒绝旧结果，却可能排队计算大量最终不用的任务。提交节奏、在途数量和发布条件应分别设计。") }
    InSystem { text: qsTr("本机目标example_latest_result，运行build/examples/integration/latest_result/example_latest_result。它复用ImageOperations和ScreenCapture，在终端观察提交、完成、丢弃、发布；理解机制后再对照ImageWorkshop与VisionRunner，不为教学建立新的全局任务框架。") }
}
