// 图形视图：场景里放图元，视图负责缩放、平移、查找
//
// 运行：QT_QPA_PLATFORM=offscreen ./example_qt_graphicsview [截图目录]     输出见 output.txt

#include <QApplication>
#include <QElapsedTimer>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsView>
#include <QImage>
#include <QPainter>
#include <QRandomGenerator>
#include <cstdio>

// 一张 800×600 的「工件照片」：灰底上一个圆
static QPixmap partPhoto()
{
    QImage img(800, 600, QImage::Format_RGB32);
    img.fill(QColor("#3f3f46"));
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor("#a1a1aa"));
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPointF(400, 300), 250, 250);
    p.setBrush(QColor("#3f3f46"));
    p.drawEllipse(QPointF(400, 300), 90, 90);
    return QPixmap::fromImage(img);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    const QString dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");

    // [region scene]
    QGraphicsScene scene(0, 0, 800, 600);                     // 场景坐标：和照片的像素坐标一致
    scene.addPixmap(partPhoto());                             // 底图
    auto *box = scene.addRect(QRectF(0, 0, 60, 40), QPen(QColor("#ef4444"), 2));
    box->setPos(520, 180);                                    // 缺陷框：图元自己的坐标原点放在场景 (520,180)
    auto *label = scene.addSimpleText("划痕 23.1");
    label->setBrush(QColor("#fecaca"));
    label->setParentItem(box);                                // 标签挂在框下面：跟着框一起移动
    label->setPos(0, -22);
    // [endregion]
    box->setFlag(QGraphicsItem::ItemIsSelectable);

    std::printf("==== 1. 三套坐标 ====\n");
    // [region view]
    QGraphicsView view(&scene);
    view.resize(500, 400);
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.show();
    view.fitInView(scene.sceneRect(), Qt::KeepAspectRatio);  // 整张图缩进窗口
    // [endregion]
    QApplication::processEvents();
    auto pt = [](QPointF p) { return QString("(%1,%2)").arg(p.x(), 0, 'f', 1).arg(p.y(), 0, 'f', 1); };
    std::printf("  缩放倍数 %.3f\n", view.transform().m11());
    std::printf("  框的左上角：图元坐标 (0,0) → 场景坐标 %s → 视图（窗口像素）坐标 %s\n",
                qPrintable(pt(box->mapToScene(0, 0))), qPrintable(pt(view.mapFromScene(box->mapToScene(0, 0)))));
    std::printf("  标签：图元坐标 (0,0) → 父图元坐标 %s → 场景坐标 %s\n",
                qPrintable(pt(label->pos())), qPrintable(pt(label->mapToScene(0, 0))));
    {
        // [region pick]
        const QPoint click = view.mapFromScene(QPointF(540, 200));   // 模拟：用户点在框中间
        QGraphicsItem *hit = view.itemAt(click);
        // [endregion]
        std::printf("  在窗口 %d,%d 处点击，itemAt 得到：%s\n", click.x(), click.y(),
                    hit == box ? "缺陷框" : hit == label ? "标签" : hit ? "底图" : "无");
        const QPoint edge = view.mapFromScene(QPointF(521, 181));
        QGraphicsItem *hitEdge = view.itemAt(edge);
        std::printf("  点在框的边线上 %d,%d：%s\n", edge.x(), edge.y(),
                    hitEdge == box ? "缺陷框" : hitEdge == label ? "标签" : hitEdge ? "底图" : "无");
    }
    view.grab().save(dir + "/graphicsview-fit.png");

    std::printf("\n==== 2. 放大 ====\n");
    {
        // [region zoom]
        view.resetTransform();
        view.scale(4, 4);                                     // 放大 4 倍
        view.centerOn(box);
        // [endregion]
        QApplication::processEvents();
        const QPoint tl = view.mapFromScene(box->mapToScene(0, 0));
        const QImage shot = view.viewport()->grab().toImage();
        int red = 0;                                          // 从框左边线外侧往右数，连续多少个红色像素
        for (int x = tl.x() - 10; x < tl.x() + 20; ++x) {
            const QColor c = shot.pixelColor(x, tl.y() + 40);
            if (c.red() > 200 && c.green() < 120) ++red;
        }
        std::printf("  缩放倍数 %.1f；框的左上角在窗口 %s；截图里框的左边线宽 %d 像素（画笔宽 2）\n", view.transform().m11(),
                    qPrintable(pt(tl)), red);
        view.grab().save(dir + "/graphicsview-zoom.png");
        auto onScreenHeight = [&] {                           // 标签在窗口里实际多高（像素）
            return label->deviceTransform(view.viewportTransform()).mapRect(label->boundingRect()).height();
        };
        const double before = onScreenHeight();
        // [region ignore]
        label->setFlag(QGraphicsItem::ItemIgnoresTransformations);   // 标签不跟着缩放：文字始终一样大
        // [endregion]
        QApplication::processEvents();
        view.grab().save(dir + "/graphicsview-zoom-label.png");
        std::printf("  标签自身高 %.0f；在窗口里的高度：设 ItemIgnoresTransformations 之前 %.0f 像素，之后 %.0f 像素\n",
                    label->boundingRect().height(), before, onScreenHeight());
    }

    std::printf("\n==== 3. 十万个图元里找东西 ====\n");
    for (auto method : {QGraphicsScene::BspTreeIndex, QGraphicsScene::NoIndex}) {
        // [region index]
        QGraphicsScene big(0, 0, 10000, 10000);
        big.setItemIndexMethod(method);
        QRandomGenerator rng(1);
        for (int i = 0; i < 100000; ++i)
            big.addRect(rng.bounded(9990), rng.bounded(9990), 10, 10);
        QElapsedTimer t;
        t.start();
        int found = 0;
        for (int q = 0; q < 1000; ++q)                        // 1000 次「这个小区域里有哪些图元」
            found += int(big.items(QRectF(rng.bounded(9900), rng.bounded(9900), 100, 100)).size());
        // [endregion]
        const double ms = t.nsecsElapsed() / 1e6;
        std::printf("  %-12s 1000 次区域查询：%.1f ms，平均每次找到 %.1f 个\n",
                    method == QGraphicsScene::BspTreeIndex ? "BspTreeIndex" : "NoIndex", ms, found / 1000.0);
    }
    return 0;
}
