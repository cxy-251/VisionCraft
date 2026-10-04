// Model / View 架构：一个模型、两个视图；视图只读看得见的那几行
//
// 运行：QT_QPA_PLATFORM=offscreen ./example_qt_modelview [截图目录]     输出见 output.txt

#include "partmodel.h"
#include <QAbstractItemModelTester>
#include <QApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QListView>
#include <QTableView>
#include <QElapsedTimer>
#include <cstdio>

// [region broken]
// 一个有错的模型：加行时忘了 beginInsertRows / endInsertRows
// 另一个错误：声明插入 2 行，实际只加了 1 行
class BrokenModel : public QAbstractListModel {
public:
    int rowCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : int(rows.size()); }
    QVariant data(const QModelIndex &i, int role) const override { return role == Qt::DisplayRole ? rows.at(i.row()) : QVariant(); }
    void appendWrong(const QString &s) { rows.append(s); }   // 视图不知道多了一行
    void appendWrongRange(const QString &s)
    {
        beginInsertRows({}, int(rows.size()), int(rows.size()) + 1);   // 范围写成了 2 行
        rows.append(s);
        endInsertRows();
    }
    QStringList rows;
};
// [endregion]

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    const QString dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");

    std::printf("==== 1. 10 万行，视图读了多少 ====\n");
    PartModel model;
    QElapsedTimer t;
    t.start();
    for (int i = 0; i < 100000; ++i)
        model.append(makePart(i));
    std::printf("  模型里放 %d 行：%.0f ms\n", model.rowCount(), t.nsecsElapsed() / 1e6);

    // [region views]
    QWidget w;
    auto *row = new QHBoxLayout(&w);
    auto *table = new QTableView;
    auto *list = new QListView;
    table->setModel(&model);                                  // 两个视图，同一个模型
    list->setModel(&model);
    list->setModelColumn(PartModel::Id);                      // 列表只显示一列
    row->addWidget(table, 3);
    row->addWidget(list, 1);
    // [endregion]
    table->horizontalHeader()->setStretchLastSection(true);
    w.resize(520, 300);
    model.dataCalls = 0;
    t.restart();
    w.show();
    QApplication::processEvents();
    std::printf("  显示出来：%.0f ms，data() 被调用 %ld 次\n", t.nsecsElapsed() / 1e6, model.dataCalls);
    model.dataCalls = 0;
    table->scrollTo(model.index(50000, 0), QAbstractItemView::PositionAtTop);
    QApplication::processEvents();
    std::printf("  表格滚到第 50000 行：data() 又被调用 %ld 次\n", model.dataCalls);
    table->scrollToTop();
    QApplication::processEvents();

    // [region which-view]
    auto measure = [&](const char *name, QAbstractItemView *v) {
        v->resize(400, 300);
        model.dataCalls = 0;
        QElapsedTimer t2;
        t2.start();
        v->show();
        QApplication::processEvents();
        std::printf("  单独显示 %-36s %4.0f ms，data() %ld 次\n", name, t2.nsecsElapsed() / 1e6, model.dataCalls);
        v->hide();
    };
    // [endregion]
    { QTableView v; v.setModel(&model); measure("QTableView", &v); }
    { QListView v; v.setModel(&model); measure("QListView", &v); }
    // [region uniform]
    { QListView v; v.setModel(&model); v.setUniformItemSizes(true); measure("QListView + setUniformItemSizes(true)", &v); }
    // [endregion]
    list->setUniformItemSizes(true);

    std::printf("\n==== 2. 改一格，两个视图都知道 ====\n");
    {
        int changed = 0;
        QObject::connect(&model, &QAbstractItemModel::dataChanged, [&](const QModelIndex &a, const QModelIndex &b) {
            ++changed;
            std::printf("  dataChanged：行 %d，列 %d–%d\n", a.row(), a.column(), b.column());
        });
        const QModelIndex scratch = model.index(4, PartModel::Scratch);
        std::printf("  第 4 行改之前：划痕 %s，结果 %s\n", qPrintable(scratch.data().toString()),
                    qPrintable(model.index(4, PartModel::Result).data().toString()));
        // [region edit]
        model.setData(scratch, 25.0, Qt::EditRole);           // 等同于用户在表格里双击改值
        // [endregion]
        std::printf("  第 4 行改之后：划痕 %s，结果 %s，颜色 %s\n", qPrintable(scratch.data().toString()),
                    qPrintable(model.index(4, PartModel::Result).data().toString()),
                    qPrintable(model.index(4, PartModel::Result).data(Qt::ForegroundRole).value<QColor>().name()));
        std::printf("  第 4 行划痕格：DisplayRole = \"%s\"（%s），EditRole = %s（%s）\n",
                    qPrintable(scratch.data(Qt::DisplayRole).toString()), scratch.data(Qt::DisplayRole).typeName(),
                    qPrintable(scratch.data(Qt::EditRole).toString()), scratch.data(Qt::EditRole).typeName());
        w.resize(860, 360);                                   // 截图用：大一点，放进手册不用放大
        QApplication::processEvents();
        w.grab().save(dir + "/modelview-two-views.png");
    }

    std::printf("\n==== 3. 选择也可以共享 ====\n");
    {
        // [region selection]
        list->setSelectionModel(table->selectionModel());     // 两个视图用同一个选择模型
        table->selectRow(5);
        // [endregion]
        std::printf("  表格选中第 5 行后，列表的当前选中：%s\n",
                    qPrintable(list->selectionModel()->selectedIndexes().value(0).data().toString()));
    }

    std::printf("\n==== 4. 模型写错了 ====\n");
    {
        BrokenModel broken;
        broken.rows = {"A", "B"};
        QListView view;
        view.setModel(&broken);
        view.show();
        QApplication::processEvents();
        broken.appendWrong("C");
        std::fflush(stdout);
        QApplication::processEvents();
        std::printf("  忘了 beginInsertRows：模型有 %d 行，视图里第 3 行 %s\n", broken.rowCount(),
                    view.visualRect(broken.index(2)).isValid() ? "有位置" : "没有位置（visualRect 无效，画不出来）");
        std::fflush(stdout);
        new QAbstractItemModelTester(&broken, QAbstractItemModelTester::FailureReportingMode::Warning, &app);
        std::printf("  接上 QAbstractItemModelTester，再用错误的范围插入一行（警告见 stderr）：\n");
        std::fflush(stdout);
        broken.appendWrongRange("D");
        // [region tester]
        new QAbstractItemModelTester(&model, QAbstractItemModelTester::FailureReportingMode::Warning, &app);
        // [endregion]
        model.append(makePart(100000));
        std::printf("  PartModel 接上 QAbstractItemModelTester 后再加一行：（stderr 上没有新警告）\n");
        std::fflush(stdout);
    }
    return 0;
}

#include "main.moc"
