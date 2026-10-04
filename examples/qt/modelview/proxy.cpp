// 代理模型：不改原始数据，在它上面套一层做筛选、排序
//
// 运行：./example_qt_proxy     输出见 proxy-output.txt

#include "partmodel.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QRegularExpression>
#include <QSortFilterProxyModel>
#include <cstdio>

// [region custom-filter]
// 自定义筛选：只看 NG，且划痕在 [lo, hi] 之间
class NgFilter : public QSortFilterProxyModel {
public:
    void setRange(double lo, double hi)
    {
        m_lo = lo;
        m_hi = hi;
        invalidateFilter();                                   // 条件变了：通知代理重新筛一遍
    }
    void setRangeForgetInvalidate(double lo, double hi) { m_lo = lo; m_hi = hi; }   // 错误示范
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        const QModelIndex scratch = sourceModel()->index(sourceRow, PartModel::Scratch, sourceParent);
        const double v = scratch.data(Qt::EditRole).toDouble();
        const bool ng = sourceModel()->index(sourceRow, PartModel::Result, sourceParent).data().toString() == "NG";
        return ng && v >= m_lo && v <= m_hi;
    }
private:
    double m_lo = 0, m_hi = 1e9;
};
// [endregion]

static void printRows(const char *title, QAbstractItemModel *m, int n = 5)
{
    std::printf("  %s（共 %d 行）：", title, m->rowCount());
    for (int r = 0; r < qMin(n, m->rowCount()); ++r)
        std::printf(" %s/%s", qPrintable(m->index(r, PartModel::Id).data().toString()),
                    qPrintable(m->index(r, PartModel::Scratch).data().toString()));
    std::printf("\n");
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    PartModel model;
    for (int i = 0; i < 100000; ++i)
        model.append(makePart(i));

    std::printf("==== 1. 按编号筛选 ====\n");
    {
        // [region basic]
        QSortFilterProxyModel proxy;
        proxy.setSourceModel(&model);                         // 套在原始模型上面
        proxy.setFilterKeyColumn(PartModel::Id);              // 按哪一列筛
        QElapsedTimer t;
        t.start();
        proxy.setFilterRegularExpression(QRegularExpression("^A0123"));   // 编号以 A0123 开头
        // [endregion]
        const double setMs = t.nsecsElapsed() / 1e6;
        t.restart();
        const int rows = proxy.rowCount();
        std::printf("  setFilterRegularExpression 本身 %.2f ms；之后第一次 rowCount() %.1f ms，得到 %d 行\n",
                    setMs, t.nsecsElapsed() / 1e6, rows);
        t.restart();
        proxy.rowCount();
        std::printf("  第二次 rowCount() %.3f ms\n", t.nsecsElapsed() / 1e6);
        // [region refilter]
        t.restart();
        proxy.setFilterRegularExpression(QRegularExpression("^A0456"));   // 代理已经被用过了，再换条件
        const double again = t.nsecsElapsed() / 1e6;
        // [endregion]
        std::printf("  代理已被访问过之后再换条件：setFilterRegularExpression 本身 %.1f ms，得到 %d 行\n", again, proxy.rowCount());
        proxy.setFilterRegularExpression(QRegularExpression("^A0123"));
        printRows("结果", &proxy);
        std::printf("  原始模型仍是 %d 行\n", model.rowCount());

        // [region mapping]
        const QModelIndex p = proxy.index(2, PartModel::Id);       // 代理里的第 2 行
        const QModelIndex s = proxy.mapToSource(p);                // 对应原始模型的哪一行
        // [endregion]
        std::printf("  代理第 2 行是 %s，对应原始模型第 %d 行；如果拿代理的行号直接去原始模型取，得到的是 %s\n",
                    qPrintable(p.data().toString()), s.row(), qPrintable(model.index(2, PartModel::Id).data().toString()));
    }

    std::printf("\n==== 2. 按划痕排序 ====\n");
    {
        QSortFilterProxyModel proxy;
        proxy.setSourceModel(&model);
        proxy.setFilterRegularExpression(QRegularExpression("^A0000"));    // 只取 10 行，方便看
        proxy.setFilterKeyColumn(PartModel::Id);
        // [region sort]
        proxy.sort(PartModel::Scratch, Qt::DescendingOrder);  // 划痕从大到小
        // [endregion]
        printRows("默认（按显示文字排）", &proxy, 10);
        // [region sort-role]
        proxy.setSortRole(Qt::EditRole);                      // 改成按 EditRole（double）比较
        // [endregion]
        printRows("setSortRole(EditRole)", &proxy, 10);
    }

    std::printf("\n==== 3. 自定义筛选条件 ====\n");
    {
        NgFilter ng;
        ng.setSourceModel(&model);
        ng.setRange(18, 20);
        printRows("NG 且划痕 18–20", &ng, 3);
        ng.setRangeForgetInvalidate(25, 30);
        printRows("改成 25–30，但忘了 invalidateFilter", &ng, 3);
        ng.setRange(25, 30);
        printRows("改成 25–30，调用 invalidateFilter", &ng, 3);

        // [region dynamic]
        model.append({"Z99999", 26.0, false});                // 原始模型新加一行
        // [endregion]
        std::printf("  原始模型加了一行 Z99999/26.0：代理现在 %d 行，最后一行 %s\n", ng.rowCount(),
                    qPrintable(ng.index(ng.rowCount() - 1, PartModel::Id).data().toString()));
        // [region edit-through]
        const QModelIndex first = ng.index(0, PartModel::Scratch);
        const QString id = ng.index(0, PartModel::Id).data().toString();
        ng.setData(first, 5.0, Qt::EditRole);                 // 通过代理改：改的是原始模型
        // [endregion]
        std::printf("  通过代理把 %s 的划痕改成 5.0：它变成 OK，从代理里消失；代理现在 %d 行，第一行变成 %s\n",
                    qPrintable(id), ng.rowCount(), qPrintable(ng.index(0, PartModel::Id).data().toString()));
    }
    return 0;
}
