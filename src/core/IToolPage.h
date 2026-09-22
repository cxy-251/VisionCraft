#pragma once

#include <QWidget>
#include <QString>

/**
 * @brief 统一工具模块接口
 * 任何新功能只需继承此类并在 ToolManager 中注册，即可自动展示在功能首页上。
 */
class IToolPage : public QWidget {
    Q_OBJECT
public:
    explicit IToolPage(QWidget *parent = nullptr) : QWidget(parent) {}
    virtual ~IToolPage() = default;

    // 工具唯一标识符 (例如 "vision_matcher")
    virtual QString id() const = 0;

    // 工具显示名称 (例如 "屏幕视觉与模板匹配")
    virtual QString title() const = 0;

    // 工具功能描述
    virtual QString description() const = 0;

    // 工具图标文字/Emoji (例如 "🎯")
    virtual QString icon() const = 0;

    // 切换到本工具时触发（可用于开启相机、启动定时器等）
    virtual void onActivated() {}

    // 离开本工具时触发（用于停止后台任务、释放资源）
    virtual void onDeactivated() {}
};
