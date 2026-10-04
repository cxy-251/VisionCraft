// 检测记录的表格模型：本目录几个示例共用
#pragma once

#include <QAbstractTableModel>
#include <QColor>
#include <QVector>

struct Part {
    QString id;
    double scratch;     // 划痕长度（像素）
    bool ok;
};

// [region model]
class PartModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column { Id, Scratch, Result, ColumnCount };

    int rowCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : int(m_parts.size()); }
    int columnCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : ColumnCount; }

    QVariant data(const QModelIndex &index, int role) const override
    {
        ++dataCalls;
        const Part &p = m_parts.at(index.row());
        if (role == Qt::DisplayRole) {
            switch (index.column()) {
            case Id: return p.id;
            case Scratch: return QString::number(p.scratch, 'f', 1);
            case Result: return p.ok ? "OK" : "NG";
            }
        }
        if (role == Qt::ForegroundRole && index.column() == Result)
            return QColor(p.ok ? "#16a34a" : "#dc2626");      // 同一个格子，不同的「角色」返回不同的东西
        if (role == Qt::EditRole && index.column() == Scratch)
            return p.scratch;                                 // 编辑时给数字，不给格式化后的字符串
        return {};
    }

    QVariant headerData(int section, Qt::Orientation o, int role) const override
    {
        if (o == Qt::Horizontal && role == Qt::DisplayRole)
            return QStringList{"编号", "划痕", "结果"}.value(section);
        return QAbstractTableModel::headerData(section, o, role);
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        Qt::ItemFlags f = QAbstractTableModel::flags(index);
        if (index.column() == Scratch) f |= Qt::ItemIsEditable;
        return f;
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role) override
    {
        if (role != Qt::EditRole || index.column() != Scratch)
            return false;
        Part &p = m_parts[index.row()];
        p.scratch = value.toDouble();
        p.ok = p.scratch < 18;                                // 阈值 18：改了划痕，结果跟着变
        emit dataChanged(index, this->index(index.row(), Result));   // 通知所有视图：这两格变了
        return true;
    }

    void append(const Part &p)                                // 插入行：先 begin，改数据，再 end
    {
        beginInsertRows({}, int(m_parts.size()), int(m_parts.size()));
        m_parts.append(p);
        endInsertRows();
    }

    mutable long dataCalls = 0;                               // 示例用：data() 被调用的次数

private:
    QVector<Part> m_parts;
};
// [endregion]

inline Part makePart(int i)
{
    const double s = (i * 37 % 41) * 0.7;                     // 0–28 之间的伪随机划痕长度
    return {QString("A%1").arg(i, 5, 10, QChar('0')), s, s < 18};
}
