#include "KnowledgeRegistry.h"

void KnowledgeRegistry::registerQtArchitectureTopics() {
    {
        KnowledgeTopic t;
        t.id = "qt_property_animation";
        t.framework = "Qt";
        t.category = "Qt 06. 系统工程与现代桌面架构";
        t.name = "现代化流畅动效与缓动插值 (QPropertyAnimation / QEasingCurve)";
        t.tag = "无阻塞视觉过渡、侧边栏丝滑抽屉与告警呼吸灯";
        t.isVisualInteractive = false;
        t.apiSignature = "QPropertyAnimation *anim = new QPropertyAnimation(widget, \"maximumWidth\");\nanim->setDuration(300);\nanim->setEasingCurve(QEasingCurve::OutCubic);\nanim->setStartValue(60);\nanim->setEndValue(260);\nanim->start(QAbstractAnimation::DeleteWhenStopped);";
        t.docSummary = "传统工业软件界面往往显得僵硬死板，状态切换突兀。Qt 的动画框架基于元对象系统（<code>Q_PROPERTY</code>），无需手写任何定时器循环插值代码，即可让任意 Qt 属性（如控件尺寸 <code>geometry</code>、透明度 <code>opacity</code>、背景颜色、最大宽度等）在时间线上平滑过渡。通过搭配 <code>QEasingCurve</code> 提供的 40 余种物理级数学缓动曲线（如弹簧回弹 <code>OutBack</code>、惯性平滑 <code>OutCubic</code>、弹性碰撞 <code>OutBounce</code>），能为工业桌面注入现代消费级软件的丝滑体验。";
        t.docParams = "• <b>targetObject & propertyName:</b> 目标动画对象及其注册的 Q_PROPERTY 属性名（如 \"pos\"、\"size\"、\"windowOpacity\" 等）。<br>"
                      "• <b>setDuration(ms):</b> 动画持续时长（毫秒），一般微交互在 150ms~350ms 为人体工学最舒适区间。<br>"
                      "• <b>setEasingCurve:</b> 插值数学曲线，如 <code>QEasingCurve::InOutQuad</code> 实现物理重力加速减速。";
        t.usageTiming = "侧边导航栏折叠与展开收起抽屉动效；视觉质检超差（NG）时界面的红色半透明呼吸告警灯；数据卡片 hover 悬浮升起浮雕阴影过度。";
        t.bestPractices = "① 布局系统兼容：当控件位于 <code>QLayout</code>（如 <code>QVBoxLayout</code>）内部时，直接对 <code>geometry</code> 做动画可能被父布局管理器强行重置重算。推荐对 <code>minimumWidth</code> / <code>maximumWidth</code> 或自定义的 <code>QGraphicsOpacityEffect</code> 的 <code>opacity</code> 属性做动画；<br>"
                          "② 内存泄漏防范：动态创建的一次性过渡动画，建议设置 <code>anim->start(QAbstractAnimation::DeleteWhenStopped)</code>，使动画播放结束后自动安全销毁对象。";
        t.codeSnippet = 
            "// 工业级抽屉侧边栏折叠/展开丝滑物理动画：\n"
            "#include <QPropertyAnimation>\n"
            "#include <QEasingCurve>\n"
            "#include <QWidget>\n\n"
            "void animateDrawerSidebar(QWidget *sidebar, bool expand) {\n"
            "    auto *anim = new QPropertyAnimation(sidebar, \"maximumWidth\");\n"
            "    anim->setDuration(280);\n"
            "    anim->setEasingCurve(QEasingCurve::OutCubic);\n"
            "    \n"
            "    int startW = sidebar->width();\n"
            "    int targetW = expand ? 260 : 64;\n"
            "    anim->setStartValue(startW);\n"
            "    anim->setEndValue(targetW);\n"
            "    \n"
            "    // 动画完成后自动销毁指针，零内存泄漏\n"
            "    anim->start(QAbstractAnimation::DeleteWhenStopped);\n"
            "}";
        registerTopic(t);
    }

    // ========================================================
    // 16. Qt 05. 高性能交互与图形视图 (自绘与原生拖放)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_qpainter_advanced";
        t.framework = "Qt";
        t.category = "Qt 05. 高性能交互与图形视图";
        t.name = "高级几何自绘与抗锯齿变换 (QPainter 高级特性)";
        t.tag = "视口变换、线性渐变、亚像素高精度工业仪表与曲线";
        t.isVisualInteractive = false;
        t.apiSignature = "void paintEvent(QPaintEvent *event) override {\n    QPainter p(this);\n    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);\n    p.translate(width() / 2.0, height() / 2.0);\n    p.rotate(m_angle);\n    p.drawPath(path);\n}";
        t.docSummary = "当标准 Qt 控件无法满足特定工业视觉需求（如圆形压力/转速仪表盘、示波器实时动态正弦波形、带公差上下限的尺寸直方图、旋转角度亚像素指针）时，重写 <code>paintEvent</code> 配合 <code>QPainter</code> 提供了无与伦比的自绘控制力。结合坐标矩阵变换（<code>translate</code>、<code>rotate</code>、<code>scale</code>）、高阶画刷渐变（<code>QLinearGradient</code>、<code>QRadialGradient</code>）以及贝塞尔矢量路径（<code>QPainterPath</code>），可以绘制出超越原生控件的精美工业 HMI 交互界面。";
        t.docParams = "• <b>setRenderHint(QPainter::Antialiasing):</b> 开启几何抗锯齿，彻底消除折线与圆弧边缘的粗糙锯齿伪影。<br>"
                      "• <b>translate / rotate / scale:</b> 2D 仿射矩阵变换，将绘制原点移动到组件中心，使旋转计算完全脱离复杂的三角函数坐标换算。<br>"
                      "• <b>save() / restore():</b> 状态栈保护，成对保存与恢复画笔、画刷、变换矩阵与裁剪区，确保组件模块化绘制不受污染。";
        t.usageTiming = "工业相机帧率实时波动折线图、转盘式多工位分度盘状态监控、带刻度与游标卡尺的亚像素量测标注覆盖层。";
        t.bestPractices = "① 严禁在 <code>paintEvent</code> 内部构造重型对象（如解析字体、读取磁盘图片、重新计算上万个点的复杂算法），应在外部完成数据预计算，绘图事件中仅做纯粹的渲染管线消费；<br>"
                          "② 动态波形高频重绘时，调用 <code>update(dirtyRect)</code> 局部刷新脏矩形，而非盲目 <code>update()</code> 全量重刷，可显著降低 GPU 与 CPU 渲染负载。";
        t.codeSnippet = 
            "// 工业级圆形速度与旋转刻度盘自绘范式：\n"
            "#include <QWidget>\n"
            "#include <QPainter>\n"
            "#include <QPainterPath>\n\n"
            "class IndustrialGaugeWidget : public QWidget {\n"
            "protected:\n"
            "    void paintEvent(QPaintEvent *event) override {\n"
            "        Q_UNUSED(event);\n"
            "        QPainter p(this);\n"
            "        p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);\n\n"
            "        // 1. 将原点移至组件正中心\n"
            "        p.translate(width() / 2.0, height() / 2.0);\n"
            "        int side = qMin(width(), height());\n"
            "        p.scale(side / 200.0, side / 200.0); // 坐标归一化到 [-100, 100]\n\n"
            "        // 2. 绘制弧形渐变外圈\n"
            "        QConicalGradient grad(0, 0, -90);\n"
            "        grad.setColorAt(0.0, QColor(\"#10b981\"));\n"
            "        grad.setColorAt(0.7, QColor(\"#f59e0b\"));\n"
            "        grad.setColorAt(1.0, QColor(\"#ef4444\"));\n"
            "        p.setPen(QPen(QBrush(grad), 8, Qt::SolidLine, Qt::RoundCap));\n"
            "        p.drawArc(-80, -80, 160, 160, -30 * 16, 240 * 16);\n\n"
            "        // 3. 绘制旋转指针\n"
            "        p.save();\n"
            "        p.rotate(45.0); // 指向目标角度\n"
            "        p.setPen(Qt::NoPen);\n"
            "        p.setBrush(QColor(\"#38bdf8\"));\n"
            "        static const QPoint needle[3] = { QPoint(-4, 0), QPoint(4, 0), QPoint(0, -75) };\n"
            "        p.drawConvexPolygon(needle, 3);\n"
            "        p.restore();\n"
            "    }\n"
            "};";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "qt_drag_and_drop";
        t.framework = "Qt";
        t.category = "Qt 05. 高性能交互与图形视图";
        t.name = "原生桌面拖放与 MIME 交互系统 (Drag & Drop / QMimeData)";
        t.tag = "文件直接拖入视觉检测视窗即刻分析、跨控件交互";
        t.isVisualInteractive = false;
        t.apiSignature = "void dragEnterEvent(QDragEnterEvent *event) override {\n    if (event->mimeData()->hasUrls()) event->acceptProposedAction();\n}\nvoid dropEvent(QDropEvent *event) override {\n    for (const QUrl &url : event->mimeData()->urls()) {\n        QString file = url.toLocalFile();\n    }\n}";
        t.docSummary = "现代桌面应用程序极度依赖直觉化操作。用户期望将 Windows 文件资源管理器中的工件图片、缺陷样本或者标定文件直接拖拽并释放到软件视窗内立即进行推理。Qt 提供了深植于操作系统底层的 Drag and Drop（拖放）体系，通过 <code>QMimeData</code> 封装标准互联网 MIME 协议（支持文本、富文本、URI 列表、任意自定义二进制序列化载荷），实现进程间与控件间的自由拖拽对接。";
        t.docParams = "• <b>setAcceptDrops(true):</b> 必须在目标控件构造函数中显式开启接收拖放权限。<br>"
                      "• <b>dragEnterEvent(event):</b> 鼠标拖着数据悬停进入控件边缘瞬间触发，用于检查数据类型并决定是否点亮释放准许手势。<br>"
                      "• <b>dropEvent(event):</b> 鼠标松开释放时触发，解包 QMimeData 并提取本地文件路径 <code>toLocalFile()</code>。";
        t.usageTiming = "直接拖入 BMP/PNG/TIFF 工业检测图至视窗即刻执行缺陷识别；在视觉算法算子树中拖拽算子节点重排执行先后顺序；拖拽标定参数文件快速载入。";
        t.bestPractices = "① 必须在 <code>dragEnterEvent</code> 和 <code>dragMoveEvent</code> 中显式调用 <code>event->acceptProposedAction()</code>，否则系统鼠标指针会显示禁止放置图标，且不会派发随后的 <code>dropEvent</code>；<br>"
                          "② <b>文件路径跨平台兼容：</b>从 <code>event->mimeData()->urls()</code> 取出的 <code>QUrl</code> 必须通过 <code>url.toLocalFile()</code> 转换为本地操作系统的原生文件路径，直接调用 <code>toString()</code> 会带 <code>file:///</code> 前缀从而导致 <code>cv::imread</code> 读取失败。";
        t.codeSnippet = 
            "// 工业视觉视窗支持文件直接拖拽载入的生产级实现：\n"
            "#include <QLabel>\n"
            "#include <QDragEnterEvent>\n"
            "#include <QDropEvent>\n"
            "#include <QMimeData>\n"
            "#include <QFileInfo>\n\n"
            "class VisionDropTargetLabel : public QLabel {\n"
            "public:\n"
            "    explicit VisionDropTargetLabel(QWidget *parent = nullptr) : QLabel(parent) {\n"
            "        setAcceptDrops(true); // 必须显式激活接收拖放权限\n"
            "        setText(\"【可将图片文件直接拖拽至此处立即检测】\");\n"
            "        setAlignment(Qt::AlignCenter);\n"
            "    }\n\n"
            "protected:\n"
            "    void dragEnterEvent(QDragEnterEvent *event) override {\n"
            "        if (event->mimeData()->hasUrls()) {\n"
            "            event->acceptProposedAction(); // 接受拖拽悬停，显示绿色加号手势\n"
            "        }\n"
            "    }\n\n"
            "    void dropEvent(QDropEvent *event) override {\n"
            "        const auto urls = event->mimeData()->urls();\n"
            "        if (urls.isEmpty()) return;\n\n"
            "        QString localPath = urls.first().toLocalFile(); // 提取真实操作系统本地路径\n"
            "        QFileInfo info(localPath);\n"
            "        QString ext = info.suffix().toLower();\n"
            "        if (ext == \"png\" || ext == \"jpg\" || ext == \"bmp\" || ext == \"tif\") {\n"
            "            qDebug() << \"成功接收并加载拖放图片:\" << localPath;\n"
            "            // 载入图片并通知算法处理流水线\n"
            "            event->acceptProposedAction();\n"
            "        }\n"
            "    }\n"
            "};";
        registerTopic(t);
    }

    // ========================================================
    // 17. Qt 03. 高性能 Model/View 架构 (自定义单元格委托)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_custom_delegate";
        t.framework = "Qt";
        t.category = "Qt 03. 高性能 Model/View 架构";
        t.name = "自定义项委托与单元格嵌入组件 (QStyledItemDelegate)";
        t.tag = "表格内嵌实时动态进度条、质检结论Badge与交互按钮";
        t.isVisualInteractive = false;
        t.apiSignature = "class StatusBadgeDelegate : public QStyledItemDelegate {\n    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;\n};\ntableView->setItemDelegateForColumn(2, new StatusBadgeDelegate(this));";
        t.docSummary = "在工业视觉 HMI 监控表格中，单调的文字展示已无法满足现代现场需求。例如需要直接在单元格内呈现彩色圆角 PASS/NG 胶囊徽章（Badge）、实时缺陷面积微型进度条，或者工件缩略图。<code>QStyledItemDelegate</code> 是 Qt Model/View 架构的核心定制器，重写 <code>paint()</code> 方法可在极低内存开销下批量自绘任何精美组件，避免为每行创建重型独立 QWidget 控件，即便十万行数据依然保持 60 FPS 流畅满帧！";
        t.docParams = "• <b>paint(painter, option, index):</b> 自定义渲染回调，`option.rect` 指定当前单元格的绘制矩形，`index.data()` 提取底层模型数据。<br>"
                      "• <b>sizeHint(option, index):</b> 声明单元格的理想长宽，防止自绘内容被截断。<br>"
                      "• <b>createEditor / setEditorData:</b> 用于在用户双击时临时弹出下拉框或微调器进行参数交互。";
        t.usageTiming = "工业质检流水明细表中的状态胶囊徽章渲染、多工位产量完成率进度条、表格内嵌缩略图与操作按钮。";
        t.bestPractices = "① 在 `paint()` 中绘制完成后，必须调用 `painter->restore()` 或保存画笔画刷状态，防止污染后续单元格的绘制；<br>"
                          "② 严禁在委托的 `paint()` 中执行任何耗时的格式转换或磁盘 I/O（如 `cv::imread`），图片必须在外部缓存为 `QPixmap` 后通过数据源传递。";
        t.codeSnippet = 
            "// 工业级质检状态胶囊徽章 (Badge) 自定义委托实现：\n"
            "#include <QStyledItemDelegate>\n"
            "#include <QPainter>\n"
            "#include <QPainterPath>\n\n"
            "class StatusBadgeDelegate : public QStyledItemDelegate {\n"
            "public:\n"
            "    explicit StatusBadgeDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}\n\n"
            "    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {\n"
            "        painter->save();\n"
            "        painter->setRenderHint(QPainter::Antialiasing);\n\n"
            "        QString status = index.data(Qt::DisplayRole).toString();\n"
            "        bool isPass = status.contains(\"PASS\");\n\n"
            "        // 计算居中胶囊区域\n"
            "        QRect badgeRect = option.rect.adjusted(8, 4, -8, -4);\n"
            "        QPainterPath path;\n"
            "        path.addRoundedRect(badgeRect, 4, 4);\n\n"
            "        painter->setPen(Qt::NoPen);\n"
            "        painter->setBrush(isPass ? QColor(\"#10b981\") : QColor(\"#ef4444\"));\n"
            "        painter->drawPath(path);\n\n"
            "        painter->setPen(QColor(\"#ffffff\"));\n"
            "        painter->setFont(QFont(\"Segoe UI\", 9, QFont::Bold));\n"
            "        painter->drawText(badgeRect, Qt::AlignCenter, status);\n\n"
            "        painter->restore();\n"
            "    }\n"
            "};";
        registerTopic(t);
    }

    // ========================================================
    // 18. Qt 01. 核心架构与元对象系统 (事件过滤器)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_event_filter";
        t.framework = "Qt";
        t.category = "Qt 01. 核心架构与元对象系统 (MOC / 信号槽)";
        t.name = "全局与对象事件过滤器 (eventFilter / installEventFilter)";
        t.tag = "工业扫码枪按键拦截、无干扰鼠标防抖与手势劫持";
        t.isVisualInteractive = false;
        t.apiSignature = "bool eventFilter(QObject *watched, QEvent *event) override {\n    if (event->type() == QEvent::KeyPress) {\n        QKeyEvent *ke = static_cast<QKeyEvent*>(event);\n        // 拦截条码枪输入缓冲区\n        return true; // 消费事件，阻止下发\n    }\n    return QObject::eventFilter(watched, event);\n}";
        t.docSummary = "在工业现场，外接硬件（如 USB 条码枪、脚踏开关、急停按钮）通常模拟键盘输入。若当前界面焦点不在特定输入框，扫码枪数据就会丢失或串入其他控件。Qt 的事件过滤器机制允许一个 <code>QObject</code> 在目标对象处理事件之前先行截获。将其安装在 <code>qApp</code> 上即可实现系统级全局按键监听与前置分发，是工控软件必不可少的架构利器。";
        t.docParams = "• <b>installEventFilter(filterObj):</b> 向目标对象注册监听器，目标接收的所有事件先流经 filterObj。<br>"
                      "• <b>eventFilter(watched, event):</b> 事件过滤核心虚函数。返回 `true` 表示吃掉该事件（阻断传播），返回 `false` 放行继续流动。";
        t.usageTiming = "工业 USB 扫码枪自动收集条码输入缓冲区、触摸屏误触物理防抖、全局快捷键捕获。";
        t.bestPractices = "事件过滤器必须极速执行完毕（严禁内部做网络请求、耗时计算或磁盘读写），否则会导致整个界面的鼠标和键盘响应发生肉眼可见的严重卡顿。";
        t.codeSnippet = 
            "// 工业扫码枪全局按键拦截器标准范式：\n"
            "#include <QObject>\n"
            "#include <QEvent>\n"
            "#include <QKeyEvent>\n"
            "#include <QTimer>\n"
            "#include <QDebug>\n\n"
            "class BarcodeScannerFilter : public QObject {\n"
            "    Q_OBJECT\n"
            "public:\n"
            "    explicit BarcodeScannerFilter(QObject *parent = nullptr) : QObject(parent) {}\n\n"
            "signals:\n"
            "    void barcodeScanned(const QString &barcode);\n\n"
            "protected:\n"
            "    bool eventFilter(QObject *watched, QEvent *event) override {\n"
            "        if (event->type() == QEvent::KeyPress) {\n"
            "            auto *ke = static_cast<QKeyEvent*>(event);\n"
            "            if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {\n"
            "                if (!m_buffer.isEmpty()) {\n"
            "                    emit barcodeScanned(m_buffer);\n"
            "                    m_buffer.clear();\n"
            "                    return true; // 拦截回车，避免触发界面其他默认按钮\n"
            "                }\n"
            "            } else if (!ke->text().isEmpty()) {\n"
            "                m_buffer.append(ke->text());\n"
            "            }\n"
            "        }\n"
            "        return QObject::eventFilter(watched, event);\n"
            "    }\n"
            "private:\n"
            "    QString m_buffer;\n"
            "};";
        registerTopic(t);
    }

    // ========================================================
    // 19. Qt 04. 工业网络通信与进程间 IPC (串口通信)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_serial_port";
        t.framework = "Qt";
        t.category = "Qt 04. 工业网络通信与进程间 IPC";
        t.name = "工业硬件串口与 PLC 协议总线 (QSerialPort)";
        t.tag = "RS232/RS485 异步全双工通讯、Modbus-RTU 校验与粘包处理";
        t.isVisualInteractive = false;
        t.apiSignature = "QSerialPort *serial = new QSerialPort(this);\nserial->setPortName(\"COM3\");\nserial->setBaudRate(QSerialPort::Baud115200);\nserial->setDataBits(QSerialPort::Data8);\nserial->setParity(QSerialPort::NoParity);\nserial->setStopBits(QSerialPort::OneStop);\nserial->open(QIODevice::ReadWrite);";
        t.docSummary = "工业现场相机光源控制器、光电传感器、下位机 PLC 多采用 RS232/RS485 串口连接。<code>QSerialPort</code> 提供了跨平台的异步硬件通信，结合非阻塞的 <code>readyRead</code> 信号驱动事件循环。配合环形缓冲区与超时防抖机制，能完美解决 Modbus-RTU 工业通讯中的 3.5 字符帧间隔超时判定与粘包分包难题。";
        t.docParams = "• <b>setBaudRate:</b> 设定波特率（常用 9600、19200、115200）。<br>"
                      "• <b>readyRead:</b> 底层 UART 接收 FIFO 缓冲区有数据到达时异步触发通知。<br>"
                      "• <b>write(const QByteArray &):</b> 向硬件写入控制帧。";
        t.usageTiming = "机器视觉工位给光源控制器发送亮度调节指令、与欧姆龙/西门子/三菱 PLC 进行握手交互。";
        t.bestPractices = "① 严禁在 GUI 线程中调用 `serial->waitForReadyRead()`（会冻结主界面鼠标和渲染）！必须使用信号槽非阻塞驱动；<br>"
                          "② 现场常遇强电磁干扰导致 USB 串口拔插断开，应监听 `errorOccurred` 信号，在设备掉线时执行断线重连重试。";
        t.codeSnippet = 
            "// 工业级串口全双工通信与帧解析标准范式：\n"
            "#include <QSerialPort>\n"
            "#include <QByteArray>\n"
            "#include <QDebug>\n\n"
            "class PLCHandler : public QObject {\n"
            "    Q_OBJECT\n"
            "public:\n"
            "    void initPort(const QString &portName) {\n"
            "        m_serial = new QSerialPort(this);\n"
            "        m_serial->setPortName(portName);\n"
            "        m_serial->setBaudRate(QSerialPort::Baud115200);\n"
            "        \n"
            "        connect(m_serial, &QSerialPort::readyRead, this, [this]() {\n"
            "            m_recvBuffer.append(m_serial->readAll());\n"
            "            // 针对工业协议（如以 0x0D 0x0A 换行结尾）进行拆包\n"
            "            while (m_recvBuffer.contains(\"\\r\\n\")) {\n"
            "                int idx = m_recvBuffer.indexOf(\"\\r\\n\");\n"
            "                QByteArray frame = m_recvBuffer.left(idx);\n"
            "                m_recvBuffer.remove(0, idx + 2);\n"
            "                processCompleteFrame(frame);\n"
            "            }\n"
            "        });\n"
            "        m_serial->open(QIODevice::ReadWrite);\n"
            "    }\n\n"
            "    void sendTriggerCommand() {\n"
            "        if (m_serial && m_serial->isOpen()) {\n"
            "            m_serial->write(\"TRIG:START\\r\\n\");\n"
            "        }\n"
            "    }\n"
            "private:\n"
            "    QSerialPort *m_serial = nullptr;\n"
            "    QByteArray m_recvBuffer;\n"
            "    void processCompleteFrame(const QByteArray &data) {\n"
            "        qDebug() << \"[PLC Frame]:\" << data;\n"
            "    }\n"
            "};";
        registerTopic(t);
    }

    // ========================================================
    // 20. Qt 03. 高性能 Model/View 架构 (多列过滤与排序)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_sort_filter_proxy";
        t.framework = "Qt";
        t.category = "Qt 03. 高性能 Model/View 架构";
        t.name = "多列实时筛选与虚拟多态排序 (QSortFilterProxyModel)";
        t.tag = "百万级工件质检数据秒级模糊查询、多维度正逆序重排";
        t.isVisualInteractive = false;
        t.apiSignature = "QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);\nproxy->setSourceModel(sourceModel);\nproxy->setFilterCaseSensitivity(Qt::CaseInsensitive);\nproxy->setFilterKeyColumn(-1); // 全列匹配\ntableView->setModel(proxy);";
        t.docSummary = "当底层数据源存储了数十万行视觉抽检流水账时，直接在原始 Model 中做数据增删排序会导致全量重绘卡顿。<code>QSortFilterProxyModel</code> 充当数据源与视图之间的轻量级“虚拟光学滤镜”，原始数据不发生任何物理位移，仅通过行号映射建立索引，实现毫秒级正则搜索与任意列点击升降序，代码量几乎为零！";
        t.docParams = "• <b>setSourceModel:</b> 挂接底层真实业务数据模型。<br>"
                      "• <b>setFilterRegularExpression:</b> 注入正则表达式进行动态智能过滤。<br>"
                      "• <b>filterAcceptsRow:</b> 可重写的虚函数，支持组合复杂业务逻辑（如“同时满足孔径超差且检验耗时>5ms”）。";
        t.usageTiming = "工业质检流水明细表实时多维度筛选（仅看合格、仅看特定产线、按工件批号模糊搜索）。";
        t.bestPractices = "当在 View 中获取选中项时，行号是经过 Proxy 映射的！如果需要修改底层真实数据，务必调用 `proxy->mapToSource(proxyIndex)` 转换回真实索引，否则会引发严重的数据错位修改 Bug！";
        t.codeSnippet = 
            "// 工业级质检流水多列代理过滤与排序集成：\n"
            "#include <QSortFilterProxyModel>\n"
            "#include <QTableView>\n"
            "#include <QLineEdit>\n\n"
            "void setupFilteredInspectionView(QTableView *view, QAbstractItemModel *rawModel, QLineEdit *searchEdit) {\n"
            "    auto *proxy = new QSortFilterProxyModel(view);\n"
            "    proxy->setSourceModel(rawModel);\n"
            "    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);\n"
            "    proxy->setFilterKeyColumn(-1); // -1 代表检索全表所有列数据\n\n"
            "    view->setModel(proxy);\n"
            "    view->setSortingEnabled(true); // 开启表头点击自动升降序\n\n"
            "    // 搜索框输入与模型实时联动过滤\n"
            "    QObject::connect(searchEdit, &QLineEdit::textChanged, proxy, &QSortFilterProxyModel::setFilterFixedString);\n"
            "}";
        registerTopic(t);
    }

    // ========================================================
    // 21. Qt 04. 工业网络通信与进程间 IPC (UDP 组播)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_udp_broadcast";
        t.framework = "Qt";
        t.category = "Qt 04. 工业网络通信与进程间 IPC";
        t.name = "局域网设备自发现与组播推流 (QUdpSocket)";
        t.tag = "GigE 工业相机 IP 自动探测、产线心跳多播与分布式同步";
        t.isVisualInteractive = false;
        t.apiSignature = "QUdpSocket *udp = new QUdpSocket(this);\nudp->bind(QHostAddress::AnyIPv4, 8888, QUdpSocket::ShareAddress);\nconnect(udp, &QUdpSocket::readyRead, this, &Discovery::onDatagram);";
        t.docSummary = "在分布式机器视觉系统中（如 8 台工控机协同拼图），设备之间需要自动寻址握手，并同步传送带编码器触发脉冲。<code>QUdpSocket</code> 提供微秒级低延迟的无连接单播、局域网全网广播（255.255.255.255）与组播（Multicast），零连接开销，是工业现场设备自发现（GenICam 相机探测协议）的标准基石。";
        t.docParams = "• <b>bind(port, ShareAddress):</b> 监听指定 UDP 端口，允许多个工控进程共享绑定同一端口。<br>"
                      "• <b>writeDatagram:</b> 发送无连接数据包。<br>"
                      "• <b>readPendingDatagrams:</b> 提取排队数据包与发送方 IP 和端口号。";
        t.usageTiming = "工控机多机视觉集群心跳广播、局域网相机设备快速扫描与绑定、产线流水线光电传感器触发同步广播。";
        t.bestPractices = "UDP 不保证包序与可靠送达，在网络拥堵时可能丢包。用于关键控制信号时，建议在应用层数据包头加入序列号与确认重传（ACK）机制。";
        t.codeSnippet = 
            "// 工业级局域网设备自发现心跳广播与监听：\n"
            "#include <QUdpSocket>\n"
            "#include <QNetworkDatagram>\n"
            "#include <QDebug>\n\n"
            "class VisionDeviceDiscoverer : public QObject {\n"
            "    Q_OBJECT\n"
            "public:\n"
            "    explicit VisionDeviceDiscoverer(quint16 port = 9000, QObject *parent = nullptr) : QObject(parent), m_port(port) {\n"
            "        m_socket = new QUdpSocket(this);\n"
            "        m_socket->bind(QHostAddress::AnyIPv4, m_port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);\n"
            "        connect(m_socket, &QUdpSocket::readyRead, this, [this]() {\n"
            "            while (m_socket->hasPendingDatagrams()) {\n"
            "                QNetworkDatagram datagram = m_socket->receiveDatagram();\n"
            "                qDebug() << \"[UDP Heartbeat]:\" << datagram.senderAddress().toString() << datagram.data();\n"
            "            }\n"
            "        });\n"
            "    }\n\n"
            "    void broadcastPresence(const QString &deviceName) {\n"
            "        QByteArray msg = QString(\"VISION_AGENT:%1\").arg(deviceName).toUtf8();\n"
            "        m_socket->writeDatagram(msg, QHostAddress::Broadcast, m_port);\n"
            "    }\n"
            "private:\n"
            "    QUdpSocket *m_socket = nullptr;\n"
            "    quint16 m_port;\n"
            "};";
        registerTopic(t);
    }

    // ========================================================
    // 22. Qt 05. 高性能交互与图形视图 (自定义控件规范)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_custom_widget";
        t.framework = "Qt";
        t.category = "Qt 05. 高性能交互与图形视图";
        t.name = "工业自定义控件封装范式 (Q_PROPERTY / QPainter / 独立样式)";
        t.tag = "可复用工业仪表盘、发光 LED 状态指示灯、可导出 Qt Designer 插件";
        t.isVisualInteractive = false;
        t.apiSignature = "class IndustrialStatusLed : public QWidget {\n    Q_OBJECT\n    Q_PROPERTY(QColor ledColor READ ledColor WRITE setLedColor NOTIFY ledColorChanged)\n    Q_PROPERTY(bool isBlinking READ isBlinking WRITE setBlinking)\n};";
        t.docSummary = "开发工业机器视觉上位机时，标准按钮与标签无法传达设备运行状态。封装具备专业感的高内聚独立控件（如三色报警灯、圆环压力计、双向滑块）是资深 Qt 工程师的看家本领。遵循 <code>Q_PROPERTY</code> 属性系统（便于动效驱动与属性检查器配置）、重写 <code>paintEvent</code> 与 <code>sizeHint</code>，可让自定义控件达到开箱即用的工业级复用标准。";
        t.docParams = "• <b>Q_PROPERTY:</b> 向元对象系统暴露属性，使得 `QPropertyAnimation` 可以对该控件直接做动效插值。<br>"
                      "• <b>sizeHint / minimumSizeHint:</b> 声明控件的默认自然尺寸，保证控件在布局管理器中自动合理排布。<br>"
                      "• <b>paintEvent:</b> 基于 QPainter 的纯矢量自绘渲染。";
        t.usageTiming = "三色质检工位塔灯（红黄绿状态灯）、相机曝光/增益量测精密微调旋钮、工业温湿度仪表盘。";
        t.bestPractices = "自绘控件必须适配高 DPI 屏幕缩放。绘制圆弧或图标时切勿硬编码像素数值，应通过 `qMin(width(), height())` 动态求取比例系数。";
        t.codeSnippet = 
            "// 工业级发光呼吸状态指示灯 (LED Indicator) 封装范式：\n"
            "#include <QWidget>\n"
            "#include <QPainter>\n"
            "#include <QRadialGradient>\n\n"
            "class IndustrialStatusLed : public QWidget {\n"
            "    Q_OBJECT\n"
            "    Q_PROPERTY(QColor color READ color WRITE setColor)\n"
            "public:\n"
            "    explicit IndustrialStatusLed(QWidget *parent = nullptr) : QWidget(parent), m_color(\"#10b981\") {\n"
            "        setFixedSize(28, 28);\n"
            "    }\n"
            "    QColor color() const { return m_color; }\n"
            "    void setColor(const QColor &c) { m_color = c; update(); }\n\n"
            "protected:\n"
            "    void paintEvent(QPaintEvent *) override {\n"
            "        QPainter p(this);\n"
            "        p.setRenderHint(QPainter::Antialiasing);\n\n"
            "        int side = qMin(width(), height());\n"
            "        QPointF center(width() / 2.0, height() / 2.0);\n"
            "        qreal radius = side / 2.0 - 2.0;\n\n"
            "        // 绘制带径向渐变的光晕质感\n"
            "        QRadialGradient grad(center, radius, center - QPointF(radius * 0.3, radius * 0.3));\n"
            "        grad.setColorAt(0.0, QColor(255, 255, 255, 220));\n"
            "        grad.setColorAt(0.4, m_color);\n"
            "        grad.setColorAt(1.0, m_color.darker(150));\n\n"
            "        p.setPen(QPen(m_color.darker(200), 1.5));\n"
            "        p.setBrush(grad);\n"
            "        p.drawEllipse(center, radius, radius);\n"
            "    }\n"
            "private:\n"
            "    QColor m_color;\n"
            "};";
        registerTopic(t);
    }

    // ========================================================
    // Qt 06. 工业上位机架构与高级机制 (动态热插拔插件架构)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_plugin_architecture";
        t.framework = "Qt";
        t.category = "Qt 06. 工业上位机架构与高级机制";
        t.name = "工业算子动态热插拔插件架构 (QPluginLoader / Q_DECLARE_INTERFACE)";
        t.tag = "零重启装载 .dll/.so 算法模块、接口契约解耦、动态算子市场";
        t.isVisualInteractive = false;
        t.apiSignature = "Q_DECLARE_INTERFACE(IVisionAlgorithm, \"com.visioncraft.algorithm/1.0\")\nQPluginLoader loader(pluginPath);\nauto *algo = qobject_cast<IVisionAlgorithm*>(loader.instance());";
        t.docSummary = "现代大型工业机器视觉软件（如 Cognex VisionPro、Halcon）绝不会将所有算法写死在主程序中，而是通过插件化架构实现第三方或自研算法的即插即用与独立发版。Qt 基于元对象系统提供了强大的 <code>QPluginLoader</code> 机制，通过抽象纯虚接口契约与 <code>Q_DECLARE_INTERFACE</code> 宏，实现主程序与算法动态库 (.dll/.so) 的完全二进制解耦，且支持在程序运行期间热扫描、热装载与热卸载。";
        t.docParams = "• <b>Q_DECLARE_INTERFACE:</b> 声明纯虚接口的全局唯一 URI 标识符与版本号契约。<br>"
                      "• <b>Q_PLUGIN_METADATA:</b> 插件实现类声明元数据 JSON 文件，提供插件作者、算法类别、算子版本等描述。<br>"
                      "• <b>qobject_cast:</b> 安全下转型。若插件 ABI 不兼容或未实现该接口，安全返回 nullptr，杜绝进程崩溃。";
        t.usageTiming = "算法模块独立编译与商业交付、第三方定制检测算法热更新、多机种多工序灵活切换插件库。";
        t.bestPractices = "① 插件与主程序必须使用相同大版本的 Qt 库和 MSVC 运行时库（Debug/Release 严禁混用）；<br>"
                          "② 接口类所有公开方法必须是纯虚函数，且绝不能传递包含具体 CRT 内存分配器的裸指针，推荐传递结构体引用或智能指针。";
        t.codeSnippet =
            "// 工业视觉插件架构标准工程范式：\n"
            "// 1. 公共接口契约头文件 (IVisionAlgorithm.h)\n"
            "#pragma once\n"
            "#include <QtPlugin>\n"
            "#include <QString>\n"
            "#include <opencv2/core.hpp>\n\n"
            "class IVisionAlgorithm {\n"
            "public:\n"
            "    virtual ~IVisionAlgorithm() = default;\n"
            "    virtual QString pluginName() const = 0;\n"
            "    virtual QString pluginVersion() const = 0;\n"
            "    virtual bool execute(const cv::Mat &input, cv::Mat &output, QString &errorMsg) = 0;\n"
            "};\n"
            "Q_DECLARE_INTERFACE(IVisionAlgorithm, \"com.visioncraft.algorithm/1.0\")\n\n"
            "// 2. 独立算法插件实现 (MyCustomCaliperPlugin.cpp -> 生成 .dll)\n"
            "#include \"IVisionAlgorithm.h\"\n"
            "#include <QObject>\n\n"
            "class MyCustomCaliperPlugin : public QObject, public IVisionAlgorithm {\n"
            "    Q_OBJECT\n"
            "    Q_PLUGIN_METADATA(IID \"com.visioncraft.algorithm/1.0\")\n"
            "    Q_INTERFACES(IVisionAlgorithm)\n"
            "public:\n"
            "    QString pluginName() const override { return \"HighPrecisionCaliper\"; }\n"
            "    QString pluginVersion() const override { return \"1.2.0\"; }\n"
            "    bool execute(const cv::Mat &input, cv::Mat &output, QString &) override {\n"
            "        output = input.clone();\n"
            "        return true;\n"
            "    }\n"
            "};\n\n"
            "// 3. 上位机主框架动态热装载逻辑 (PluginHost.cpp)\n"
            "#include <QPluginLoader>\n"
            "#include <QDir>\n"
            "#include <QDebug>\n\n"
            "void loadAllVisionPlugins(const QString &pluginsDir) {\n"
            "    QDir dir(pluginsDir);\n"
            "    for (const QString &fileName : dir.entryList(QDir::Files)) {\n"
            "        if (!QLibrary::isLibrary(fileName)) continue;\n"
            "        QPluginLoader loader(dir.absoluteFilePath(fileName));\n"
            "        QObject *instance = loader.instance();\n"
            "        if (instance) {\n"
            "            auto *algo = qobject_cast<IVisionAlgorithm*>(instance);\n"
            "            if (algo) {\n"
            "                qInfo() << \"成功载入算子插件:\" << algo->pluginName() << \"版本:\" << algo->pluginVersion();\n"
            "            }\n"
            "        } else {\n"
            "            qWarning() << \"插件加载失败:\" << loader.errorString();\n"
            "        }\n"
            "    }\n"
            "}";
        registerTopic(t);
    }

    // ========================================================
    // Qt 02. 多线程与并发 (Concurrent) (图像帧无锁环形队列)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_lockfree_ringbuffer";
        t.framework = "Qt";
        t.category = "Qt 02. 多线程与并发 (Concurrent)";
        t.name = "图像帧无锁环形队列 (Lock-free SPSC Ring Buffer / std::atomic)";
        t.tag = "工业高速相机 500FPS 采图、零互斥锁无竞争争用、纳秒级跨线程传递";
        t.isVisualInteractive = false;
        t.apiSignature = "template<typename T, size_t Capacity>\nclass SPSCLockFreeRingBuffer {\n    alignas(64) std::atomic<size_t> m_head{0};\n    alignas(64) std::atomic<size_t> m_tail{0};\n    std::array<T, Capacity> m_ring;\n};";
        t.docSummary = "在千兆网 (GigE) 或 CoaXPress 高速工业相机以数百 FPS 连续采图的高吞吐场景下，传统的 `QMutex + QWaitCondition` 会引发严重的线程上下文切换损耗与 CPU 锁竞争。单生产者单消费者 (SPSC) 无锁环形缓冲通过现代 C++ `std::atomic` 配合松弛/获取/释放内存序 (`memory_order_acquire / release`)，消除所有内核锁开销，实现微秒/纳秒级的图像指针交接，彻底消除采图丢帧风险。";
        t.docParams = "• <b>alignas(64):</b> 核心性能硬件法则！将 head 与 tail 指针强制对齐到不同 CPU 缓存行 (Cache Line)，彻底根除伪共享 (False Sharing) 导致的缓存行抖动。<br>"
                      "• <b>memory_order_acquire / release:</b> 建立跨核同步屏障，保证图像数据在被消费者读取前已经完全写入内存。<br>"
                      "• <b>Capacity 必须为 2 的幂次方:</b> 使得求模运算 (index % Capacity) 转换为高效位与运算 (index & (Capacity - 1))。";
        t.usageTiming = "工业相机采集回调线程将图像帧迅速投递给后台视觉算法推断线程，保证采图线程 0 阻塞。";
        t.bestPractices = "仅适用于 Single-Producer Single-Consumer 场景；若需多工作线程消费，需使用 MPMC 队列或派发中心。";
        t.codeSnippet =
            "// 工业级 SPSC 零锁高性能环形图像队列实现：\n"
            "#include <atomic>\n"
            "#include <array>\n"
            "#include <optional>\n"
            "#include <opencv2/core.hpp>\n\n"
            "template <typename T, size_t Capacity>\n"
            "class SPSCLockFreeQueue {\n"
            "    static_assert((Capacity & (Capacity - 1)) == 0, \"Capacity 必须为 2 的整数次幂\");\n"
            "public:\n"
            "    SPSCLockFreeQueue() = default;\n\n"
            "    // 采图线程调用：尝试无锁入队 (Producer)\n"
            "    bool tryPush(T item) {\n"
            "        const size_t tail = m_tail.load(std::memory_order_relaxed);\n"
            "        const size_t head = m_head.load(std::memory_order_acquire);\n"
            "        if ((tail - head) >= Capacity) {\n"
            "            return false; // 队列已满，丢帧防内存暴涨\n"
            "        }\n"
            "        m_ring[tail & BufferMask] = std::move(item);\n"
            "        m_tail.store(tail + 1, std::memory_order_release);\n"
            "        return true;\n"
            "    }\n\n"
            "    // 算法处理线程调用：尝试无锁出队 (Consumer)\n"
            "    std::optional<T> tryPop() {\n"
            "        const size_t head = m_head.load(std::memory_order_relaxed);\n"
            "        const size_t tail = m_tail.load(std::memory_order_acquire);\n"
            "        if (head == tail) {\n"
            "            return std::nullopt; // 队列为空\n"
            "        }\n"
            "        T item = std::move(m_ring[head & BufferMask]);\n"
            "        m_head.store(head + 1, std::memory_order_release);\n"
            "        return item;\n"
            "    }\n\n"
            "    size_t size() const {\n"
            "        return m_tail.load(std::memory_order_relaxed) - m_head.load(std::memory_order_relaxed);\n"
            "    }\n\n"
            "private:\n"
            "    static constexpr size_t BufferMask = Capacity - 1;\n"
            "    std::array<T, Capacity> m_ring;\n\n"
            "    // 强制按 64 字节缓存行独立对齐，彻底隔离 CPU Cache Line 伪共享！\n"
            "    alignas(64) std::atomic<size_t> m_head{0};\n"
            "    alignas(64) std::atomic<size_t> m_tail{0};\n"
            "};\n\n"
            "// 典型应用：SPSCLockFreeQueue<cv::Mat, 64> g_cameraFrameQueue;";
        registerTopic(t);
    }

    // ========================================================
    // Qt 06. 工业上位机架构与高级机制 (机台有限状态机引擎)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_state_machine";
        t.framework = "Qt";
        t.category = "Qt 06. 工业上位机架构与高级机制";
        t.name = "工业机台有限状态机引擎 (QStateMachine / QState / QSignalTransition)";
        t.tag = "自动化机台生命周期管理 (复位/就绪/运行/报警/急停)、信号驱动解耦";
        t.isVisualInteractive = false;
        t.apiSignature = "auto *machine = new QStateMachine(this);\nauto *idle = new QState(machine);\nidle->addTransition(startBtn, &QPushButton::clicked, runningState);\nmachine->start();";
        t.docSummary = "工业控制最忌讳使用布尔变量群 (`bool isRunning, isHoming, isAlarm...`) 与数百行嵌套 `if-else` 来控制机台运转，极易引发非法状态越界与机械碰撞事故。Qt 官方提供了符合 SCXML 国际标准的 <code>QStateMachine</code> 状态机框架，将机台的「空闲、回原点、高速质检、物料报警、紧急停止」封装为强隔离的状态节点，跳转完全由 Qt 信号与条件约束驱动，状态转移严密可靠。";
        t.docParams = "• <b>QState::assignProperty:</b> 状态激活时自动原子修改 UI 属性（例如进入 Running 状态自动禁用配置按钮、点亮绿灯）。<br>"
                      "• <b>QSignalTransition:</b> 监听外部信号（如传感器触发、按钮点击、PLC心跳超时）并驱动状态跃迁。<br>"
                      "• <b>全局急停跃迁:</b> 在顶层状态上绑定急停信号，无论当前处于哪一子状态，均可瞬间无条件熔断切入 Emergency 状态。";
        t.usageTiming = "自动化视觉点胶机、半导体分选机、锂电池极片质检机的主控调度循环。";
        t.bestPractices = "善用复合状态（Hierarchical States）与并行状态（Parallel States），保持单个状态类的专注度。";
        t.codeSnippet =
            "// 工业检测机台有限状态机 (FSM) 生产级实现：\n"
            "#include <QStateMachine>\n"
            "#include <QState>\n"
            "#include <QPushButton>\n"
            "#include <QLabel>\n"
            "#include <QDebug>\n\n"
            "void setupMachineFSM(QWidget *parent, QPushButton *startBtn, QPushButton *stopBtn, QPushButton *eStopBtn, QLabel *statusLabel) {\n"
            "    auto *machine = new QStateMachine(parent);\n\n"
            "    // 1. 定义机台核心状态\n"
            "    auto *idleState = new QState(machine);\n"
            "    auto *runningState = new QState(machine);\n"
            "    auto *alarmState = new QState(machine);\n"
            "    auto *emergencyState = new QState(machine);\n\n"
            "    // 2. 状态进入时自动联动修改 UI 属性\n"
            "    idleState->assignProperty(statusLabel, \"text\", \"机台就绪 (IDLE)\");\n"
            "    idleState->assignProperty(startBtn, \"enabled\", true);\n"
            "    idleState->assignProperty(stopBtn, \"enabled\", false);\n\n"
            "    runningState->assignProperty(statusLabel, \"text\", \"高速质检中 (RUNNING)\");\n"
            "    runningState->assignProperty(startBtn, \"enabled\", false);\n"
            "    runningState->assignProperty(stopBtn, \"enabled\", true);\n\n"
            "    emergencyState->assignProperty(statusLabel, \"text\", \"紧急停机 (E-STOP)\");\n"
            "    emergencyState->assignProperty(startBtn, \"enabled\", false);\n"
            "    emergencyState->assignProperty(stopBtn, \"enabled\", false);\n\n"
            "    // 3. 配置信号驱动跃迁\n"
            "    idleState->addTransition(startBtn, &QPushButton::clicked, runningState);\n"
            "    runningState->addTransition(stopBtn, &QPushButton::clicked, idleState);\n\n"
            "    // 4. 全局最高优先级急停跳转：任何状态点击 E-Stop 瞬间切入 Emergency 状态\n"
            "    idleState->addTransition(eStopBtn, &QPushButton::clicked, emergencyState);\n"
            "    runningState->addTransition(eStopBtn, &QPushButton::clicked, emergencyState);\n"
            "    alarmState->addTransition(eStopBtn, &QPushButton::clicked, emergencyState);\n\n"
            "    // 5. 监听状态进出日志\n"
            "    QObject::connect(runningState, &QState::entered, []() {\n"
            "        qInfo() << \"机台进入质检状态，开启相机高频采集...\";\n"
            "    });\n\n"
            "    machine->setInitialState(idleState);\n"
            "    machine->start();\n"
            "}";
        registerTopic(t);
    }

    // ========================================================
    // Qt 06. 工业上位机架构与高级机制 (Windows 崩溃拦截与转储)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_crash_dump";
        t.framework = "Qt";
        t.category = "Qt 06. 工业上位机架构与高级机制";
        t.name = "Windows 崩溃拦截与全自动 MiniDump 转储 (SetUnhandledExceptionFilter)";
        t.tag = "7x24 工业现场黑匣子、非法内存访问 0xC0000005 抓取、WinDbg 毫秒定位";
        t.isVisualInteractive = false;
        t.apiSignature = "LONG WINAPI GlobalCrashHandler(EXCEPTION_POINTERS *pException);\nSetUnhandledExceptionFilter(GlobalCrashHandler);";
        t.docSummary = "在 7×24 小时无人值守的自动化工厂车间，野指针、空指针解引用或第三方相机底层 SDK 内存越界可能导致上位机瞬间闪退，造成整条流水线停摆。通过注册 Windows 结构化异常处理 (SEH) 拦截未捕获异常，并在进程崩溃濒死时刻调用 `DbgHelp.dll` 的 `MiniDumpWriteDump` 写入 `.dmp` 转储文件，配合符号文件 (.pdb) 可在 WinDbg 或 Visual Studio 中精准还原崩溃发生时的调用堆栈与变量上下文。";
        t.docParams = "• <b>EXCEPTION_POINTERS:</b> 包含发生崩溃时的 CPU 寄存器上下文 (ContextRecord) 与异常记录 (ExceptionRecord)。<br>"
                      "• <b>MiniDumpWithFullMemory / MiniDumpNormal:</b> 转储级别。Normal 转储仅约数十 MB，包含全部线程堆栈与加载模块，适合网络回传。<br>"
                      "• <b>安全原则:</b> 崩溃回调函数中严禁调用 malloc、new 或 Qt GUI 弹窗，因为进程堆空间可能已被破坏。";
        t.usageTiming = "所有发布到工业生产现场的 Qt/C++ 上位机软件标配基石架构。";
        t.bestPractices = "随每个正式发布的版本归档当次编译生成的 `.pdb` 符号文件，否则 dump 文件无法还原行号和局部变量。";
        t.codeSnippet =
            "// Windows 工业级崩溃捕获与 MiniDump 全自动落盘：\n"
            "#ifdef _WIN32\n"
            "#include <windows.h>\n"
            "#include <DbgHelp.h>\n"
            "#pragma comment(lib, \"dbghelp.lib\")\n\n"
            "LONG WINAPI VisionCraftCrashFilter(EXCEPTION_POINTERS *pExceptionInfo) {\n"
            "    SYSTEMTIME st;\n"
            "    GetLocalTime(&st);\n"
            "    wchar_t dumpPath[MAX_PATH];\n"
            "    wsprintfW(dumpPath, L\"CrashDump_%04d%02d%02d_%02d%02d%02d.dmp\", \n"
            "              st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);\n\n"
            "    HANDLE hFile = CreateFileW(dumpPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);\n"
            "    if (hFile != INVALID_HANDLE_VALUE) {\n"
            "        MINIDUMP_EXCEPTION_INFORMATION mei;\n"
            "        mei.ThreadId = GetCurrentThreadId();\n"
            "        mei.ExceptionPointers = pExceptionInfo;\n"
            "        mei.ClientPointers = TRUE;\n\n"
            "        // 写入 MiniDump (包含线程调用堆栈、加载的模块与句柄信息)\n"
            "        MiniDumpWriteDump(\n"
            "            GetCurrentProcess(),\n"
            "            GetCurrentProcessId(),\n"
            "            hFile,\n"
            "            MiniDumpNormal,\n"
            "            &mei,\n"
            "            NULL,\n"
            "            NULL\n"
            "        );\n"
            "        CloseHandle(hFile);\n"
            "    }\n"
            "    return EXCEPTION_EXECUTE_HANDLER;\n"
            "}\n\n"
            "void installCrashDumper() {\n"
            "    SetUnhandledExceptionFilter(VisionCraftCrashFilter);\n"
            "}\n"
            "#endif";
        registerTopic(t);
    }

    // ========================================================
    // Qt 01. 核心机制与底层哲学 (多语言国际化热更体系)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_i18n_translator";
        t.framework = "Qt";
        t.category = "Qt 01. 核心机制与底层哲学";
        t.name = "多语言国际化免重启热更体系 (QTranslator / tr() / 动态翻译加载)";
        t.tag = "工业设备全球化交付、中/英/德/日/韩免重启无缝热更、changeEvent 监听";
        t.isVisualInteractive = false;
        t.apiSignature = "bool QTranslator::load(const QString &filename);\nqApp->installTranslator(&m_translator);\nvoid changeEvent(QEvent *event) override;";
        t.docSummary = "面向全球交付的工业视觉检测设备，必须支持现场随时切换语言环境（如中文操作工与外籍驻厂工程师交接），且严禁要求产线停机重启软件。Qt 拥有世界级的国际化架构：通过在每个窗口重写 `changeEvent(QEvent *event)` 并捕获 `QEvent::LanguageChange`，配合全局 `QTranslator` 的安装与卸载，可实现主窗口及所有嵌套子组件在毫秒内静默无缝刷新全部文本标签。";
        t.docParams = "• <b>tr(\"Text\"):</b> 标记待翻译文本。注意参数必须是字面常量字符串，严禁在 tr 内做字符串拼接。<br>"
                      "• <b>QEvent::LanguageChange:</b> 语言包变更事件。接收到此事件后调用内部 retranslateUi() 更新所有 label、button 文本。<br>"
                      "• <b>lupdate / lrelease:</b> Qt 自带命令行工具，一键提取源码中所有 tr() 文本生成 .ts 并编译为二进制 .qm 文件。";
        t.usageTiming = "任何出口欧洲、北美、东南亚的高端工业装备人机界面 (HMI)。";
        t.bestPractices = "动态文本应使用带占位符的 `tr(\"Found %1 defects in %2 ms\").arg(count).arg(time)`，绝不可将前后词汇拆开翻译，否则无法适应德语、日语的不同语序。";
        t.codeSnippet =
            "// 工业上位机无重启多语言热更标准工程实现：\n"
            "#include <QApplication>\n"
            "#include <QTranslator>\n"
            "#include <QWidget>\n"
            "#include <QLabel>\n"
            "#include <QPushButton>\n"
            "#include <QEvent>\n\n"
            "class LanguageManager : public QObject {\n"
            "    Q_OBJECT\n"
            "public:\n"
            "    static LanguageManager& instance() {\n"
            "        static LanguageManager s;\n"
            "        return s;\n"
            "    }\n"
            "    void switchLanguage(const QString &qmPath) {\n"
            "        qApp->removeTranslator(&m_translator);\n"
            "        if (m_translator.load(qmPath)) {\n"
            "            qApp->installTranslator(&m_translator);\n"
            "            // installTranslator 触发全局向所有 QWidget 发送 QEvent::LanguageChange\n"
            "        }\n"
            "    }\n"
            "private:\n"
            "    QTranslator m_translator;\n"
            "};\n\n"
            "class InspectionPanel : public QWidget {\n"
            "    Q_OBJECT\n"
            "public:\n"
            "    explicit InspectionPanel(QWidget *parent = nullptr) : QWidget(parent) {\n"
            "        m_title = new QLabel(this);\n"
            "        m_startBtn = new QPushButton(this);\n"
            "        retranslateUi();\n"
            "    }\n"
            "protected:\n"
            "    void changeEvent(QEvent *event) override {\n"
            "        if (event->type() == QEvent::LanguageChange) {\n"
            "            retranslateUi(); // 捕获语言变更事件，刷新 UI 文本\n"
            "        }\n"
            "        QWidget::changeEvent(event);\n"
            "    }\n"
            "private:\n"
            "    void retranslateUi() {\n"
            "        m_title->setText(tr(\"工业机器视觉精密质检站\"));\n"
            "        m_startBtn->setText(tr(\"启动全自动检测\"));\n"
            "    }\n"
            "    QLabel *m_title;\n"
            "    QPushButton *m_startBtn;\n"
            "};";
        registerTopic(t);
    }

    // ========================================================
    // Qt 02. 多线程与并发 (Concurrent) (异步双缓冲日志系统)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_async_logger";
        t.framework = "Qt";
        t.category = "Qt 02. 多线程与并发 (Concurrent)";
        t.name = "工业级异步双缓冲日志系统 (qInstallMessageHandler / 双缓冲分卷落盘)";
        t.tag = "高频质检日志零卡顿、qInstallMessageHandler 重定向、双缓冲无锁交换落盘";
        t.isVisualInteractive = false;
        t.apiSignature = "void qInstallMessageHandler(QtMessageHandler handler);\nclass AsyncLogEngine : public QThread { ... };";
        t.docSummary = "在每秒检测 30 个工件的视觉产线上，每次检测都会输出尺寸公差、OK/NG 判定、算子耗时等日志。若在 UI 线程直接调用同步文件 I/O，遇到机械硬盘或工业固态写入抖动时，主界面会直接冻结卡顿。基于双缓冲队列 (Double Buffering) 的异步日志引擎：业务线程仅将日志对象推入前台内存缓冲区（微秒级），后台落盘线程定期原子交换前后缓冲区并批量写入磁盘，提供超高并发与零 GUI 阻塞体验。";
        t.docParams = "• <b>qInstallMessageHandler:</b> Qt 全局消息重定向钩子，将所有 qDebug(), qInfo(), qWarning() 统一转入异步管道。<br>"
                      "• <b>双缓冲区 (Active/Back Buffer):</b> 写入与落盘分离。交换指针仅需极短暂的一次互斥锁保护，随后磁盘写入全程无需加锁。<br>"
                      "• <b>自动分卷与清理:</b> 按天或者按单个文件大小 (如 50MB) 自动切分新日志，并自动清理 30 天以前的过期历史日志。";
        t.usageTiming = "高频流水线连续质检、多相机并发运行、需留存审计日志的医疗/汽车工业场景。";
        t.bestPractices = "程序退出或异常崩溃时，务必调用 flush() 强制将前台内存中尚未落盘的剩余日志写入文件。";
        t.codeSnippet =
            "// 工业级异步双缓冲日志落盘引擎实现：\n"
            "#include <QThread>\n"
            "#include <QMutex>\n"
            "#include <QWaitCondition>\n"
            "#include <QFile>\n"
            "#include <QTextStream>\n"
            "#include <QDateTime>\n"
            "#include <vector>\n\n"
            "struct LogItem {\n"
            "    QString time;\n"
            "    QtMsgType type;\n"
            "    QString message;\n"
            "};\n\n"
            "class AsyncLogWorker : public QThread {\n"
            "    Q_OBJECT\n"
            "public:\n"
            "    void append(const LogItem &item) {\n"
            "        QMutexLocker locker(&m_mutex);\n"
            "        m_activeBuffer.push_back(item);\n"
            "        m_cond.wakeOne();\n"
            "    }\n"
            "    void stop() {\n"
            "        m_running = false;\n"
            "        m_cond.wakeAll();\n"
            "        wait();\n"
            "    }\n"
            "protected:\n"
            "    void run() override {\n"
            "        std::vector<LogItem> writeBuffer;\n"
            "        QFile file(\"visioncraft_system.log\");\n"
            "        file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);\n"
            "        QTextStream out(&file);\n\n"
            "        while (m_running) {\n"
            "            {\n"
            "                QMutexLocker locker(&m_mutex);\n"
            "                if (m_activeBuffer.empty()) {\n"
            "                    m_cond.wait(&m_mutex, 100);\n"
            "                }\n"
            "                // 微秒级指针交换 (Double Buffer Swap)\n"
            "                writeBuffer.swap(m_activeBuffer);\n"
            "            }\n"
            "            if (!writeBuffer.empty()) {\n"
            "                for (const auto &it : writeBuffer) {\n"
            "                    out << \"[\" << it.time << \"] \" << it.message << \"\\n\";\n"
            "                }\n"
            "                out.flush();\n"
            "                writeBuffer.clear();\n"
            "            }\n"
            "        }\n"
            "    }\n"
            "private:\n"
            "    QMutex m_mutex;\n"
            "    QWaitCondition m_cond;\n"
            "    std::vector<LogItem> m_activeBuffer;\n"
            "    bool m_running = true;\n"
            "};";
        registerTopic(t);
    }
}
