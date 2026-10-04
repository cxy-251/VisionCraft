// 布局与伸缩因子：同一个窗口拉到不同宽度，打印每个控件分到的宽度，并截图
//
// 运行：QT_QPA_PLATFORM=offscreen ./example_qt_layout [截图目录]     输出见 output.txt

#include <QApplication>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>
#include <cstdio>

static QString g_dir;

// 彩色方块，最小尺寸 0，方便看出布局分给它多少
static QLabel *box(const QString &text, const char *color)
{
    auto *l = new QLabel(text);
    l->setAlignment(Qt::AlignCenter);
    l->setStyleSheet(QString("background:%1; color:white; font-weight:bold;").arg(color));
    l->setMinimumSize(0, 40);
    return l;
}

static void show(QWidget &w, const QList<QWidget *> &parts, const QList<int> &widths, const char *shot = nullptr)
{
    for (int width : widths) {
        w.resize(width, w.sizeHint().height());
        w.show();
        QApplication::processEvents();                       // 布局在事件循环里生效
        std::printf("  窗口 %4d：", width);
        for (QWidget *p : parts)
            std::printf("  %s=%d", qPrintable(p->objectName()), p->width());
        std::printf("\n");
        if (shot && width == widths.last())
            w.grab().save(g_dir + "/" + shot + ".png");
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    g_dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");

    std::printf("==== 1. 伸缩因子 1:3 ====\n");
    {
        // [region stretch]
        QWidget w;
        auto *row = new QHBoxLayout(&w);
        auto *list = box("列表 1", "#3b6ea5");
        auto *view = box("图像 3", "#4a8a4a");
        row->addWidget(list, 1);                              // 第二个参数就是伸缩因子
        row->addWidget(view, 3);
        // [endregion]
        list->setObjectName("列表"); view->setObjectName("图像");
        std::printf("  风格 %s，边距 %d，间隔 %d\n", qPrintable(QApplication::style()->name()),
                    row->contentsMargins().left(), row->spacing());
        // [region before-layout]
        w.resize(800, 60);
        std::printf("  show() 之前：列表宽 %d\n", list->width());
        // [endregion]
        show(w, {list, view}, {400, 800, 1200}, "layout-stretch");
    }

    std::printf("\n==== 2. 伸缩因子遇上最小宽度 ====\n");
    {
        // [region minimum]
        QWidget w;
        auto *row = new QHBoxLayout(&w);
        auto *list = box("列表 1", "#3b6ea5");
        auto *view = box("图像 3", "#4a8a4a");
        list->setMinimumWidth(250);                           // 列表至少 250 像素
        row->addWidget(list, 1);
        row->addWidget(view, 3);
        // [endregion]
        list->setObjectName("列表"); view->setObjectName("图像");
        show(w, {list, view}, {400, 800, 1200});
        w.resize(100, w.height());
        QApplication::processEvents();
        std::printf("  resize(100) 之后窗口实际宽 %d，minimumSizeHint 宽 %d\n", w.width(), w.minimumSizeHint().width());
    }

    std::printf("\n==== 3. 不设伸缩因子：按大小策略分 ====\n");
    {
        // [region policy]
        QWidget w;
        auto *row = new QHBoxLayout(&w);
        auto *button = new QPushButton("检测");               // 按钮：水平 Minimum —— 能变大，但不主动要空间
        auto *edit = new QLineEdit;                           // 输入框：水平 Expanding —— 主动要多余的空间
        auto *label = new QLabel("阈值");                     // 标签：水平 Preferred
        row->addWidget(label);
        row->addWidget(edit);
        row->addWidget(button);
        // [endregion]
        label->setObjectName("标签"); edit->setObjectName("输入框"); button->setObjectName("按钮");
        std::printf("  大小策略（水平）：标签 %d，输入框 %d，按钮 %d（Preferred=%d, Expanding=%d, Minimum=%d）\n",
                    label->sizePolicy().horizontalPolicy(), edit->sizePolicy().horizontalPolicy(),
                    button->sizePolicy().horizontalPolicy(), QSizePolicy::Preferred, QSizePolicy::Expanding, QSizePolicy::Minimum);
        std::printf("  sizeHint 宽度：标签 %d，输入框 %d，按钮 %d\n", label->sizeHint().width(), edit->sizeHint().width(), button->sizeHint().width());
        show(w, {label, edit, button}, {300, 600, 900}, "layout-policy");
    }

    std::printf("\n==== 4. addStretch：把按钮推到右边 ====\n");
    {
        // [region add-stretch]
        QWidget w;
        auto *row = new QHBoxLayout(&w);
        auto *ok = new QPushButton("确定");
        auto *cancel = new QPushButton("取消");
        row->addStretch();                                    // 一个看不见的弹簧，吃掉多余的空间
        row->addWidget(ok);
        row->addWidget(cancel);
        // [endregion]
        ok->setObjectName("确定"); cancel->setObjectName("取消");
        QLayoutItem *spring = row->itemAt(0);
        std::printf("  弹簧：row->stretch(0) = %d，水平策略 %d，expandingDirections 含水平 = %d\n", row->stretch(0),
                    spring->spacerItem()->sizePolicy().horizontalPolicy(), bool(spring->expandingDirections() & Qt::Horizontal));
        show(w, {ok, cancel}, {300, 600}, "layout-addstretch");
        std::printf("  「取消」右边缘 x = %d，窗口宽 %d，边距 %d\n", cancel->geometry().right() + 1, w.width(),
                    row->contentsMargins().right());
    }

    std::printf("\n==== 5. 网格：第 1 列伸缩 ====\n");
    {
        // [region grid]
        QWidget w;
        auto *grid = new QGridLayout(&w);
        const char *names[] = {"曝光", "增益", "阈值"};
        for (int r = 0; r < 3; ++r) {
            grid->addWidget(new QLabel(names[r]), r, 0);
            grid->addWidget(new QLineEdit, r, 1);
        }
        grid->addWidget(new QPushButton("应用"), 3, 0, 1, 2);  // 跨两列
        grid->setColumnStretch(1, 1);                        // 多余宽度全给第 1 列
        // [endregion]
        auto *label = qobject_cast<QWidget *>(grid->itemAtPosition(0, 0)->widget());
        auto *edit = qobject_cast<QWidget *>(grid->itemAtPosition(0, 1)->widget());
        auto *apply = qobject_cast<QWidget *>(grid->itemAtPosition(3, 0)->widget());
        label->setObjectName("标签列"); edit->setObjectName("输入列"); apply->setObjectName("应用按钮");
        show(w, {label, edit, apply}, {300, 600}, "layout-grid");
    }
    return 0;
}
