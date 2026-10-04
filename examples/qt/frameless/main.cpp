// 无边框窗口：自己画标题栏、自己处理拖动和边缘缩放
//
// 运行：./example_qt_frameless [截图目录]
//   在桌面上运行（本机是 xcb，经 XWayland）会短暂弹出窗口；QT_QPA_PLATFORM=offscreen 则不弹窗
// 输出见 output.txt（两种平台各跑一次）

#include <QApplication>
#include <QElapsedTimer>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>
#include <cstdio>

// 等窗口管理器处理完（真实桌面上，移动、改大小都是异步的）
static void settle(int ms = 200)
{
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < ms)
        QApplication::processEvents(QEventLoop::AllEvents, 10);
}

// [region title-bar]
// 自己画的标题栏：按住拖动整个窗口
class TitleBar : public QWidget {
public:
    explicit TitleBar(QWidget *parent) : QWidget(parent)
    {
        setFixedHeight(36);
        setAttribute(Qt::WA_StyledBackground);               // 见「QSS 样式表与换肤」
        setStyleSheet("background: #1e293b; color: #e2e8f0;");
        auto *row = new QHBoxLayout(this);
        row->setContentsMargins(12, 0, 4, 0);
        row->addWidget(new QLabel("VisionCraft 工位"));
        row->addStretch();
        auto *close = new QPushButton("✕");
        close->setFixedSize(32, 28);
        connect(close, &QPushButton::clicked, window(), &QWidget::close);
        row->addWidget(close);
    }
protected:
    void mousePressEvent(QMouseEvent *e) override
    {
        if (e->button() == Qt::LeftButton) {
            m_pressOffset = e->globalPosition().toPoint() - window()->pos();   // 鼠标相对窗口左上角的位置
            m_dragging = true;
        }
    }
    void mouseMoveEvent(QMouseEvent *e) override
    {
        if (m_dragging)
            window()->move(e->globalPosition().toPoint() - m_pressOffset);     // 保持这个相对位置不变
    }
    void mouseReleaseEvent(QMouseEvent *) override { m_dragging = false; }
    void mouseDoubleClickEvent(QMouseEvent *) override
    {
        window()->isMaximized() ? window()->showNormal() : window()->showMaximized();
    }
private:
    QPoint m_pressOffset;
    bool m_dragging = false;
};
// [endregion]

// [region edges]
// 鼠标在窗口边缘 kBorder 像素以内时，返回它靠近哪几条边
static constexpr int kBorder = 6;
static Qt::Edges edgesAt(const QRect &r, const QPoint &p)
{
    Qt::Edges e;
    if (p.x() < kBorder) e |= Qt::LeftEdge;
    if (p.x() >= r.width() - kBorder) e |= Qt::RightEdge;
    if (p.y() < kBorder) e |= Qt::TopEdge;
    if (p.y() >= r.height() - kBorder) e |= Qt::BottomEdge;
    return e;
}
// [endregion]

// [region resizable]
// 无边框窗口的边缘：悬停时换光标，按下时把缩放交给窗口管理器
class ResizableFrame : public QWidget {
public:
    ResizableFrame() : QWidget(nullptr, Qt::FramelessWindowHint | Qt::Window) {}
    bool lastSystemResize = false;
protected:
    void mouseMoveEvent(QMouseEvent *e) override
    {
        const Qt::Edges ed = edgesAt(rect(), e->position().toPoint());
        if (ed == (Qt::LeftEdge | Qt::TopEdge) || ed == (Qt::RightEdge | Qt::BottomEdge)) setCursor(Qt::SizeFDiagCursor);
        else if (ed == (Qt::RightEdge | Qt::TopEdge) || ed == (Qt::LeftEdge | Qt::BottomEdge)) setCursor(Qt::SizeBDiagCursor);
        else if (ed & (Qt::LeftEdge | Qt::RightEdge)) setCursor(Qt::SizeHorCursor);
        else if (ed & (Qt::TopEdge | Qt::BottomEdge)) setCursor(Qt::SizeVerCursor);
        else unsetCursor();
    }
    void mousePressEvent(QMouseEvent *e) override
    {
        const Qt::Edges ed = edgesAt(rect(), e->position().toPoint());
        if (e->button() == Qt::LeftButton && ed)
            lastSystemResize = windowHandle()->startSystemResize(ed);   // 之后的拖动由窗口管理器负责
    }
};
// [endregion]

static const char *edgeName(Qt::Edges e)
{
    if (e == (Qt::LeftEdge | Qt::TopEdge)) return "左上角";
    if (e == (Qt::RightEdge | Qt::BottomEdge)) return "右下角";
    if (e == Qt::LeftEdge) return "左边";
    if (e == Qt::RightEdge) return "右边";
    if (e == Qt::TopEdge) return "上边";
    if (e == Qt::BottomEdge) return "下边";
    if (!e) return "（不在边缘）";
    return "其他";
}

// [region rounded]
// 圆角窗口：窗口本身透明，自己画一个圆角矩形当背景
class RoundedWindow : public QWidget {
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(QColor("#1e293b"));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect(), 12, 12);
    }
};
// [endregion]

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    const QString dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");
    std::printf("平台：%s\n", qPrintable(QGuiApplication::platformName()));

    std::printf("\n==== 1. 有边框和无边框 ====\n");
    for (bool frameless : {false, true}) {
        QWidget w;
        // [region flag]
        if (frameless)
            w.setWindowFlags(Qt::FramelessWindowHint | Qt::Window);   // 必须在 show() 之前设置
        // [endregion]
        w.resize(400, 300);
        w.move(200, 200);
        w.show();
        settle();
        const QRect g = w.geometry(), f = w.frameGeometry();
        std::printf("  %s：geometry %dx%d @(%d,%d)，frameGeometry %dx%d @(%d,%d)\n", frameless ? "无边框" : "有边框",
                    g.width(), g.height(), g.x(), g.y(), f.width(), f.height(), f.x(), f.y());
    }
    {
        QWidget w;
        w.resize(400, 300);
        w.show();
        settle();
        // [region flag-after-show]
        w.setWindowFlags(Qt::FramelessWindowHint | Qt::Window);   // 窗口已经显示了才改
        // [endregion]
        std::printf("  show() 之后再 setWindowFlags：isVisible=%d\n", w.isVisible());
    }

    std::printf("\n==== 2. 拖动自绘标题栏 ====\n");
    {
        QWidget w(nullptr, Qt::FramelessWindowHint | Qt::Window);
        auto *col = new QVBoxLayout(&w);
        col->setContentsMargins(0, 0, 0, 0);
        auto *bar = new TitleBar(&w);
        col->addWidget(bar);
        col->addWidget(new QLabel("  内容区"), 1);
        w.resize(400, 300);
        w.move(200, 200);
        w.show();
        settle();
        const QPoint before = w.pos();
        // [region simulate]
        // 模拟：在标题栏 (50,10) 处按下，向右下拖 120×80，松开
        const QPoint local(50, 10), global = bar->mapToGlobal(local);
        QMouseEvent press(QEvent::MouseButtonPress, local, global, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(bar, &press);
        for (int i = 1; i <= 4; ++i) {
            const QPoint g = global + QPoint(30 * i, 20 * i);
            QMouseEvent move(QEvent::MouseMove, bar->mapFromGlobal(g), g, Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(bar, &move);
        }
        // [endregion]
        settle();
        std::printf("  拖之前 (%d,%d)，拖之后 (%d,%d)，移动了 (%d,%d)\n", before.x(), before.y(), w.pos().x(), w.pos().y(),
                    w.pos().x() - before.x(), w.pos().y() - before.y());
        if (dir != "-")
            w.grab().save(dir + "/frameless-titlebar.png");
        QMouseEvent dbl(QEvent::MouseButtonDblClick, local, bar->mapToGlobal(local), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(bar, &dbl);
        settle(500);
        std::printf("  双击标题栏：isMaximized=%d，大小 %dx%d\n", w.isMaximized(), w.width(), w.height());
        QApplication::sendEvent(bar, &dbl);
        settle(500);
        std::printf("  再双击：isMaximized=%d，大小 %dx%d\n", w.isMaximized(), w.width(), w.height());
    }

    std::printf("\n==== 3. 边缘判断 ====\n");
    {
        const QRect r(0, 0, 400, 300);
        for (QPoint p : {QPoint(2, 2), QPoint(398, 298), QPoint(3, 150), QPoint(200, 297), QPoint(200, 150), QPoint(7, 150)})
            std::printf("  (%3d,%3d) → %s\n", p.x(), p.y(), edgeName(edgesAt(r, p)));
    }

    std::printf("\n==== 4. 悬停换光标 ====\n");
    for (bool tracking : {false, true}) {
        ResizableFrame w;
        // [region tracking]
        w.setMouseTracking(tracking);                       // 不打开的话，不按住鼠标键就收不到移动事件
        // [endregion]
        w.resize(400, 300);
        w.show();
        settle();
        QMouseEvent hover(QEvent::MouseMove, QPointF(398, 298), w.mapToGlobal(QPointF(398, 298)), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(&w, &hover);
        std::printf("  setMouseTracking(%s)，鼠标移到右下角：光标 %s\n", tracking ? "true" : "false",
                    w.cursor().shape() == Qt::SizeFDiagCursor ? "SizeFDiagCursor（斜向双箭头）" : "ArrowCursor（普通箭头）");
        if (tracking) {
            QMouseEvent press(QEvent::MouseButtonPress, QPointF(398, 298), w.mapToGlobal(QPointF(398, 298)), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(&w, &press);
            std::printf("  在右下角按下：startSystemResize 返回 %s\n", w.lastSystemResize ? "true" : "false");
        }
    }

    std::printf("\n==== 5. 交给窗口管理器：startSystemMove / startSystemResize ====\n");
    {
        QWidget w(nullptr, Qt::FramelessWindowHint | Qt::Window);
        w.resize(400, 300);
        w.show();
        settle();
        // [region system-move]
        const bool moveOk = w.windowHandle()->startSystemMove();                 // 正常应在鼠标按下时调用
        const bool resizeOk = w.windowHandle()->startSystemResize(Qt::RightEdge | Qt::BottomEdge);
        // [endregion]
        std::printf("  没有按下鼠标时调用：startSystemMove 返回 %s，startSystemResize 返回 %s\n",
                    moveOk ? "true" : "false", resizeOk ? "true" : "false");
    }

    std::printf("\n==== 6. 圆角：透明背景 ====\n");
    for (bool translucent : {false, true}) {
        RoundedWindow w;
        w.setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
        // [region translucent]
        if (translucent)
            w.setAttribute(Qt::WA_TranslucentBackground);       // 窗口没画到的地方是透明的
        // [endregion]
        w.resize(200, 120);
        w.show();
        settle();
        const QImage img = w.grab().toImage();
        const QColor corner = img.pixelColor(0, 0), center = img.pixelColor(100, 60);
        std::printf("  %s：左上角像素 %s alpha=%d，中心 %s\n", translucent ? "WA_TranslucentBackground" : "不设透明",
                    qPrintable(corner.name()), corner.alpha(), qPrintable(center.name()));
        if (dir != "-")
            img.save(dir + QString("/frameless-round-%1.png").arg(translucent ? "translucent" : "opaque"));
    }
    return 0;
}
