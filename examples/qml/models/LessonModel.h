#pragma once
#include <QAbstractListModel>
#include <QVector>
// [region model]
class LessonModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit LessonModel(QObject *parent=nullptr):QAbstractListModel(parent) {}
    enum Role { LabelRole=Qt::UserRole+1, AmountRole };
    int rowCount(const QModelIndex &parent={}) const override { return parent.isValid()?0:int(m_rows.size()); }
    QHash<int,QByteArray> roleNames() const override { return {{LabelRole,"label"},{AmountRole,"amount"}}; }
    QVariant data(const QModelIndex &index,int role) const override {
        if(!index.isValid() || index.row()<0 || index.row()>=m_rows.size()) return {};
        const auto &row=m_rows[index.row()];
        if(role==LabelRole)return row.label;
        if(role==AmountRole)return row.amount;
        return {};
    }
    Q_INVOKABLE void setAmount(int row,int amount) {
        if(row<0 || row>=m_rows.size() || amount<0 || amount>100 || amount==m_rows[row].amount)return;
        m_rows[row].amount=amount;
        emit dataChanged(index(row,0),index(row,0),{AmountRole});
    }
    Q_INVOKABLE void appendRow() {
        if(m_rows.size()>=20)return;
        const int row=int(m_rows.size());
        beginInsertRows({},row,row);
        m_rows.append({QString("项目%1").arg(row+1),0});
        endInsertRows();
    }
    Q_INVOKABLE void removeLast() {
        if(m_rows.isEmpty())return;
        const int row=int(m_rows.size()-1);
        beginRemoveRows({},row,row);m_rows.removeLast();endRemoveRows();
    }
private:
    struct Row {QString label;int amount;};
    QVector<Row> m_rows{{"项目1",2},{"项目2",5},{"项目3",8}};
};
// [endregion]
