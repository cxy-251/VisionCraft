// 检测算子的插件接口：主程序和插件都包含这个头文件
#pragma once

#include <QByteArray>
#include <QString>
#include <QtPlugin>

// [region interface]
class Operator {
public:
    virtual ~Operator() = default;
    virtual QString name() const = 0;
    virtual QByteArray apply(const QByteArray &pixels) const = 0;   // 输入、输出都是 8 位灰度像素
};

#define VC_OPERATOR_IID "org.visioncraft.Operator/1.0"            // 接口改了就改版本号
Q_DECLARE_INTERFACE(Operator, VC_OPERATOR_IID)
// [endregion]
