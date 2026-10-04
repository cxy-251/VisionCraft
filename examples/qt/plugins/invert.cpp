// 插件：反色。编译成单独的 .so，主程序运行时加载
#include "operator.h"
#include <QObject>

// [region plugin]
class InvertOperator : public QObject, public Operator {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID VC_OPERATOR_IID FILE "invert.json")      // 元数据：不加载插件也能读
    Q_INTERFACES(Operator)
public:
    QString name() const override { return QStringLiteral("反色 " VC_PLUGIN_VERSION); }
    QByteArray apply(const QByteArray &px) const override
    {
        QByteArray out(px.size(), 0);
        for (qsizetype i = 0; i < px.size(); ++i) out[i] = char(255 - uchar(px[i]));
        return out;
    }
};
// [endregion]

#include "invert.moc"
