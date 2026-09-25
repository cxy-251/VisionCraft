#include "KnowledgeRegistry.h"

void KnowledgeRegistry::registerQtCoreTopics() {
    // ========================================================
    // 9. Qt 01. 核心机制与底层哲学 (深度架构与机制)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_signals_slots";
        t.framework = "Qt";
        t.category = "Qt 01. 核心机制与底层哲学";
        t.name = "信号与槽机制 (Signals & Slots)";
        t.tag = "现代 C++ 观察者模式典范";
        t.isVisualInteractive = false;
        t.apiSignature = "connect(sender, &Sender::valueChanged, receiver, &Receiver::onValueChanged, Qt::ConnectionType);";
        t.docSummary = "Qt 最灵魂的对象间通信机制。彻底解除了调用者与接收者的强耦合。<br>"
                       "支持<b>编译期类型检查</b>、支持 Lambda 表达式、支持<b>自动跨线程安全投递</b>！";
        t.docParams = "• <b>编译期类型安全:</b> 新版语法在编译时直接校验信号参数与槽函数签名是否匹配。<br>"
                      "• <b>自动生命周期管理:</b> 只要发送者或接收者任一方析构，该连接全自动注销断开。";
        t.usageTiming = "所有 UI 事件响应（按钮点击、滑块拖动）、异步后台线程向 UI 线程安全通知进度、组件间松耦合通信。";
        t.bestPractices = "① <b>绝对不要使用已淘汰的 `SIGNAL(...)` 和 `SLOT(...)` 宏语法</b>！必须使用 C++11 函数指针语法。<br>"
                          "② 信号声明在 `signals:` 下，只需声明无需编写实现代码，MOC 编译器会自动生成触发存根。";
        t.codeSnippet = 
            "// 1. 现代函数指针连接（带编译期检查）\n"
            "connect(slider, &QSlider::valueChanged, this, &MyClass::handleValChanged);\n\n"
            "// 2. 现代 Lambda 优雅连接（支持捕获局部变量）\n"
            "connect(button, &QPushButton::clicked, this, [this]() {\n"
            "    qDebug() << \"按钮被点击，当前状态:\" << m_status;\n"
            "});\n\n"
            "// 3. 跨线程异步安全投递（从算法子线程把 Mat 送回 UI 渲染）\n"
            "connect(worker, &WorkerThread::frameReady, uiWindow, &MainWindow::updateView,\n"
            "        Qt::QueuedConnection);";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "qt_connection_types";
        t.framework = "Qt";
        t.category = "Qt 01. 核心机制与底层哲学";
        t.name = "信号与槽 5 种连接类型深度剖析";
        t.tag = "跨线程通信与防死锁准则";
        t.isVisualInteractive = false;
        t.apiSignature = "enum Qt::ConnectionType {\n    AutoConnection,\n    DirectConnection,\n    QueuedConnection,\n    BlockingQueuedConnection,\n    UniqueConnection\n};";
        t.docSummary = "Qt 的 connect 第 5 个参数决定了槽函数的执行上下文与同步/异步行为。深入理解连接类型是掌握 Qt 多线程编程的绝对分水岭。";
        t.docParams = "• <b>AutoConnection (默认):</b> 若发送者和接收者处于同一线程，采用 Direct；处于不同线程，自动转为 Queued。<br>"
                      "• <b>DirectConnection:</b> 槽函数在<b>信号发送者的当前线程</b>中同步直接执行（类似函数直接调用）。<br>"
                      "• <b>QueuedConnection:</b> 跨线程事件排队。信号转换为事件压入<b>接收者所在线程的事件队列</b>，由接收者线程的事件循环取出执行。<br>"
                      "• <b>BlockingQueuedConnection:</b> 发送线程挂起阻塞，直到接收线程执行完槽函数后才被唤醒继续执行。<br>"
                      "• <b>UniqueConnection:</b> 防止多次重复 connect 导致同一槽函数被重复触发多次（可与其他位或结合）。";
        t.usageTiming = "跨线程向 UI 线程发送计算结果（用 QueuedConnection）、后台线程同步等待 UI 用户弹出对话框点击确认（用 BlockingQueuedConnection）。";
        t.bestPractices = "⚠️ <b>绝对禁止死锁：</b>千万不要在<b>同一个线程内</b>使用 <code>BlockingQueuedConnection</code>，会导致当前线程自己等待自己处理事件，发生<b>永久性死锁</b>！";
        t.codeSnippet = 
            "// 1. 经典跨线程通知（异步无阻塞）：\n"
            "connect(workerThread, &Worker::dataReady, ui, &MainUI::renderData, Qt::QueuedConnection);\n\n"
            "// 2. 避免重复绑定的防抖连接：\n"
            "connect(btn, &QPushButton::clicked, this, &MainUI::onSubmit, \n"
            "        static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection));\n\n"
            "// 3. 阻塞等待主线程弹窗结果（必须跨线程）：\n"
            "// 假设在算法子线程中，需要让主线程弹窗并拿到用户是否继续的选择：\n"
            "connect(worker, &Worker::askConfirm, ui, &MainUI::promptUser, Qt::BlockingQueuedConnection);";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "qt_object_tree";
        t.framework = "Qt";
        t.category = "Qt 01. 核心机制与底层哲学";
        t.name = "QObject 对象树与自动内存管理";
        t.tag = "零内存泄漏的核心法则";
        t.isVisualInteractive = false;
        t.apiSignature = "QWidget *child = new QWidget(parent); // 声明父子所有权\n// 当 parent 析构时，child 会被全自动逐层释放！";
        t.docSummary = "Qt 构建了一套层级式的父子对象所有权树。当任何一个 `QObject` 被 `delete` 析构时，它的析构函数会自动遍历并递归 `delete` 它的所有子对象！";
        t.docParams = "• <b>parent 指针:</b> 指定父亲。在 GUI 体系中，挂载布局管理器（`layout->addWidget(child)`）也会自动将 child 的父对象设为该窗口。";
        t.usageTiming = "所有 UI 控件生命周期维护、插件化生命周期托管、防止 C++ 内存泄漏。";
        t.bestPractices = "① <b>黄金法则：</b>只要继承自 `QObject` 的类通过 `new` 在堆上创建，并传了 `parent`，就<b>绝对不需要手动写 `delete`</b>！<br>"
                          "② <b>绝命陷阱：</b>千万不要把局部栈对象（`QWidget child;`）传给已有的堆父对象，当栈对象提前析构时会导致父对象析构时发生<b>二次释放崩溃（Double Free）</b>！";
        t.codeSnippet = 
            "// 正确典范：无需任何手动 delete\n"
            "void setupWindow() {\n"
            "    QWidget *window = new QWidget(); // 顶层窗口\n"
            "    QVBoxLayout *layout = new QVBoxLayout(window); // layout 父为 window\n"
            "    QPushButton *btn = new QPushButton(\"提交\", window); // btn 父为 window\n"
            "    layout->addWidget(btn);\n"
            "    \n"
            "    delete window; // 一键递归安全销毁 window、layout、btn！\n"
            "}";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "qt_event_filters";
        t.framework = "Qt";
        t.category = "Qt 01. 核心机制与底层哲学";
        t.name = "事件派发管线与事件过滤器 (eventFilter)";
        t.tag = "全局无侵入式事件拦截与快捷键";
        t.isVisualInteractive = false;
        t.apiSignature = "void installEventFilter(QObject *filterObj);\nbool eventFilter(QObject *watched, QEvent *event) override;";
        t.docSummary = "Qt 事件系统的顶级拦截器。事件从操作系统进入后，依次经过 <code>QCoreApplication::notify()</code> -> <b>事件过滤器 eventFilter()</b> -> <code>event()</code> -> 具体事件处理函数（如 <code>keyPressEvent()</code>）。";
        t.docParams = "• <b>返回值:</b> 返回 <code>true</code> 代表“该事件已被我吃掉并处理完毕，停止继续往下分发”；返回 <code>false</code> 代表“放行，继续传递给目标控件”。";
        t.usageTiming = "全局全局快捷键捕获、给别人的第三方控件添加鼠标悬停动效、点击视窗外任意空白处自动收起下拉面板。";
        t.bestPractices = "由于所有的鼠标移动、重绘、键盘事件都会高频涌入 `eventFilter`，在过滤器内部严禁执行耗时操作，必须先通过 `event->type()` 快速过滤类型！";
        t.codeSnippet = 
            "// 典型应用：点击弹窗外部空白区域自动关闭弹窗\n"
            "bool MainWindow::eventFilter(QObject *watched, QEvent *event) {\n"
            "    if (event->type() == QEvent::MouseButtonPress) {\n"
            "        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);\n"
            "        if (m_popupCard && m_popupCard->isVisible()) {\n"
            "            if (!m_popupCard->geometry().contains(mouseEvent->pos())) {\n"
            "                m_popupCard->hide(); // 点击弹窗外部，自动收起\n"
            "                return true; // 拦截事件\n"
            "            }\n"
            "        }\n"
            "    }\n"
            "    return QMainWindow::eventFilter(watched, event); // 放行其他事件\n"
            "}\n\n"
            "// 在初始化时挂载过滤器：\n"
            "qApp->installEventFilter(this);";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "qt_threading_worker";
        t.framework = "Qt";
        t.category = "Qt 01. 核心机制与底层哲学";
        t.name = "QThread 生产级多线程架构";
        t.tag = "告别重写 run，使用 moveToThread";
        t.isVisualInteractive = false;
        t.apiSignature = "worker->moveToThread(thread);\nthread->start();";
        t.docSummary = "官方强烈推崇的<b>“工作者对象（Worker）+ moveToThread”</b>范式。让工作者对象活着在子线程的事件循环里，彻底解决跨线程资源竞争与死锁。";
        t.docParams = "• <b>moveToThread:</b> 将该对象的所有槽函数、定时器的执行上下文转移到指定的子线程中执行。";
        t.usageTiming = "执行耗时的 OpenCV 图像识别算法、海量文件解析、TCP 持续收发数据，<b>严禁在 UI 线程执行耗时超过 16ms 的代码</b>（否则界面必卡死掉帧）。";
        t.bestPractices = "① <b>绝对不要在子线程中直接调用任何 QWidget 界面组件</b>！Qt 明确规定 GUI 必须在主线程操作；子线程只能通过 `emit signal()` 异步把数据送回主线程！<br>"
                          "② 释放线程的标准三部曲：`thread->quit(); thread->wait();`。";
        t.codeSnippet = 
            "// 工业级 Worker 多线程范式：\n"
            "class VisionWorker : public QObject {\n"
            "    Q_OBJECT\n"
            "public slots:\n"
            "    void doHeavyMatching(const cv::Mat &screen) {\n"
            "        // 在子线程跑耗时 200ms 的算法\n"
            "        cv::Mat result = runAlgorithm(screen);\n"
            "        emit matchFinished(result); // 信号安全送回主线程\n"
            "    }\n"
            "signals:\n"
            "    void matchFinished(const cv::Mat &result);\n"
            "};\n\n"
            "// 在主线程调度：\n"
            "QThread *thread = new QThread();\n"
            "VisionWorker *worker = new VisionWorker();\n"
            "worker->moveToThread(thread);\n"
            "connect(this, &MainWindow::requestMatch, worker, &VisionWorker::doHeavyMatching);\n"
            "connect(worker, &VisionWorker::matchFinished, this, &MainWindow::onResultReady);\n"
            "thread->start();";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "qt_timer_system";
        t.framework = "Qt";
        t.category = "Qt 01. 核心机制与底层哲学";
        t.name = "QTimer 定时器体系与防抖节流";
        t.tag = "高精度事件驱动与搜索防抖";
        t.isVisualInteractive = false;
        t.apiSignature = "QTimer::singleShot(200, this, &MainWindow::doSearch); // 单次触发\ntimer->start(16); // 60 FPS 循环心跳";
        t.docSummary = "Qt 事件循环集成的定时器设施。支持循环触发、单次触发（singleShot）、不同精度等级（精确到毫秒级或节能粗粒度）。";
        t.docParams = "• <b>Qt::PreciseTimer:</b> 毫秒级精度，适合视频渲染心跳与物理引擎步进。<br>"
                      "• <b>Qt::CoarseTimer:</b> 节能模式，允许 5% 浮动。<br>"
                      "• <b>防抖 (Debounce):</b> 每次输入重置定时器，用户停止输入 300ms 后才执行搜索，避免暴击后台。";
        t.usageTiming = "搜索框联想实时检索、动画帧率控制、相机拉流帧率心跳、长任务超市超时中断。";
        t.bestPractices = "多线程场景下，`QTimer` 必须在其宿主所属的线程内 `start()`，在非所属线程调 `start()` 会报警告失效。";
        t.codeSnippet = 
            "// 工业级搜索框防抖动实战：\n"
            "QTimer *searchDebounce = new QTimer(this);\n"
            "searchDebounce->setSingleShot(true);\n"
            "searchDebounce->setInterval(300); // 用户停止打字 300ms 后触发\n\n"
            "connect(searchDebounce, &QTimer::timeout, this, [this]() {\n"
            "    executeHeavySearch(searchEdit->text());\n"
            "});\n\n"
            "connect(searchEdit, &QLineEdit::textChanged, this, [searchDebounce]() {\n"
            "    searchDebounce->start(); // 每次按键重新倒计时 300ms！\n"
            "});";
        registerTopic(t);
    }

    // ========================================================
    // 10. Qt 02. 现代界面开发与渲染技术 (深度架构与机制)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_qss_engine";
        t.framework = "Qt";
        t.category = "Qt 02. 现代界面开发与渲染";
        t.name = "QSS 样式表引擎与暗黑模式换肤";
        t.tag = "高颜值现代桌面 UI 核心";
        t.isVisualInteractive = false;
        t.apiSignature = "qApp->setStyleSheet(\"QWidget { background: #0f172a; color: #f8fafc; }\");";
        t.docSummary = "Qt 封装的类似于 Web CSS 的界面描述语言。支持盒模型（Margin、Border、Padding、Content）、伪类选择器（`:hover`, `:pressed`, `:disabled`）、对象名选择器（`#MyCard`）。";
        t.docParams = "• <b>全局注入 vs 局部注入:</b> `qApp->setStyleSheet(...)` 全局继承生效；`widget->setStyleSheet(...)` 局部高优先级覆盖。";
        t.usageTiming = "系统夜间/白天模式动态跟随切换、现代扁平卡片风格定制、高质感按钮悬浮态设计。";
        t.bestPractices = "① 避免频繁调用 `setStyleSheet`，每次解析字符串样式会有重绘性能开销。推荐在启动时加载全套样式变量。<br>"
                          "② 必须为高精细度组件配置动态属性：`widget->setProperty(\"state\", \"danger\"); widget->style()->polish(widget);`。";
        t.codeSnippet = 
            "// 极高质感的现代卡片样式表 QSS 范式：\n"
            "QString modernDarkCardQss = R\"(\n"
            "    #ToolCard {\n"
            "        background-color: #1e293b;\n"
            "        border: 1px solid #334155;\n"
            "        border-radius: 12px;\n"
            "        padding: 16px;\n"
            "    }\n"
            "    #ToolCard:hover {\n"
            "        background-color: #273549;\n"
            "        border: 1px solid #38bdf8;\n"
            "    }\n"
            "    QPushButton {\n"
            "        background-color: #2563eb;\n"
            "        color: #ffffff;\n"
            "        border-radius: 6px;\n"
            "        padding: 8px 16px;\n"
            "        font-weight: bold;\n"
            "    }\n"
            "    QPushButton:hover {\n"
            "        background-color: #1d4ed8;\n"
            "    }\n"
            "    QPushButton:pressed {\n"
            "        background-color: #1e40af;\n"
            "    }\n"
            ")\";\n"
            "qApp->setStyleSheet(modernDarkCardQss);";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "qt_dynamic_properties";
        t.framework = "Qt";
        t.category = "Qt 02. 现代界面开发与渲染";
        t.name = "动态属性与样式重载 (Dynamic Properties)";
        t.tag = "setProperty + polish 状态驱动 UI";
        t.isVisualInteractive = false;
        t.apiSignature = "widget->setProperty(\"status\", \"error\");\nwidget->style()->unpolish(widget);\nwidget->style()->polish(widget);";
        t.docSummary = "利用 Qt 的动态属性机制与 QSS 属性选择器 `[status=\"error\"]` 实现解耦的状态化界面。无需在 C++ 代码中到处拼接写死颜色。";
        t.docParams = "• <b>setProperty:</b> 动态为对象附加键值对属性。<br>"
                      "• <b>polish / unpolish:</b> 强制刷新 Qt 样式引擎的渲染缓存，使最新的属性匹配生效。";
        t.usageTiming = "表单输入校验高亮（成功绿框、报错红框）、工位设备状态机切换（就绪/运行中/报警/停机）。";
        t.bestPractices = "改变 dynamic property 后，如果不调用 `unpolish` 和 `polish`，QSS 样式<b>不会自动刷新</b>！这是新手最容易遇到的“修改了属性样式却没变”的陷阱！";
        t.codeSnippet = 
            "// 1. QSS 定义属性选择器：\n"
            "/*\n"
            "QLineEdit[valid=\"true\"]  { border: 2px solid #10b981; }\n"
            "QLineEdit[valid=\"false\"] { border: 2px solid #ef4444; }\n"
            "*/\n\n"
            "// 2. C++ 业务逻辑动态驱动状态切换：\n"
            "void setFieldValidity(QLineEdit *edit, bool isValid) {\n"
            "    edit->setProperty(\"valid\", isValid ? \"true\" : \"false\");\n"
            "    // 强制通知样式引擎刷新渲染：\n"
            "    edit->style()->unpolish(edit);\n"
            "    edit->style()->polish(edit);\n"
            "}";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "qt_qpainter_graphics";
        t.framework = "Qt";
        t.category = "Qt 02. 现代界面开发与渲染";
        t.name = "QPainter 2D 绘图与双缓冲技术";
        t.tag = "自定义高帧率控件与视窗";
        t.isVisualInteractive = false;
        t.apiSignature = "void paintEvent(QPaintEvent *event) override {\n    QPainter p(this);\n    p.setRenderHint(QPainter::Antialiasing);\n    // 绘制几何图形或图像\n}";
        t.docSummary = "Qt 底层 2D 绘图引擎。支持矢量几何图形、渐变填充、文字排版、以及离屏双缓冲机制（消灭画面撕裂与闪烁）。";
        t.docParams = "• <b>QPainter::Antialiasing:</b> 强制开启抗锯齿，使边缘极其丝滑平顺。<br>• <b>坐标变换:</b> `p.translate()`, `p.scale()`, `p.rotate()` 轻松实现几何缩放旋转。";
        t.usageTiming = "工业仪器仪表盘、动态曲线图表、视频播放器画面首帧渲染、截屏选区矩形拖拽绘制。";
        t.bestPractices = "① `QPainter` 只能在 `paintEvent(QPaintEvent*)` 生命周期内创建，在其他成员函数中实例化会报错失效。<br>"
                          "② 触发重绘必须调用 `this->update()`，它会智能合并多次无效重绘请求，千万不要手动直接调 `paintEvent`！";
        t.codeSnippet = 
            "// 工业仪表盘圆形进度条自定义绘制：\n"
            "void GaugeWidget::paintEvent(QPaintEvent *) {\n"
            "    QPainter painter(this);\n"
            "    painter.setRenderHint(QPainter::Antialiasing); // 开启抗锯齿\n\n"
            "    int side = qMin(width(), height());\n"
            "    painter.setViewport((width() - side)/2, (height() - side)/2, side, side);\n"
            "    painter.setWindow(-100, -100, 200, 200); // 映射为中心对称坐标系\n\n"
            "    // 绘制背景底环\n"
            "    painter.setPen(QPen(QColor(\"#334155\"), 10));\n"
            "    painter.drawArc(-80, -80, 160, 160, 0, 360 * 16);\n\n"
            "    // 绘制彩色动态进度环\n"
            "    painter.setPen(QPen(QColor(\"#38bdf8\"), 10, Qt::SolidLine, Qt::RoundCap));\n"
            "    painter.drawArc(-80, -80, 160, 160, 90 * 16, -m_progressAngle * 16);\n"
            "}";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "qt_property_animation";
        t.framework = "Qt";
        t.category = "Qt 02. 现代界面开发与渲染";
        t.name = "动效与属性动画 (QPropertyAnimation)";
        t.tag = "QEasingCurve 缓动曲线物理回弹实战";
        t.isVisualInteractive = false;
        t.apiSignature = "QPropertyAnimation *anim = new QPropertyAnimation(target, \"pos\");\nanim->setDuration(300);\nanim->setEasingCurve(QEasingCurve::OutBack);\nanim->start();";
        t.docSummary = "Qt 强大的属性动效引擎。只要类中通过 <code>Q_PROPERTY</code> 声明了属性及其 setter 方法，就可以驱动其在指定时间内平滑插值过渡。";
        t.docParams = "• <b>QEasingCurve:</b> 缓动曲线（如 <code>OutBack</code> 弹性物理回弹、<code>InOutQuad</code> 缓入缓出平滑、<code>Linear</code> 匀速）。<br>"
                      "• <b>QParallelAnimationGroup:</b> 组合多个动画同时并发执行。";
        t.usageTiming = "侧边栏平滑展开/收起抽屉动效、悬浮卡片上浮高亮动效、通知横幅自顶部滑入淡入。";
        t.bestPractices = "给 `geometry` 或 `pos` 做动画时，父容器必须使用绝对定位或在动画期间断开布局约束，否则布局管理器会与动画发生强行拉扯撕扯闪烁。";
        t.codeSnippet = 
            "// 卡片物理弹性悬浮动效实现：\n"
            "void triggerCardBounce(QWidget *card) {\n"
            "    QRect startGeo = card->geometry();\n"
            "    QRect targetGeo = startGeo.translated(0, -12); // 上浮 12 像素\n\n"
            "    QPropertyAnimation *anim = new QPropertyAnimation(card, \"geometry\");\n"
            "    anim->setDuration(350);\n"
            "    anim->setStartValue(startGeo);\n"
            "    anim->setEndValue(targetGeo);\n"
            "    anim->setEasingCurve(QEasingCurve::OutBack); // 物理过冲回弹效果！\n"
            "    anim->start(QAbstractAnimation::DeleteWhenStopped);\n"
            "}";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "qt_responsive_layout";
        t.framework = "Qt";
        t.category = "Qt 02. 现代界面开发与渲染";
        t.name = "弹性布局与伸缩因子 (Layout & Stretch Factor)";
        t.tag = "addStretch 与 setStretch 彻底消灭文本截断";
        t.isVisualInteractive = false;
        t.apiSignature = "layout->addWidget(leftNav, 0); // 伸缩权重 0\nlayout->addWidget(mainContent, 1); // 伸缩权重 1，占据所有多余空间\nlayout->addStretch();";
        t.docSummary = "Qt 强大的响应式布局系统（QHBoxLayout、QVBoxLayout、QGridLayout、QSplitter）。通过合理分配 Stretch 比例与 SizePolicy 策略，自适应任何 DPI 与分辨率。";
        t.docParams = "• <b>addStretch():</b> 插入一个弹性弹簧，将相邻控件压到边缘。<br>"
                      "• <b>setStretchFactor:</b> 设置分割条或布局中子项的宽度膨胀配比。<br>"
                      "• <b>setWordWrap(true):</b> 文本长段落自动折行，防止卡片横向撑爆截断。";
        t.usageTiming = "响应式自适应多端窗口设计、保证高分屏（4K）与 1080P 下组件不重叠、文字内容无论多长绝不截断。";
        t.bestPractices = "展示长文本的 `QLabel` 必须开启 `setWordWrap(true)`，同时卡片内禁止写死 `setFixedHeight()`，应结合 `QScrollArea` 容纳流式内容。";
        t.codeSnippet = 
            "// 黄金响应式双栏工作台布局范式：\n"
            "QWidget *container = new QWidget(this);\n"
            "QHBoxLayout *layout = new QHBoxLayout(container);\n"
            "layout->setContentsMargins(12, 12, 12, 12);\n"
            "layout->setSpacing(10);\n\n"
            "// 左侧固定导航区：\n"
            "QWidget *leftNav = new QWidget();\n"
            "leftNav->setFixedWidth(280);\n\n"
            "// 右侧自适应主视窗（配合滚动区彻底防截断）：\n"
            "QScrollArea *scrollArea = new QScrollArea();\n"
            "scrollArea->setWidgetResizable(true);\n\n"
            "layout->addWidget(leftNav, 0);        // 权重 0：锁定宽度\n"
            "layout->addWidget(scrollArea, 1);     // 权重 1：吞纳全部剩余拉伸空间";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "qt_frameless_window";
        t.framework = "Qt";
        t.category = "Qt 02. 现代界面开发与渲染";
        t.name = "现代化无边框沉浸式窗口 (Frameless Window)";
        t.tag = "Windows 原生阴影、窗口拖拽与无感缩放";
        t.isVisualInteractive = false;
        t.apiSignature = "setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint);\n// 或借助 Windows DwmExtendFrameIntoClientArea 注入原生阴影";
        t.docSummary = "打造现代化消费级桌面客户端的必备技术。剥离 Windows 原生白顶标题栏，自定义标题栏、自绘窗口阴影、实现拖拽标题栏移动与窗口八向边缘无感拉伸。";
        t.docParams = "• <b>WM_NCHITTEST:</b> Windows 底层原生击中测试，返回 HTCAPTION、HTLEFT、HTRIGHT 等，直接享受系统原生贴边分屏与平滑缩放。<br>"
                      "• <b>DwmExtendFrameIntoClientArea:</b> 借助 DWM 实现硬件加速窗口模糊与柔和环境阴影。";
        t.usageTiming = "现代工业级视觉套件、类似 VS Code / Discord 的深色一体化沉浸式主视窗。";
        t.bestPractices = "纯 Qt 模拟鼠标拖拽拉伸在低配机器上会有轻微卡顿，Windows 平台推荐在 `nativeEvent(QByteArray, void*, qintptr*)` 中直接接管 `WM_NCHITTEST`，性能与系统原生完全一致！";
        t.codeSnippet = 
            "// 轻量级纯 Qt 跨平台标题栏拖拽移动范式：\n"
            "void TitleBar::mousePressEvent(QMouseEvent *event) {\n"
            "    if (event->button() == Qt::LeftButton) {\n"
            "        m_isPressed = true;\n"
            "        m_startPos = event->globalPosition().toPoint() - window()->frameGeometry().topLeft();\n"
            "        event->accept();\n"
            "    }\n"
            "}\n\n"
            "void TitleBar::mouseMoveEvent(QMouseEvent *event) {\n"
            "    if (m_isPressed && (event->buttons() & Qt::LeftButton)) {\n"
            "        window()->move(event->globalPosition().toPoint() - m_startPos);\n"
            "        event->accept();\n"
            "    }\n"
            "}\n\n"
            "void TitleBar::mouseReleaseEvent(QMouseEvent *event) {\n"
            "    m_isPressed = false;\n"
            "}";
        registerTopic(t);
    }

    // ========================================================
    // 11. Qt 03. Model / View 架构与高级数据展现 (工业级架构)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_model_view_architecture";
        t.framework = "Qt";
        t.category = "Qt 03. Model / View 架构与高级数据展现";
        t.name = "Model / View 架构设计哲学 (QAbstractItemModel)";
        t.tag = "海量千万级数据高性能渲染的解耦基石";
        t.isVisualInteractive = false;
        t.apiSignature = "class CustomTableModel : public QAbstractTableModel {\n    int rowCount(const QModelIndex &parent = QModelIndex()) const override;\n    int columnCount(const QModelIndex &parent = QModelIndex()) const override;\n    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;\n};";
        t.docSummary = "Qt 的 Model/View 架构将底层业务数据结构与前台 UI 渲染完全解耦。传统控件（如 QTableWidget）每个单元格都是一个 QTableWidgetItem 对象，当数据达到十万行时会消耗数百兆内存并导致界面卡死。而基于 QAbstractItemModel 的自定义模型，<b>无论数据有多少千万行，内存开销为零额外冗余，视图（QTableView）只在滚动到可视区域时才动态向 Model 索取当前可见单元格的 data()！</b>";
        t.docParams = "• <b>rowCount & columnCount:</b> 返回虚拟数据矩阵的行数和列数。<br>"
                      "• <b>data(index, role):</b> 视图渲染时的回调核心。根据不同 role（DisplayRole 文本、DecorationRole 图标/颜色、ToolTipRole 提示、BackgroundRole 底色）返回对应 QVariant。";
        t.usageTiming = "工业检测日志实时滚动瀑布流、千万级点云/标注坐标列表、大型传感器时序监控数据展现。";
        t.bestPractices = "① 数据发生变更时（如插入新行），务必在修改底层数据容器前调用 <code>beginInsertRows(...)</code>，并在修改后调用 <code>endInsertRows()</code>，通知所有观察者视图平滑局部重绘，严禁暴力的 `modelReset()`！<br>"
                          "② `data()` 函数由视图高频调用，严禁在其中执行数据库查询或复杂算法运算。";
        t.codeSnippet = 
            "// 生产级高性能千万行虚拟只读表格模型范式：\n"
            "#include <QAbstractTableModel>\n"
            "#include <vector>\n\n"
            "struct VisionInspectionItem {\n"
            "    int id;\n"
            "    QString timestamp;\n"
            "    double confidence;\n"
            "    bool passed;\n"
            "};\n\n"
            "class VisionLogModel : public QAbstractTableModel {\n"
            "    Q_OBJECT\n"
            "    std::vector<VisionInspectionItem> m_records;\n\n"
            "public:\n"
            "    explicit VisionLogModel(QObject *parent = nullptr) : QAbstractTableModel(parent) {}\n\n"
            "    int rowCount(const QModelIndex &) const override { return static_cast<int>(m_records.size()); }\n"
            "    int columnCount(const QModelIndex &) const override { return 4; }\n\n"
            "    QVariant data(const QModelIndex &index, int role) const override {\n"
            "        if (!index.isValid() || index.row() >= static_cast<int>(m_records.size())) return {};\n"
            "        const auto &rec = m_records[index.row()];\n\n"
            "        if (role == Qt::DisplayRole) {\n"
            "            switch (index.column()) {\n"
            "                case 0: return rec.id;\n"
            "                case 1: return rec.timestamp;\n"
            "                case 2: return QString::number(rec.confidence, 'f', 2) + \"%\";\n"
            "                case 3: return rec.passed ? \"PASS\" : \"FAIL\";\n"
            "            }\n"
            "        } else if (role == Qt::ForegroundRole && index.column() == 3) {\n"
            "            return rec.passed ? QColor(\"#16a34a\") : QColor(\"#dc2626\");\n"
            "        }\n"
            "        return {};\n"
            "    }\n\n"
            "    void appendRecord(const VisionInspectionItem &item) {\n"
            "        int row = static_cast<int>(m_records.size());\n"
            "        beginInsertRows(QModelIndex(), row, row);\n"
            "        m_records.push_back(item);\n"
            "        endInsertRows();\n"
            "    }\n"
            "};";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "qt_custom_delegate";
        t.framework = "Qt";
        t.category = "Qt 03. Model / View 架构与高级数据展现";
        t.name = "自定义单元格委托代理 (QStyledItemDelegate)";
        t.tag = "表格无损定制绘制、内嵌状态药丸与进度条";
        t.isVisualInteractive = false;
        t.apiSignature = "void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;\nQWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;";
        t.docSummary = "Delegate（委托）负责 Model/View 体系中<b>单个单元格的绘制与交互编辑</b>。初学者常犯的错误是调用 <code>setCellWidget()</code> 为每个单元格塞入一个实体 QPushButton 或 QProgressBar，导致数千个 QWidget 实例将操作系统 GDI 句柄与内存耗尽。使用 QStyledItemDelegate 仅需在 <code>paint()</code> 中利用 QPainter 轻量绘制形状，<b>0 个多余 Widget，瞬间拥有惊艳的胶囊状态药丸与进度条！</b>";
        t.docParams = "• <b>paint():</b> 单元格绘制入口。传入的 `option.rect` 指定了该单元格的精确绘制像素矩形。<br>"
                      "• <b>createEditor():</b> 当用户双击单元格时动态创建编辑器（如 QSpinBox、QComboBox），编辑完成后自动销毁。";
        t.usageTiming = "在表格中优雅展示质检状态胶囊徽章（绿色 PASS / 红色 NG）、算法匹配相似度百分比彩色进度条、操作按钮组。";
        t.bestPractices = "在 `paint()` 绘制前务必调用 `painter->save()`，绘制结束后调用 `painter->restore()`，防止画笔颜色与坐标变换污染后续单元格的绘制上下文。";
        t.codeSnippet = 
            "// 生产级药丸徽章（Status Badge）高性能自绘委托：\n"
            "#include <QStyledItemDelegate>\n"
            "#include <QPainter>\n\n"
            "class StatusBadgeDelegate : public QStyledItemDelegate {\n"
            "public:\n"
            "    using QStyledItemDelegate::QStyledItemDelegate;\n\n"
            "    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {\n"
            "        QString status = index.data(Qt::DisplayRole).toString();\n"
            "        painter->save();\n"
            "        painter->setRenderHint(QPainter::Antialiasing);\n\n"
            "        // 药丸矩形计算（垂直居中，自适应内边距）\n"
            "        QRect badgeRect = option.rect.adjusted(10, 6, -10, -6);\n"
            "        bool isPass = (status == \"PASS\");\n\n"
            "        // 绘制圆角背景药丸\n"
            "        painter->setPen(Qt::NoPen);\n"
            "        painter->setBrush(isPass ? QColor(\"#dcfce7\") : QColor(\"#fee2e2\"));\n"
            "        painter->drawRoundedRect(badgeRect, badgeRect.height() / 2, badgeRect.height() / 2);\n\n"
            "        // 绘制高对比度文字\n"
            "        painter->setPen(isPass ? QColor(\"#15803d\") : QColor(\"#b91c1c\"));\n"
            "        QFont font = painter->font();\n"
            "        font.setBold(true);\n"
            "        painter->setFont(font);\n"
            "        painter->drawText(badgeRect, Qt::AlignCenter, status);\n\n"
            "        painter->restore();\n"
            "    }\n"
            "};\n\n"
            "// 挂载到 QTableView 指定列：\n"
            "// tableView->setItemDelegateForColumn(3, new StatusBadgeDelegate(tableView));";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "qt_sort_filter_proxy";
        t.framework = "Qt";
        t.category = "Qt 03. Model / View 架构与高级数据展现";
        t.name = "代理模型与动态搜索排序 (QSortFilterProxyModel)";
        t.tag = "零性能损耗的即时搜索与多列智能排序";
        t.isVisualInteractive = false;
        t.apiSignature = "QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);\nproxy->setSourceModel(sourceModel);\nview->setModel(proxy);";
        t.docSummary = "QSortFilterProxyModel 充当原始 Model 与 View 之间的“智能透镜”。它不需要对底层原始数据做任何排序或拷贝，仅通过维护映射索引，就能以毫秒级响应实现<b>全局模糊搜索过滤、正则表达式筛选、任意列升降序排序</b>。";
        t.docParams = "• <b>setSourceModel:</b> 挂载原始数据源模型。<br>"
                      "• <b>setFilterCaseSensitivity:</b> 设置大小写敏感度。<br>"
                      "• <b>setFilterKeyColumn(-1):</b> 设置为 -1 时触发全列全局智能搜索；设定具体数字时仅过滤指定列。";
        t.usageTiming = "软件顶部搜索框实时输入搜索日志、点击表头点击字段排序、多状态下拉组合过滤。";
        t.bestPractices = "View 中选中的索引是 `proxyIndex`，若要修改底层业务数据，必须调用 <code>proxy->mapToSource(proxyIndex)</code> 转换为原始 Model 坐标，严禁混用两者坐标！";
        t.codeSnippet = 
            "// 生产级全局搜索与表头双向排序范式：\n"
            "#include <QSortFilterProxyModel>\n"
            "#include <QLineEdit>\n"
            "#include <QTableView>\n\n"
            "void setupSearchableTable(QAbstractItemModel *rawModel, QTableView *tableView, QLineEdit *searchBox) {\n"
            "    auto *proxyModel = new QSortFilterProxyModel(tableView);\n"
            "    proxyModel->setSourceModel(rawModel);\n"
            "    \n"
            "    // 全列不区分大小写模糊匹配\n"
            "    proxyModel->setFilterKeyColumn(-1);\n"
            "    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);\n\n"
            "    // 启用点击表头自动升降序排序\n"
            "    tableView->setModel(proxyModel);\n"
            "    tableView->setSortingEnabled(true);\n"
            "    tableView->sortByColumn(0, Qt::AscendingOrder);\n\n"
            "    // 搜索框文本变化联动过滤\n"
            "    QObject::connect(searchBox, &QLineEdit::textChanged, proxyModel, &QSortFilterProxyModel::setFilterWildcard);\n"
            "}";
        registerTopic(t);
    }

    // ========================================================
    // 12. Qt 04. 工业网络通信与进程间 IPC (工业级通信)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_network_http";
        t.framework = "Qt";
        t.category = "Qt 04. 工业网络通信与进程间 IPC";
        t.name = "网络请求管理器与异步客户端 (QNetworkAccessManager)";
        t.tag = "非阻塞异步 HTTP(S) 请求与 RESTful API";
        t.isVisualInteractive = false;
        t.apiSignature = "QNetworkAccessManager *mgr = new QNetworkAccessManager(this);\nQNetworkReply *reply = mgr->post(request, jsonData);";
        t.docSummary = "Qt 官方异步非阻塞网络通信核心引擎。采用底层事件循环驱动，发送任何 GET/POST 请求都不会阻塞 UI 主线程。支持 SSL/TLS、流式大文件断点续传、Cookie 管理与 RESTful API 数据上报。";
        t.docParams = "• <b>QNetworkRequest:</b> 请求封装器，可设置目标 URL、请求头（如 Content-Type、Authorization Token）。<br>"
                      "• <b>QNetworkReply:</b> 异步响应流。通过 `finished` 信号通知完成，具备 `readAll()` 读取数据流。";
        t.usageTiming = "视觉质检结果上报 MES/ERP 工厂系统、软件在线检查更新版本、从云端算法服务器拉取最新检测模型权重。";
        t.bestPractices = "① <b>生命周期陷阱：</b>在 `reply` 的 `finished` 槽函数处理完毕后，必须调用 <code>reply->deleteLater()</code>，否则高频网络请求会导致严重内存泄露！<br>"
                          "② 全局通常仅需保留<b>一个</b> `QNetworkAccessManager` 实例，复用底层 HTTP/2 连接池。";
        t.codeSnippet = 
            "// 生产级异步 RESTful POST 上报缺陷质检数据：\n"
            "#include <QNetworkAccessManager>\n"
            "#include <QNetworkRequest>\n"
            "#include <QNetworkReply>\n"
            "#include <QJsonDocument>\n"
            "#include <QJsonObject>\n"
            "#include <QDateTime>\n\n"
            "void reportDefectToCloud(QNetworkAccessManager *netMgr, const QString &partId, double score) {\n"
            "    QUrl url(\"https://mes.factory.local/api/v1/defect-report\");\n"
            "    QNetworkRequest request(url);\n"
            "    request.setHeader(QNetworkRequest::ContentTypeHeader, \"application/json\");\n"
            "    request.setRawHeader(\"Authorization\", \"Bearer token_secret_xyz\");\n\n"
            "    QJsonObject json;\n"
            "    json[\"part_id\"] = partId;\n"
            "    json[\"defect_score\"] = score;\n"
            "    json[\"timestamp\"] = QDateTime::currentSecsSinceEpoch();\n\n"
            "    QNetworkReply *reply = netMgr->post(request, QJsonDocument(json).toJson());\n"
            "    QObject::connect(reply, &QNetworkReply::finished, [reply]() {\n"
            "        reply->deleteLater(); // 确保安全销毁防泄露\n"
            "        if (reply->error() == QNetworkReply::NoError) {\n"
            "            qDebug() << \"上报成功：\" << reply->readAll();\n"
            "        } else {\n"
            "            qWarning() << \"网络异常：\" << reply->errorString();\n"
            "        }\n"
            "    });\n"
            "}";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "qt_tcp_socket";
        t.framework = "Qt";
        t.category = "Qt 04. 工业网络通信与进程间 IPC";
        t.name = "高性能 TCP 工业网络与二进制防粘包协议 (QTcpSocket)";
        t.tag = "工业以太网、QDataStream 与固定消息头分包";
        t.isVisualInteractive = false;
        t.apiSignature = "QTcpSocket socket;\nsocket.connectToHost(\"192.168.1.50\", 5000);\nconnect(&socket, &QTcpSocket::readyRead, this, &Client::onReadyRead);";
        t.docSummary = "TCP 是面向流的协议（Byte Stream），在操作系统传输层不存在“单条消息边界”，多包合并（粘包）或单包截断（拆包）是工业网络的必然常态。掌握<b>“4字节包头消息长度（quint32 payloadSize）+ 动态循环缓冲读取”</b>是工业机器视觉通信的合格标尺！";
        t.docParams = "• <b>readyRead 信号:</b> 只要网络缓冲区收到新字节就触发，并不代表一条完整消息刚好到齐。<br>"
                      "• <b>bytesAvailable():</b> 当前 socket 输入缓冲区中已累积的未读字节数。";
        t.usageTiming = "与 PLC（欧姆龙/西门子/三菱）工业以太网通讯交互触发相机信号、视觉工控机向机械臂发送三维抓取坐标点位。";
        t.bestPractices = "写入数据必须前置写入消息总包体大小，读取时若 `bytesAvailable() < payloadSize` 则坚决保留在缓冲区中等待下一次 `readyRead`，绝对严禁盲目直接 `readAll()` 当作完整指令解析！";
        t.codeSnippet = 
            "// 工业标准级防粘包 TCP 数据接收器范式：\n"
            "#include <QTcpSocket>\n"
            "#include <QDataStream>\n\n"
            "class IndustrialTcpClient : public QObject {\n"
            "    Q_OBJECT\n"
            "    QTcpSocket m_socket;\n"
            "    quint32 m_expectedBlockSize = 0;\n\n"
            "public:\n"
            "    IndustrialTcpClient() {\n"
            "        connect(&m_socket, &QTcpSocket::readyRead, this, &IndustrialTcpClient::handleIncomingData);\n"
            "    }\n\n"
            "private slots:\n"
            "    void handleIncomingData() {\n"
            "        QDataStream in(&m_socket);\n"
            "        in.setVersion(QDataStream::Qt_6_5);\n\n"
            "        while (true) {\n"
            "            // 阶段 1：先读取 4 字节消息体长度头\n"
            "            if (m_expectedBlockSize == 0) {\n"
            "                if (m_socket.bytesAvailable() < static_cast<qint64>(sizeof(quint32))) {\n"
            "                    return; // 头都还没凑齐，等待下批字节涌入\n"
            "                }\n"
            "                in >> m_expectedBlockSize;\n"
            "            }\n\n"
            "            // 阶段 2：校验缓冲区是否已容纳完整的一包数据体\n"
            "            if (m_socket.bytesAvailable() < m_expectedBlockSize) {\n"
            "                return; // 数据体尚未到齐，退出等待\n"
            "            }\n\n"
            "            // 阶段 3：完整消费该包，重置状态以处理粘在后面的下一包\n"
            "            QByteArray packetData = m_socket.read(m_expectedBlockSize);\n"
            "            m_expectedBlockSize = 0;\n"
            "            processCompleteMessage(packetData);\n"
            "        }\n"
            "    }\n\n"
            "    void processCompleteMessage(const QByteArray &data) {\n"
            "        // 保证拿到的一定是 100% 完整干净的单个数据包！\n"
            "    }\n"
            "};";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "qt_shared_memory";
        t.framework = "Qt";
        t.category = "Qt 04. 工业网络通信与进程间 IPC";
        t.name = "跨进程共享内存与互斥守护 (QSharedMemory / QSystemSemaphore)";
        t.tag = "零拷贝大图像帧跨进程微秒级极速共享";
        t.isVisualInteractive = false;
        t.apiSignature = "QSharedMemory shm(\"VisionGlobalMemoryKey\");\nshm.create(1920 * 1080 * 3);\nshm.lock();\nmemcpy(shm.data(), frame.data, size);\nshm.unlock();";
        t.docSummary = "工业机器视觉处理中，4K 60FPS 的原始图像每秒产生近 1.5GB 数据流。如果采用 TCP/管道/Socket 进行跨进程传递，严重的内存二次拷贝与序列化会导致 CPU 满载掉帧。<code>QSharedMemory</code> 允许独立进程将<b>同一段物理内存直接映射到各自的虚拟地址空间</b>，写方写入与读方读取处于同一个物理芯片块，实现<b>真正的零拷贝微秒级通信！</b>";
        t.docParams = "• <b>setKey:</b> 系统级全局唯一标识符键名。<br>"
                      "• <b>lock() / unlock():</b> 进程级互斥锁，保证同一时刻只有一个进程读写共享内存，防止脏读。";
        t.usageTiming = "相机独立底层采集守护进程（Daemon）与上层 Qt 算法界面的高速图像共享、多算法进程并行抢占式推理。";
        t.bestPractices = "当进程异常崩溃退出时，共享内存段在 Linux/Windows 上可能残留锁定状态。若 `attach()` 失败，可先调用 `detach()` 清除僵尸引用再尝试挂载。";
        t.codeSnippet = 
            "// 跨进程图像共享写入方标准范式：\n"
            "#include <QSharedMemory>\n"
            "#include <opencv2/core.hpp>\n\n"
            "void publishFrameToIPC(const cv::Mat &bgrFrame) {\n"
            "    static QSharedMemory shm(\"VISION_FRAME_SHM_KEY\");\n"
            "    int requiredSize = static_cast<int>(bgrFrame.total() * bgrFrame.elemSize());\n\n"
            "    // 首次创建共享内存块\n"
            "    if (!shm.isAttached()) {\n"
            "        if (!shm.create(requiredSize)) {\n"
            "            shm.attach(); // 若其他进程已创建，则直接挂载\n"
            "        }\n"
            "    }\n\n"
            "    // 加锁并零拷贝拷贝内存\n"
            "    if (shm.lock()) {\n"
            "        char *to = static_cast<char*>(shm.data());\n"
            "        memcpy(to, bgrFrame.data, requiredSize);\n"
            "        shm.unlock(); // 解锁放行读者进程\n"
            "    }\n"
            "}";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "qt_process_launch";
        t.framework = "Qt";
        t.category = "Qt 04. 工业网络通信与进程间 IPC";
        t.name = "外部子进程异步调度与管道交互 (QProcess)";
        t.tag = "非阻塞唤起 Python / FFmpeg / 命令行工具";
        t.isVisualInteractive = false;
        t.apiSignature = "QProcess *proc = new QProcess(this);\nproc->start(\"ffmpeg.exe\", QStringList() << \"-i\" << ...);\nconnect(proc, &QProcess::readyReadStandardOutput, this, &Handler::onLogReady);";
        t.docSummary = "Qt 强大的外部进程管理组件。在工业生产中，很多优秀工具（如 FFmpeg 推流、Python 脚本、Halcon 离线引擎、系统硬件诊断工具）以独立可执行文件存在。<code>QProcess</code> 允许我们异步拉起子进程，全双工重定向其标准输入/输出/错误流（stdin/stdout/stderr），实时获取运行日志并捕获异常退出码。";
        t.docParams = "• <b>start(program, arguments):</b> 异步启动外部程序，绝不阻塞 UI 主界面。<br>"
                      "• <b>readyReadStandardOutput:</b> 外部程序产生控制台打印时触发通知。<br>"
                      "• <b>write(data):</b> 向外部进程的 stdin 输入管道追加输入命令。";
        t.usageTiming = "一键调用 Python YOLO 模型训练脚本并实时在界面滚动日志、调用 FFmpeg 对工业录制视频进行 H.264 硬件编码转码。";
        t.bestPractices = "① 严禁使用系统的 `system(\"...\")` 阻塞调用！必须使用 `QProcess` 异步驱动；<br>"
                          "② 主程序关闭时应先温和触发 `proc->terminate()`，超时未退出再调用 `proc->kill()` 强制收尾，防止残留孤儿进程占用系统端口。";
        t.codeSnippet = 
            "// 生产级非阻塞调用外部 Python 脚本并流式收集输出：\n"
            "#include <QProcess>\n"
            "#include <QDebug>\n\n"
            "void runExternalPythonWorker(QObject *parent) {\n"
            "    auto *proc = new QProcess(parent);\n"
            "    \n"
            "    // 监听子进程标准输出日志流\n"
            "    QObject::connect(proc, &QProcess::readyReadStandardOutput, [proc]() {\n"
            "        QByteArray output = proc->readAllStandardOutput();\n"
            "        qDebug() << \"[Python Output]:\" << QString::fromUtf8(output);\n"
            "    });\n\n"
            "    // 监听执行完毕信号并自释放\n"
            "    QObject::connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),\n"
            "        [proc](int exitCode, QProcess::ExitStatus status) {\n"
            "            qDebug() << \"子进程执行完毕，退出码:\" << exitCode;\n"
            "            proc->deleteLater();\n"
            "        });\n\n"
            "    // 异步拉起（入参安全传递，无命令注入隐患）\n"
            "    proc->start(\"python.exe\", QStringList() << \"scripts/deep_eval.py\" << \"--threshold\" << \"0.85\");\n"
            "}";
        registerTopic(t);
    }

    // ========================================================
    // 13. Qt 05. 高性能交互与图形视图 (工业级绘图与标注)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_graphics_view";
        t.framework = "Qt";
        t.category = "Qt 05. 高性能交互与图形视图";
        t.name = "交互式图形视图架构 (QGraphicsView / QGraphicsScene)";
        t.tag = "十万图元流畅平移缩放、ROI 自由拖拽与形变";
        t.isVisualInteractive = false;
        t.apiSignature = "QGraphicsScene *scene = new QGraphicsScene(this);\nQGraphicsView *view = new QGraphicsView(scene, this);\nview->setDragMode(QGraphicsView::ScrollHandDrag);\nscene->addItem(new QGraphicsPixmapItem(pixmap));";
        t.docSummary = "Qt 的 Graphics View 框架专为<b>超大规模图元渲染与高级几何交互</b>而生。基于 BSP（二叉空间分割树）空间索引，即便是十万个独立图元（如点、线、圆、多边形框），缩放与平移仍可保持 60 FPS 流畅满帧！支持图元点击选定、八向拖拽拉伸变形、碰撞检测、图层分组管理。";
        t.docParams = "• <b>QGraphicsScene:</b> 逻辑世界画布容器，存储所有图元的几何拓扑信息。<br>"
                      "• <b>QGraphicsView:</b> 摄像机视窗窗口，支持鼠标滚轮平滑缩放、手势平移、OpenGL 硬件渲染加速。<br>"
                      "• <b>QGraphicsItem:</b> 自定义图元基类，重写 <code>boundingRect()</code> 和 <code>paint()</code> 即可实现任何几何标注。";
        t.usageTiming = "机器视觉工业相机标定 ROI 自定义框选（矩形/旋转矩形/自由多边形）、缺陷检测结果标记热力图、大型电子地图绘制。";
        t.bestPractices = "① 大图显示时务必设置 `view->setViewport(new QOpenGLWidget())`，借助 GPU 显卡硬件流水线秒级吞吐 4K 图像；<br>"
                          "② `boundingRect()` 必须严格包裹图元的全部外边缘（包括画笔宽度），否则移动或缩放图元时会留下未刷新的花屏残影。";
        t.codeSnippet = 
            "// 工业机器视觉可交互自由拖拽 ROI 矩形框范式：\n"
            "#include <QGraphicsView>\n"
            "#include <QGraphicsScene>\n"
            "#include <QGraphicsRectItem>\n\n"
            "void setupInteractiveVisionViewport(QWidget *parent, const QPixmap &inspectionImg) {\n"
            "    auto *scene = new QGraphicsScene(parent);\n"
            "    auto *view = new QGraphicsView(scene, parent);\n\n"
            "    // 1. 底层大图\n"
            "    scene->addPixmap(inspectionImg);\n\n"
            "    // 2. 注入一个可被鼠标拖拽、可被选中的 ROI 检测框\n"
            "    auto *roiItem = new QGraphicsRectItem(QRectF(100, 100, 200, 150));\n"
            "    roiItem->setPen(QPen(QColor(\"#38bdf8\"), 2));\n"
            "    roiItem->setBrush(QColor(56, 189, 248, 40)); // 半透明浅蓝底色\n"
            "    roiItem->setFlags(QGraphicsItem::ItemIsMovable | \n"
            "                      QGraphicsItem::ItemIsSelectable | \n"
            "                      QGraphicsItem::ItemSendsGeometryChanges);\n"
            "    scene->addItem(roiItem);\n\n"
            "    // 3. 开启视窗抗锯齿与拖拽漫游\n"
            "    view->setRenderHint(QPainter::Antialiasing);\n"
            "    view->setDragMode(QGraphicsView::RubberBandDrag);\n"
            "}";
        registerTopic(t);
    }

    // ========================================================
    // 14. Qt 02. 多线程、并发与异步流水线 (高阶拓展)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_concurrent_run";
        t.framework = "Qt";
        t.category = "Qt 02. 多线程、并发与异步流水线";
        t.name = "函数式高阶并发计算 (QtConcurrent::run / QFutureWatcher)";
        t.tag = "免写 QThread / 线程池自适应多核密集型任务";
        t.isVisualInteractive = false;
        t.apiSignature = "QFuture<cv::Mat> future = QtConcurrent::run(QThreadPool::globalInstance(), [img]() {\n    return heavyVisionAlgorithm(img);\n});\nQFutureWatcher<cv::Mat> *watcher = new QFutureWatcher<cv::Mat>(this);\nconnect(watcher, &QFutureWatcher::finished, this, &Handler::onDone);";
        t.docSummary = "工业视觉算法通常包含计算密集型任务（如大津阈值分割、双边降噪滤波、三维点云法向量估计）。编写独立的 <code>QThread</code> 或 <code>Worker</code> 类往往需要繁琐的信号槽样板代码。<code>QtConcurrent::run</code> 提供了现代 C++ 高阶函数式并发接口，能够将任意 Lambda 表达式或函数直接提交到底层全局线程池 <code>QThreadPool</code> 执行。配合 <code>QFuture</code> 异步凭据和 <code>QFutureWatcher</code> 观察者，能在后台多核并发计算完成的瞬间，以 Qt 信号槽安全回弹主 UI 线程，彻底消除界面卡顿，代码量大幅缩减。";
        t.docParams = "• <b>QtConcurrent::run:</b> 模板函数，接受可选的 QThreadPool*、可调用对象（Lambda/函数/成员函数指针）及可变参数列表，返回 QFuture&lt;T&gt;。<br>"
                      "• <b>QFutureWatcher&lt;T&gt;:</b> 桥接 QFuture 与 Qt 事件循环的观察者 QObject，提供 <code>finished()</code>、<code>canceled()</code>、<code>progressValueChanged()</code> 等信号。<br>"
                      "• <b>watcher-&gt;result():</b> 获取异步任务返回值，在 <code>finished()</code> 信号槽中调用是瞬时且绝对安全的。";
        t.usageTiming = "单次、按需触发的耗时计算任务，如按下“一键图像去噪”、“深度特征提取”、“加载大图点云”、“导出高分辨率缺陷报表”等无需长期常驻线程的场景。";
        t.bestPractices = "① 严禁在 <code>QtConcurrent::run</code> 的子线程中直接访问、读写任何 QWidget 或调用 UI 界面函数（Qt 严格限制 GUI 只能在主线程更新）；<br>"
                          "② 后台任务若持有外部指针，需警惕生命周期悬空（生命周期陷阱），推荐在 Lambda 捕获时采用值拷贝（如按值捕获 <code>cv::Mat</code> 或智能指针）；<br>"
                          "③ 对于持续性高频数据流（如 60FPS 相机采集流水线），优先使用常驻 <code>QThread + Worker</code>，避免频繁入队调度开销；而对于批量独立离散任务，推荐 <code>QtConcurrent::mapped</code> / <code>filtered</code>。";
        t.codeSnippet = 
            "// 工业级高阶异步并发算法执行范式：\n"
            "#include <QtConcurrent/QtConcurrent>\n"
            "#include <QFutureWatcher>\n"
            "#include <opencv2/imgproc.hpp>\n\n"
            "void runAsyncVisionTask(QWidget *parent, const cv::Mat &inputMat) {\n"
            "    // 1. 创建异步结果监视器\n"
            "    auto *watcher = new QFutureWatcher<cv::Mat>(parent);\n\n"
            "    // 2. 挂接计算完成事件（主线程安全接收回调）\n"
            "    QObject::connect(watcher, &QFutureWatcher<cv::Mat>::finished, [watcher, parent]() {\n"
            "        cv::Mat processed = watcher->result(); // 零阻塞提取运算结果\n"
            "        // 安全更新主窗口 UI 或渲染视图\n"
            "        watcher->deleteLater(); // 自动释放监视器内存\n"
            "    });\n\n"
            "    // 3. 将密集型算法抛入全局线程池异步并发执行\n"
            "    QFuture<cv::Mat> future = QtConcurrent::run(QThreadPool::globalInstance(), [inputMat]() {\n"
            "        cv::Mat dst;\n"
            "        cv::bilateralFilter(inputMat, dst, 9, 75, 75); // 耗时双边滤波\n"
            "        return dst;\n"
            "    });\n\n"
            "    watcher->setFuture(future);\n"
            "}";
        registerTopic(t);
    }

    // ========================================================
    // 15. Qt 06. 系统工程与现代桌面架构 (持久化配置与热重载)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_qsettings_config";
        t.framework = "Qt";
        t.category = "Qt 06. 系统工程与现代桌面架构";
        t.name = "工业配方与持久化配置管理 (QSettings)";
        t.tag = "跨平台 INI/注册表无缝读写、相机参数与窗口状态记忆";
        t.isVisualInteractive = false;
        t.apiSignature = "QSettings settings(\"config/recipe.ini\", QSettings::IniFormat);\nsettings.beginGroup(\"CameraParameters\");\nsettings.setValue(\"exposureTime\", 5000);\nsettings.endGroup();\nint exp = settings.value(\"CameraParameters/exposureTime\", 3000).toInt();";
        t.docSummary = "工业检测软件必须具备断电或重启后的状态自愈能力，包括相机曝光/增益配方、检测工件公差阈值、主窗口几何尺寸（全屏/最大化/悬浮位置）以及最近打开的图像历史记录。<code>QSettings</code> 提供了优雅的无锁持久化抽象，支持以分组层次（Group）在 Windows 注册表或跨平台 INI 文件中进行无缝读写，原生支持 <code>QByteArray</code>（可直接将 <code>saveGeometry()</code> 和 <code>saveState()</code> 序列化入配置）。";
        t.docParams = "• <b>QSettings::IniFormat:</b> 指定存储为通用的纯文本 INI 配置文件，便于现场工程师通过记事本直接排查与修改参数。<br>"
                      "• <b>beginGroup / endGroup:</b> 层次化前缀作用域，避免键名（Key）冲突，逻辑结构清晰。<br>"
                      "• <b>value(key, defaultValue):</b> 读取指定键的值，当该键在文件中不存在时自动回退为默认值，极具防御性。";
        t.usageTiming = "软件启动时恢复上次关闭时的窗口布局与分割条比例；切换不同产品检测型号时加载对应参数配方；保存用户暗黑/明亮主题偏好与快捷键绑定。";
        t.bestPractices = "① 工业视觉工控机现场常遇非正常断电，建议在写入关键配方后显式调用 <code>settings.sync()</code> 强制刷盘，防止操作系统写缓存丢失；<br>"
                          "② 窗口几何持久化：关闭事件重写 <code>closeEvent</code>，执行 <code>settings.setValue(\"geometry\", saveGeometry()); settings.setValue(\"windowState\", saveState());</code>，在构造函数中通过 <code>restoreGeometry()</code> 还原，可完美兼容多显示器插拔情况。";
        t.codeSnippet = 
            "// 工业级相机参数与系统状态持久化配方管理器：\n"
            "#include <QSettings>\n"
            "#include <QWidget>\n\n"
            "class RecipeManager {\n"
            "public:\n"
            "    static void saveCameraRecipe(const QString &recipePath, double exposure, double gain) {\n"
            "        QSettings s(recipePath, QSettings::IniFormat);\n"
            "        s.beginGroup(\"SensorProfile\");\n"
            "        s.setValue(\"ExposureUs\", exposure);\n"
            "        s.setValue(\"GainDb\", gain);\n"
            "        s.setValue(\"AutoWhiteBalance\", true);\n"
            "        s.endGroup();\n"
            "        s.sync(); // 强制刷入硬件磁盘\n"
            "    }\n\n"
            "    static void restoreWindowLayout(QWidget *window, const QString &iniPath) {\n"
            "        QSettings s(iniPath, QSettings::IniFormat);\n"
            "        s.beginGroup(\"MainWindow\");\n"
            "        if (s.contains(\"geometry\")) {\n"
            "            window->restoreGeometry(s.value(\"geometry\").toByteArray());\n"
            "        }\n"
            "        s.endGroup();\n"
            "    }\n"
            "};";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "qt_file_watcher";
        t.framework = "Qt";
        t.category = "Qt 06. 系统工程与现代桌面架构";
        t.name = "文件目录监控与热重载体系 (QFileSystemWatcher)";
        t.tag = "深度学习模型热更新、生产配方变更瞬时热响应";
        t.isVisualInteractive = false;
        t.apiSignature = "QFileSystemWatcher *watcher = new QFileSystemWatcher(this);\nwatcher->addPath(\"models/yolo_defect.onnx\");\nconnect(watcher, &QFileSystemWatcher::fileChanged, this, &ModelManager::onHotReload);";
        t.docSummary = "在自动化无人值守质检工位中，算法工程师经常需要在线更换 ONNX/TensorRT 模型权重文件，或者 MES 制造执行系统会在特定共享文件夹下动态推送最新的待测产品批次 JSON 配置。<code>QFileSystemWatcher</code> 封装了操作系统的原生内核通知机制（Windows 的 <code>ReadDirectoryChangesW</code> / Linux 的 <code>inotify</code>），无需开启死循环轮询即可毫秒级捕获文件或目录的创建、修改、重命名与删除。";
        t.docParams = "• <b>addPath / addPaths:</b> 注册需要监控的目标文件绝对路径或目录绝对路径。<br>"
                      "• <b>fileChanged(const QString &path):</b> 被监控的文件内容被外部修改并落盘时触发。<br>"
                      "• <b>directoryChanged(const QString &path):</b> 被监控的目录内有新文件生成、文件删除或子目录变动时触发。";
        t.usageTiming = "工业相机采集端目录“热入库”自动触发质检流水线；算法模型/配方文件修改后免重启应用热重载；日志目录自动滚动与清理。";
        t.bestPractices = "① <b>重命名覆盖原子写入防丢失：</b>许多现代化编辑器保存文件时采用“写临时文件 -> 删除原文件 -> 重命名覆盖”的原子策略。这会导致原始文件被短暂删除，<code>QFileSystemWatcher</code> 会自动将其从监控列表中移除！因此在 <code>fileChanged</code> 槽函数中，若 <code>QFile::exists(path)</code>，务必再次调用 <code>watcher->addPath(path)</code> 重新注册；<br>"
                          "② <b>防抖定时器（Debounce）：</b>大文件分块写入可能触发多次连续的文件修改事件，应使用单次定时器 <code>QTimer::singleShot(250, ...)</code> 进行防抖合并，等待文件完全写闭合后再读取。";
        t.codeSnippet = 
            "// 模型与配方文件热重载工业防御性实现：\n"
            "#include <QFileSystemWatcher>\n"
            "#include <QTimer>\n"
            "#include <QFile>\n"
            "#include <QDebug>\n\n"
            "class ModelHotReloader : public QObject {\n"
            "public:\n"
            "    explicit ModelHotReloader(const QString &modelPath, QObject *parent = nullptr)\n"
            "        : QObject(parent), m_path(modelPath), m_watcher(new QFileSystemWatcher(this)) {\n"
            "        m_watcher->addPath(m_path);\n"
            "        connect(m_watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &path) {\n"
            "            // 重新挂载监控（防原子重命名脱落）\n"
            "            if (QFile::exists(path) && !m_watcher->files().contains(path)) {\n"
            "                m_watcher->addPath(path);\n"
            "            }\n"
            "            // 250ms 防抖合并多次连续写入\n"
            "            QTimer::singleShot(250, this, [this]() {\n"
            "                qDebug() << \"[HotReload] 检测到新模型权重，重新加载推理引擎...\" << m_path;\n"
            "            });\n"
            "        });\n"
            "    }\n"
            "private:\n"
            "    QString m_path;\n"
            "    QFileSystemWatcher *m_watcher;\n"
            "};";
        registerTopic(t);
    }
}
