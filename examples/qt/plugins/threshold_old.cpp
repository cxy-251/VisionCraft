// 插件：阈值。按旧版（0.9）接口头文件编译：模拟「插件是用老版本主程序的头文件编出来的」
#include <QObject>
#include <QtPlugin>

// [region old-interface]
class Operator {                                              // 旧版接口：当时还没有 apply 的这种签名
public:
    virtual ~Operator() = default;
    virtual QString name() const = 0;
};
#define VC_OPERATOR_IID "org.visioncraft.Operator/0.9"
Q_DECLARE_INTERFACE(Operator, VC_OPERATOR_IID)
// [endregion]

class ThresholdOperator : public QObject, public Operator {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID VC_OPERATOR_IID)
    Q_INTERFACES(Operator)
public:
    QString name() const override { return QStringLiteral("阈值（旧版）"); }
};

#include "threshold_old.moc"
