// QSS 样式表与换肤：用截图读回像素颜色，确认每条规则到底有没有生效
//
// 运行：QT_QPA_PLATFORM=offscreen ./example_qt_qss [截图目录]     输出见 output.txt

#include <QApplication>
#include <QCheckBox>
#include <QElapsedTimer>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>
#include <cstdio>

static QString g_dir;

// [region panel]
class BigButton : public QPushButton {                       // QPushButton 的子类
    Q_OBJECT
public:
    using QPushButton::QPushButton;
};

class Panel : public QWidget {                               // 自己写的 QWidget 子类，没有重写 paintEvent
    Q_OBJECT
};
// [endregion]

// 控件背景的颜色（先让事件循环把样式、布局都处理完）
static QString colorAt(QWidget *top, QWidget *w)
{
    QApplication::processEvents();
    const QPoint c = w->mapTo(top, QPoint(4, 4));             // 左上角往里 4 像素：避开 1 像素边框和文字
    return top->grab().toImage().pixelColor(c).name();
}

// [region themes]
static const char *kLight = R"(
    QWidget            { background: #f4f6f8; color: #1f2933; }
    QPushButton        { background: #dde3ea; border: 1px solid #9aa5b1; padding: 6px 14px; }
    QPushButton#start  { background: #2f80ed; color: white; }
    QPushButton:disabled { background: #e8e8e8; color: #a0a0a0; }
    QLabel[state="ok"] { background: #27ae60; color: white; }
    QLabel[state="ng"] { background: #eb5757; color: white; }
)";
static const char *kDark = R"(
    QWidget            { background: #1e2329; color: #e4e7eb; }
    QPushButton        { background: #323a45; border: 1px solid #52606d; padding: 6px 14px; }
    QPushButton#start  { background: #2d9cdb; color: white; }
    QPushButton:disabled { background: #2a2f36; color: #616e7c; }
    QLabel[state="ok"] { background: #219653; color: white; }
    QLabel[state="ng"] { background: #c0392b; color: white; }
)";
// [endregion]

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    g_dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");

    QWidget w;
    auto *col = new QVBoxLayout(&w);
    auto *start = new QPushButton("启动产线");
    start->setObjectName("start");                           // #start 选择器靠的就是 objectName
    auto *stop = new QPushButton("停止");
    auto *locked = new QPushButton("标定（未授权）");
    locked->setEnabled(false);
    auto *result = new QLabel("等待检测");
    result->setAlignment(Qt::AlignCenter);
    result->setMinimumHeight(40);
    result->setProperty("state", "ok");
    auto *edit = new QLineEdit("阈值 18");
    for (QWidget *x : {static_cast<QWidget *>(start), static_cast<QWidget *>(stop), static_cast<QWidget *>(locked),
                       static_cast<QWidget *>(result), static_cast<QWidget *>(edit)})
        col->addWidget(x);
    w.resize(320, w.sizeHint().height());
    w.show();

    std::printf("==== 1. 没有样式表（fusion 风格的默认外观）====\n");
    std::printf("  停止按钮 %s，结果标签 %s\n", qPrintable(colorAt(&w, stop)), qPrintable(colorAt(&w, result)));

    std::printf("\n==== 2. 浅色主题 ====\n");
    // [region apply]
    app.setStyleSheet(kLight);                               // 整个程序一份样式表
    // [endregion]
    std::printf("  启动 %s，停止 %s，禁用 %s，结果(ok) %s\n", qPrintable(colorAt(&w, start)), qPrintable(colorAt(&w, stop)),
                qPrintable(colorAt(&w, locked)), qPrintable(colorAt(&w, result)));
    w.grab().save(g_dir + "/qss-light.png");

    std::printf("\n==== 3. 属性变了，样式会跟着变吗 ====\n");
    {
        // [region property]
        result->setProperty("state", "ng");
        result->setText("NG");
        // [endregion]
        std::printf("  只 setProperty：结果标签 %s\n", qPrintable(colorAt(&w, result)));
        // [region repolish]
        result->style()->unpolish(result);                   // 让样式表按新属性重新匹配一次
        result->style()->polish(result);
        // [endregion]
        std::printf("  unpolish + polish 之后：结果标签 %s\n", qPrintable(colorAt(&w, result)));
    }

    std::printf("\n==== 4. 优先级 ====\n");
    {
        // [region specificity]
        start->setStyleSheet("background: #9b51e0;");        // 控件自己的样式表，没有选择器
        // [endregion]
        std::printf("  启动按钮自己的样式表 vs 程序的 #start：%s\n", qPrintable(colorAt(&w, start)));
        start->setStyleSheet(QString());
        std::printf("  清掉自己的样式表：%s\n", qPrintable(colorAt(&w, start)));
    }

    std::printf("\n==== 5. 换肤 ====\n");
    {
        // [region switch]
        QElapsedTimer t;
        t.start();
        app.setStyleSheet(kDark);
        QApplication::processEvents();
        const double ms = t.nsecsElapsed() / 1e6;
        // [endregion]
        std::printf("  切到深色：%.1f ms，启动 %s，停止 %s，结果(ng) %s\n", ms, qPrintable(colorAt(&w, start)),
                    qPrintable(colorAt(&w, stop)), qPrintable(colorAt(&w, result)));
        w.grab().save(g_dir + "/qss-dark.png");

        // 窗口里放 500 个按钮再换一次
        QWidget many;
        auto *grid = new QVBoxLayout(&many);
        for (int i = 0; i < 500; ++i)
            grid->addWidget(new QPushButton(QString::number(i)));
        many.show();
        QApplication::processEvents();
        t.restart();
        app.setStyleSheet(kLight);
        QApplication::processEvents();
        std::printf("  再加一个有 500 个按钮的窗口，切回浅色：%.1f ms\n", t.nsecsElapsed() / 1e6);
    }

    std::printf("\n==== 6. 样式表写错了 ====\n");
    {
        // [region typo]
        const char *sheets[] = {
            "QPushButton { background: #ff0000; border: none; }",                        // 正确
            "QPushButton { background: #ff0000; }",                                      // 正确，但没写 border
            "QPushButton { background: #ff0000; border: none; ",                         // 少了右花括号
            "QPushButton { background: #ff0000; border: none; } QLabel { color: ",       // 后面一条没写完
            "QPushButton { backgrund: #ff0000; border: none; }",                         // 属性名拼错
            "QPushButton { background: #ff0000; border: none; }; QLabel { color: red; }", // 花括号后面多了分号
        };
        // [endregion]
        int i = 0;
        for (const char *sheet : sheets) {
            std::fprintf(stderr, "[第 %d 份]\n", ++i);           // 方便对照 stderr 上的警告属于哪一份
            app.setStyleSheet(sheet);
            std::printf("  %d. %-72s → 停止按钮 %s\n", i, sheet, qPrintable(colorAt(&w, stop)));
        }
    }

    std::printf("\n==== 7. 自己写的 QWidget 子类 ====\n");
    {
        QWidget host;
        auto *lay = new QVBoxLayout(&host);
        auto *panel = new Panel;
        panel->setMinimumSize(100, 40);
        lay->addWidget(panel);
        host.setStyleSheet("QWidget { background: #ffffff; } Panel { background: #f2994a; }");
        host.resize(200, 80);
        host.show();
        std::printf("  Panel 背景 %s\n", qPrintable(colorAt(&host, panel)));
        // [region styled-bg]
        panel->setAttribute(Qt::WA_StyledBackground);        // 让 QWidget 按样式表画背景
        // [endregion]
        panel->update();
        std::printf("  设置 WA_StyledBackground 之后 %s\n", qPrintable(colorAt(&host, panel)));
    }

    std::printf("\n==== 8. 补充核对 ====\n");
    {
        app.setStyleSheet(kLight);
        locked->setEnabled(true);                            // 伪状态变化：不 polish
        std::printf("  禁用按钮改成可用（没有 polish）：%s\n", qPrintable(colorAt(&w, locked)));
        auto *big = new BigButton("子类按钮");
        col->addWidget(big);
        std::printf("  QPushButton 的子类：%s\n", qPrintable(colorAt(&w, big)));
        app.setStyleSheet("QPushButton#start { background: #2f80ed; border: none; } QPushButton { background: #dde3ea; border: none; }");
        std::printf("  #start 规则写在前面：启动 %s，停止 %s\n", qPrintable(colorAt(&w, start)), qPrintable(colorAt(&w, stop)));
        app.setStyleSheet(kLight);
        start->setStyleSheet("QPushButton { background: #9b51e0; }");
        std::printf("  启动按钮自己的样式表带选择器 QPushButton：%s\n", qPrintable(colorAt(&w, start)));
        start->setStyleSheet("QLabel { background: #9b51e0; }");
        std::printf("  启动按钮自己的样式表带选择器 QLabel：%s\n", qPrintable(colorAt(&w, start)));
    }
    return 0;
}

#include "main.moc"
