// 委托：自己画「划痕」这一列，自己提供编辑器
//
// 运行：QT_QPA_PLATFORM=offscreen ./example_qt_delegate [截图目录]     输出见 delegate-output.txt

#include "partmodel.h"
#include <QApplication>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QTest>
#include <cstdio>

// [region delegate]
class ScratchDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    static constexpr double kThreshold = 18, kMax = 30;
    mutable int paints = 0;

    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &index) const override
    {
        ++paints;
        const double v = index.data(Qt::EditRole).toDouble();      // 拿数字，不拿格式化后的文字
        QStyledItemDelegate::paint(p, opt, QModelIndex());          // 先让默认委托画背景（选中时的高亮等）
        const QRect r = opt.rect.adjusted(4, 6, -4, -6);
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setPen(Qt::NoPen);
        p->setBrush(QColor("#e2e8f0"));
        p->drawRoundedRect(r, 3, 3);                                 // 底槽
        QRect bar = r;
        bar.setWidth(int(r.width() * qMin(v, kMax) / kMax));
        p->setBrush(QColor(v < kThreshold ? "#22c55e" : "#ef4444"));
        p->drawRoundedRect(bar, 3, 3);                               // 长度表示划痕，颜色表示是否超阈值
        const int tx = r.left() + int(r.width() * kThreshold / kMax);
        p->setPen(QPen(QColor("#334155"), 1, Qt::DashLine));
        p->drawLine(tx, r.top() - 2, tx, r.bottom() + 2);            // 阈值线
        p->setPen(QColor("#0f172a"));
        p->drawText(r.adjusted(6, 0, 0, 0), Qt::AlignVCenter, QString::number(v, 'f', 1));
        p->restore();
    }

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const override
    {
        auto *sb = new QDoubleSpinBox(parent);                       // parent 必须用传进来的那个
        sb->setRange(0, 50);
        sb->setDecimals(1);
        sb->setSingleStep(0.5);
        sb->setSuffix(" px");
        return sb;
    }
    void setEditorData(QWidget *editor, const QModelIndex &index) const override
    {
        static_cast<QDoubleSpinBox *>(editor)->setValue(index.data(Qt::EditRole).toDouble());
    }
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override
    {
        auto *sb = static_cast<QDoubleSpinBox *>(editor);
        sb->interpretText();                                         // 把还在输入框里没确认的文字也算上
        model->setData(index, sb->value(), Qt::EditRole);
    }
};
// [endregion]

// 打开编辑器、输入文字、按回车，返回编辑后模型里的值
static double editCell(QTableView &view, const QModelIndex &index, const QString &typed, QString *editorInfo = nullptr)
{
    view.setCurrentIndex(index);
    view.edit(index);
    QApplication::processEvents();
    QWidget *editor = view.indexWidget(index);
    if (!editor) editor = view.viewport()->focusWidget();
    if (editorInfo && editor) {
        *editorInfo = editor->metaObject()->className();
        if (auto *sb = qobject_cast<QDoubleSpinBox *>(editor))
            *editorInfo += QString("，范围 %1–%2，小数 %3 位").arg(sb->minimum()).arg(sb->maximum()).arg(sb->decimals());
    }
    if (auto *sb = qobject_cast<QAbstractSpinBox *>(editor)) {
        sb->selectAll();
        QTest::keyClicks(sb, typed);
        QTest::keyClick(sb, Qt::Key_Return);
    }
    QApplication::processEvents();
    return index.data(Qt::EditRole).toDouble();
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    const QString dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");

    PartModel model;
    for (int i = 0; i < 12; ++i)
        model.append(makePart(i));
    QTableView view;
    view.setModel(&model);
    view.horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    view.resize(560, 300);
    view.show();
    QApplication::processEvents();

    std::printf("==== 1. 默认委托的编辑器 ====\n");
    {
        QString info;
        const double v = editCell(view, model.index(0, PartModel::Scratch), "123.456", &info);
        std::printf("  编辑器：%s\n  输入 123.456 回车后，模型里是 %g\n", qPrintable(info), v);
    }

    std::printf("\n==== 2. 换上自定义委托 ====\n");
    // [region install]
    auto *delegate = new ScratchDelegate(&view);
    view.setItemDelegateForColumn(PartModel::Scratch, delegate);    // 只管这一列
    // [endregion]
    QApplication::processEvents();
    delegate->paints = 0;
    view.viewport()->repaint();
    std::printf("  重画一次，划痕列的 paint 被调用 %d 次（表格看得见 %d 行）\n", delegate->paints,
                view.rowAt(view.viewport()->height() - 1) + 1);
    {
        QString info;
        const double v = editCell(view, model.index(1, PartModel::Scratch), "123.456", &info);
        std::printf("  编辑器：%s\n  输入 123.456 回车后，模型里是 %g\n", qPrintable(info), v);
        // [region keystrokes]
        QDoubleSpinBox probe;                                   // 同样设置的输入框，一个字一个字地敲，看每一步剩下什么
        probe.setRange(0, 50);
        probe.setDecimals(1);
        probe.setSuffix(" px");
        probe.selectAll();                                      // 第一个字替换掉原来的 0.0
        QString trace;
        for (QChar c : QString("123.456")) {
            QTest::keyClick(&probe, c.toLatin1());
            trace += QString("  「%1」→ %2").arg(c).arg(probe.cleanText());
        }
        // [endregion]
        std::printf("  逐字敲入：%s\n", qPrintable(trace));
        const double v2 = editCell(view, model.index(2, PartModel::Scratch), "12.3", &info);
        std::printf("  第 2 行输入 12.3 回车后：模型里是 %g，结果列 %s\n", v2,
                    qPrintable(model.index(2, PartModel::Result).data().toString()));
    }
    view.clearSelection();
    view.setCurrentIndex(QModelIndex());
    QApplication::processEvents();
    view.grab().save(dir + "/delegate-bars.png");
    return 0;
}
