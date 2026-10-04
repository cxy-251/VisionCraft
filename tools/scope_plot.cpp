// 把 scope_probe.tcl 采到的数据画成一张波形图（PNG），给手册用。
//   scope_plot <samples.txt> <采样率 Hz> <输出.png> [标题] [只画前多少个点]
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <cstdio>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    if (argc < 4) { std::fprintf(stderr, "用法：scope_plot samples.txt 采样率 out.png [标题] [点数]\n"); return 1; }
    QFile f(QString::fromLocal8Bit(argv[1]));
    f.open(QIODevice::ReadOnly);
    QList<int> v;
    for (const QByteArray &line : f.readAll().split('\n'))
        if (!line.trimmed().isEmpty()) v << line.trimmed().toInt();
    const double rate = QByteArray(argv[2]).toDouble();
    const QString title = argc > 4 ? QString::fromUtf8(argv[4]) : QString();
    const int n = argc > 5 ? qMin(int(v.size()), QByteArray(argv[5]).toInt()) : int(v.size());

    const int W = 900, H = 320, L = 60, R = 40, T = 34, B = 40;
    QImage img(W, H, QImage::Format_RGB32);
    img.fill(QColor("#0f172a"));
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF plot(L, T, W - L - R, H - T - B);
    auto px = [&](int i) { return plot.left() + plot.width() * i / qMax(1, n - 1); };
    auto py = [&](int code) { return plot.bottom() - plot.height() * code / 4095.0; };
    // 网格：纵轴 0、1024、2048、3072、4095；横轴 10 等分
    p.setPen(QPen(QColor("#334155"), 1));
    p.setFont(QFont("Noto Sans CJK SC", 9));
    for (int c : {0, 1024, 2048, 3072, 4095}) {
        p.drawLine(QPointF(plot.left(), py(c)), QPointF(plot.right(), py(c)));
        p.setPen(QColor("#94a3b8"));
        p.drawText(QRectF(0, py(c) - 8, L - 6, 16), Qt::AlignRight | Qt::AlignVCenter, QString::number(c));
        p.setPen(QPen(QColor("#334155"), 1));
    }
    const double span = n / rate;
    for (int k = 0; k <= 10; ++k) {
        const double x = plot.left() + plot.width() * k / 10;
        p.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
        p.setPen(QColor("#94a3b8"));
        const double t = span * k / 10;
        p.drawText(QRectF(x - 40, plot.bottom() + 4, 80, 16), Qt::AlignCenter,
                   span < 0.02 ? QString("%1 ms").arg(t * 1000, 0, 'f', 2) : QString("%1 s").arg(t, 0, 'f', 3));
        p.setPen(QPen(QColor("#334155"), 1));
    }
    // 波形：点数少时画出每个采样点
    QPainterPath path;
    for (int i = 0; i < n; ++i) i ? path.lineTo(px(i), py(v[i])) : path.moveTo(px(i), py(v[i]));
    p.setPen(QPen(QColor("#38bdf8"), 1.5));
    p.drawPath(path);
    if (n <= 300) {
        p.setBrush(QColor("#f97316"));
        p.setPen(Qt::NoPen);
        for (int i = 0; i < n; ++i) p.drawEllipse(QPointF(px(i), py(v[i])), 2.2, 2.2);
    }
    p.setPen(QColor("#e2e8f0"));
    p.setFont(QFont("Noto Sans CJK SC", 11));
    p.drawText(QRectF(L, 6, W - L - R, 22), Qt::AlignLeft | Qt::AlignVCenter, title);
    p.end();
    return img.save(QString::fromLocal8Bit(argv[3])) ? 0 : 1;
}
