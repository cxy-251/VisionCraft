// 自定义控件：一个温度表 Gauge，像标准控件一样能放进布局、能用属性、能被样式表设置
//
// 运行：QT_QPA_PLATFORM=offscreen ./example_qt_custom_widget [截图目录]     输出见 output.txt

#include <QApplication>
#include <QHBoxLayout>
#include <QPainter>
#include <QPixmap>
#include <QStyleOption>
#include <QVBoxLayout>
#include <cstdio>

// [region class]
class Gauge : public QWidget {
    Q_OBJECT
    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)   // 属性：QML、动画、样式表都能用
    Q_PROPERTY(QColor needleColor MEMBER m_needleColor)
public:
    explicit Gauge(QWidget *parent = nullptr) : QWidget(parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
    double value() const { return m_value; }
    void setValue(double v)
    {
        v = qBound(0.0, v, 100.0);
        if (qFuzzyCompare(v, m_value))                        // 没变就什么也不做：不发信号、不重画
            return;
        m_value = v;
        emit valueChanged(v);
        update();                                             // 请求重画，不是立刻重画
    }
    QSize sizeHint() const override { return {160, 160}; }    // 告诉布局「我希望多大」
    QSize minimumSizeHint() const override { return {80, 80}; }

    int paints = 0, dialRedraws = 0;                          // 示例用：统计次数

signals:
    void valueChanged(double value);

protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override { m_dial = QPixmap(); }   // 大小变了，表盘缓存作废

private:
    void drawDial();
    double m_value = 0;
    QColor m_needleColor = QColor("#f97316");
    QPixmap m_dial;
};
// [endregion]

// [region paint]
void Gauge::paintEvent(QPaintEvent *)
{
    ++paints;
    QPainter p(this);
    QStyleOption opt;                                         // 让样式表里的 background、border 也能画出来
    opt.initFrom(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

    if (m_dial.isNull())
        drawDial();
    p.drawPixmap(0, 0, m_dial);

    const QRectF r = QRectF(rect()).adjusted(8, 8, -8, -8);
    const double side = qMin(r.width(), r.height());
    p.setRenderHint(QPainter::Antialiasing);
    p.translate(r.center());
    p.rotate(-225 + 270.0 * m_value / 100);
    p.setPen(QPen(m_needleColor, 3, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(0, 0), QPointF(side / 2 - 14, 0));
}

void Gauge::drawDial()
{
    ++dialRedraws;
    m_dial = QPixmap(size());
    m_dial.fill(Qt::transparent);                             // 透明：样式表画的背景能透出来
    QPainter p(&m_dial);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF r = QRectF(rect()).adjusted(8, 8, -8, -8);
    const double side = qMin(r.width(), r.height());
    const QRectF circle(r.center() - QPointF(side / 2, side / 2), QSizeF(side, side));
    p.setBrush(QColor("#1e293b"));
    p.setPen(QPen(QColor("#64748b"), 2));
    p.drawEllipse(circle);
    p.translate(r.center());
    p.setPen(QPen(QColor("#e2e8f0"), 1.5));
    for (int i = 0; i <= 50; ++i) {
        p.save();
        p.rotate(-225 + 270.0 * i / 50);
        p.drawLine(QPointF(side / 2 - (i % 5 ? 5 : 10), 0), QPointF(side / 2 - 2, 0));
        p.restore();
    }
}
// [endregion]

static void settle() { QApplication::processEvents(); }

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    const QString dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");

    QWidget w;
    auto *row = new QHBoxLayout(&w);
    auto *a = new Gauge, *b = new Gauge;
    a->setObjectName("a");
    b->setObjectName("b");
    row->addWidget(a);
    row->addWidget(b);
    w.show();
    settle();

    std::printf("==== 1. 放进布局 ====\n");
    std::printf("  窗口按 sizeHint 自动定的大小 %dx%d，每个表 %dx%d\n", w.width(), w.height(), a->width(), a->height());

    std::printf("\n==== 2. update() 与 repaint() ====\n");
    {
        int signals_ = 0;
        QObject::connect(a, &Gauge::valueChanged, [&] { ++signals_; });
        // [region update]
        a->paints = 0;
        for (int i = 1; i <= 100; ++i)
            a->setValue(i * 0.5);                             // 连续改 100 次
        const int beforeLoop = a->paints;
        settle();                                             // 回到事件循环
        // [endregion]
        std::printf("  setValue 100 次：valueChanged %d 次；回事件循环之前画了 %d 次，之后共画了 %d 次\n",
                    signals_, beforeLoop, a->paints);
        signals_ = 0;
        a->paints = 0;
        for (int i = 0; i < 100; ++i)
            a->setValue(50);                                  // 值不变
        settle();
        std::printf("  同一个值设 100 次：valueChanged %d 次，画了 %d 次\n", signals_, a->paints);
        // [region repaint]
        a->paints = 0;
        for (int i = 1; i <= 100; ++i) {
            a->setValue(i * 0.3);
            a->repaint();                                     // 立刻、同步地画
        }
        // [endregion]
        std::printf("  每次 setValue 后再 repaint()：画了 %d 次\n", a->paints);
    }

    std::printf("\n==== 3. 表盘缓存 ====\n");
    {
        a->dialRedraws = 0;
        for (int i = 0; i < 50; ++i) { a->setValue(i); settle(); }
        std::printf("  改 50 次数值：表盘重画 %d 次\n", a->dialRedraws);
        w.resize(w.width() + 100, w.height() + 60);
        settle();
        std::printf("  窗口变大一次：表盘重画 %d 次（大小 %dx%d）\n", a->dialRedraws, a->width(), a->height());
    }

    std::printf("\n==== 4. 属性系统 ====\n");
    {
        // [region property]
        b->setProperty("value", 75);                          // 按名字设置：和 setValue(75) 一样
        app.setStyleSheet("Gauge#b { qproperty-needleColor: #22c55e; background: #0f172a; border-radius: 8px; }");
        // [endregion]
        settle();
        std::printf("  value=%.0f，needleColor=%s\n", b->value(), qPrintable(b->property("needleColor").value<QColor>().name()));
        const QImage img = w.grab().toImage();                // 截整个窗口：单独 grab 子控件时，Qt 会先用调色板填底色
        auto at = [&](int x, int y) { return img.pixelColor(b->mapTo(&w, QPoint(x, y))).name(); };
        std::printf("  b 内 (6,%d) 像素 %s（样式表的 background）\n", b->height() / 2, qPrintable(at(6, b->height() / 2)));
        std::printf("  b 的左上角 (0,0) 像素 %s（圆角外面，透出窗口底色）\n", qPrintable(at(0, 0)));
    }

    a->setValue(32);
    settle();
    w.grab().save(dir + "/custom-gauge.png");
    return 0;
}

#include "main.moc"
