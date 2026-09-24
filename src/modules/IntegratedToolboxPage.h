#pragma once

#include "core/IToolPage.h"
#include <QTabWidget>

class LiveScreenPipelinePage;
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
        return "一站式实用视觉工坊：实时屏幕流·算子流水线质检台、毫秒级找图、滤镜工坊与开发套件。"; 
    }
    QString icon() const override { return "🧰"; }

    void onActivated() override;
    void onDeactivated() override;

private:
    QTabWidget *m_tabWidget = nullptr;
    LiveScreenPipelinePage *m_livePipelinePage = nullptr;
    VisionMatcherPage *m_matcherPage = nullptr;
    ImageProcessorPage *m_processorPage = nullptr;
    DevToolboxPage *m_devPage = nullptr;
};
