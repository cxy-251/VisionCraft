// QPainter 绘图与双缓冲：画到 QImage 上，再读像素核对
//
// 运行：QT_QPA_PLATFORM=offscreen ./example_qt_painter [截图目录]     输出见 output.txt

#include <QApplication>
#include <QBackingStore>
#include <QElapsedTimer>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QSet>
#include <QWidget>
#include <QtMath>
#include <cstdio>

static QImage canvas(int w, int h)
{
    QImage img(w, h, QImage::Format_RGB32);
    img.fill(Qt::white);
    return img;
}

// 某一行（或列）上，不是纯白的像素：位置和灰度
static void printColumn(const QImage &img, int x, int y0, int y1)
{
    for (int y = y0; y <= y1; ++y)
        std::printf(" y=%d:%d", y, qGray(img.pixel(x, y)));
    std::printf("\n");
}

// [region gauge]
// 一个仪表盘：刻度是静态的，指针随数值变化
static void drawDial(QPainter &p, const QRectF &r)               // 静态部分
{
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor("#1e293b"));
    p.setPen(QPen(QColor("#64748b"), 2));
    p.drawEllipse(r);
    p.save();                                                     // 保存当前的坐标变换、画笔
    p.translate(r.center());                                      // 原点移到表盘中心
    p.setPen(QPen(QColor("#e2e8f0"), 2));
    for (int i = 0; i <= 50; ++i) {                               // 0–100 ℃，每 2 ℃ 一格
        const double a = -225 + 270.0 * i / 50;                   // 从左下 −225° 转到右下 45°
        p.save();
        p.rotate(a);
        p.drawLine(QPointF(r.width() / 2 - (i % 5 ? 8 : 16), 0), QPointF(r.width() / 2 - 3, 0));
        p.restore();
    }
    p.restore();                                                  // 回到进来时的状态
}
static void drawNeedle(QPainter &p, const QRectF &r, double value) // 动态部分
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    p.translate(r.center());
    p.rotate(-225 + 270.0 * value / 100);
    p.setPen(QPen(QColor("#f97316"), 3, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(0, 0), QPointF(r.width() / 2 - 20, 0));
    p.restore();
}
// [endregion]

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    const QString dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");

    std::printf("==== 1. 抗锯齿 ====\n");
    for (bool aa : {false, true}) {
        QImage img = canvas(100, 100);
        QPainter p(&img);
        // [region aa]
        p.setRenderHint(QPainter::Antialiasing, aa);
        p.setPen(QPen(Qt::black, 1));
        p.drawLine(QPointF(10, 10), QPointF(90, 37));            // 一条斜线
        // [endregion]
        p.end();
        QSet<int> grays;
        int touched = 0;
        for (int y = 0; y < 100; ++y)
            for (int x = 0; x < 100; ++x)
                if (const int g = qGray(img.pixel(x, y)); g < 255) { grays.insert(g); ++touched; }
        std::printf("  Antialiasing=%-5s 改动了 %d 个像素，用到 %lld 种灰度\n", aa ? "true" : "false", touched, qlonglong(grays.size()));
        img.copy(8, 6, 30, 16).scaled(240, 128, Qt::IgnoreAspectRatio, Qt::FastTransformation)
            .save(dir + QString("/painter-aa-%1.png").arg(aa ? "on" : "off"));
    }

    std::printf("\n==== 2. 一像素宽的线画在哪 ====\n");
    for (double y : {20.0, 20.5}) {
        QImage img = canvas(40, 40);
        QPainter p(&img);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(Qt::black, 1));
        // [region half-pixel]
        p.drawLine(QPointF(5, y), QPointF(35, y));               // 水平线，y 分别是 20 和 20.5
        // [endregion]
        p.end();
        std::printf("  y=%.1f，第 20 列从上到下：", y);
        printColumn(img, 20, 18, 22);
    }

    std::printf("\n==== 3. save / restore 与坐标变换 ====\n");
    {
        QImage img = canvas(200, 200);
        QPainter p(&img);
        // [region transform]
        p.translate(100, 100);                                   // 原点移到 (100,100)
        p.rotate(90);                                            // 顺时针转 90°（y 轴向下）
        p.fillRect(QRectF(40, -2, 4, 4), Qt::red);               // 局部坐标 (40,0) 附近
        p.resetTransform();
        p.fillRect(QRectF(40, -2, 4, 4), Qt::blue);              // 没有变换：贴着顶边
        // [endregion]
        p.end();
        auto find = [&](QRgb c) {
            for (int y = 0; y < 200; ++y)
                for (int x = 0; x < 200; ++x)
                    if (img.pixel(x, y) == c) return QPoint(x, y);
            return QPoint(-1, -1);
        };
        const QPoint red = find(qRgb(255, 0, 0)), blue = find(qRgb(0, 0, 255));
        std::printf("  红色方块左上角像素 (%d,%d)，蓝色方块左上角像素 (%d,%d)\n", red.x(), red.y(), blue.x(), blue.y());
    }

    std::printf("\n==== 4. 双缓冲：静态部分缓存成 QPixmap ====\n");
    {
        const QRectF r(10, 10, 280, 280);
        QImage frame = canvas(300, 300);
        const int frames = 300;
        QElapsedTimer t;

        // [region redraw-all]
        t.start();
        for (int i = 0; i < frames; ++i) {                        // 每一帧：表盘 + 指针全部重画
            QPainter p(&frame);
            p.fillRect(frame.rect(), Qt::white);
            drawDial(p, r);
            drawNeedle(p, r, i % 100);
        }
        const double allMs = t.nsecsElapsed() / 1e6 / frames;
        // [endregion]

        // [region cached]
        QPixmap dial(300, 300);                                   // 静态部分只画一次
        dial.fill(Qt::white);
        { QPainter p(&dial); drawDial(p, r); }
        t.restart();
        for (int i = 0; i < frames; ++i) {                        // 每一帧：贴缓存 + 画指针
            QPainter p(&frame);
            p.drawPixmap(0, 0, dial);
            drawNeedle(p, r, i % 100);
        }
        const double cachedMs = t.nsecsElapsed() / 1e6 / frames;
        // [endregion]
        std::printf("  每帧全部重画：%.3f ms\n  缓存表盘：    %.3f ms\n", allMs, cachedMs);
        {
            QPainter p(&frame);
            p.drawPixmap(0, 0, dial);
            drawNeedle(p, r, 63);
        }
        frame.save(dir + "/painter-dial.png");
    }

    std::printf("\n==== 5. QWidget 自己有没有双缓冲 ====\n");
    {
        // [region widget-buffer]
        QWidget w;
        w.resize(200, 100);
        w.show();
        QApplication::processEvents();
        QBackingStore *store = w.backingStore();                  // 窗口的后台缓冲区
        std::printf("  WA_PaintOnScreen=%d，backingStore %s，缓冲区大小 %dx%d\n", w.testAttribute(Qt::WA_PaintOnScreen),
                    store ? "存在" : "不存在", store ? store->size().width() : 0, store ? store->size().height() : 0);
        // [endregion]
    }
    return 0;
}
