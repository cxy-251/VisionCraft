#pragma once

#include "core/IToolPage.h"
#include <QTabWidget>

class VisionMatcherPage;
class ImageProcessorPage;
class DevToolboxPage;

class IntegratedToolboxPage : public IToolPage {
    Q_OBJECT
public:
    explicit IntegratedToolboxPage(QWidget *parent = nullptr);
    ~IntegratedToolboxPage() override = default;

    QString id() const override { return "integrated_toolbox"; }
    QString title() const override { return "综合视觉与开发工具箱"; }
    QString description() const override { 
        return "一站式实用工具收纳：屏幕毫秒级找图、OpenCV 实时滤镜工坊、以及系统诊断与 JSON 语法格式化器。"; 
    }
    QString icon() const override { return "🧰"; }

    void onActivated() override;
    void onDeactivated() override;

private:
    QTabWidget *m_tabWidget = nullptr;
    VisionMatcherPage *m_matcherPage = nullptr;
    ImageProcessorPage *m_processorPage = nullptr;
    DevToolboxPage *m_devPage = nullptr;
};
