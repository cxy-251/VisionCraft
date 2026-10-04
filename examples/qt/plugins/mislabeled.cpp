// 插件：代码按 1.0 接口编译，元数据里却写着 0.9（演示 QPluginLoader 不核对元数据里的 IID）
#include "operator.h"
#include <QObject>

#undef VC_OPERATOR_IID
#define VC_OPERATOR_IID "org.visioncraft.Operator/0.9"

class MislabeledOperator : public QObject, public Operator {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID VC_OPERATOR_IID)
    Q_INTERFACES(Operator)
public:
    QString name() const override { return QStringLiteral("标错版本号的插件"); }
    QByteArray apply(const QByteArray &px) const override { return px; }
};

#include "mislabeled.moc"
