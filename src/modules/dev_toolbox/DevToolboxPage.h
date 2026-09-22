#pragma once

#include "core/IToolPage.h"
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>

class DevToolboxPage : public IToolPage {
    Q_OBJECT
public:
    explicit DevToolboxPage(QWidget *parent = nullptr);
    ~DevToolboxPage() override = default;

    QString id() const override { return "dev_toolbox"; }
    QString title() const override { return "现代开发与系统工具箱"; }
    QString description() const override { 
        return "集成 nlohmann-json 格式化校验器与系统运行态诊断，体验现代 C++ 高性能数据处理。"; 
    }
    QString icon() const override { return "🛠️"; }

    void onActivated() override;

private slots:
    void formatJSON();
    void compressJSON();
    void refreshSystemInfo();

private:
    void setupUI();

    QTextEdit *m_inputJsonEdit = nullptr;
    QTextEdit *m_outputJsonEdit = nullptr;
    QLabel *m_jsonStatusLabel = nullptr;
    QLabel *m_sysInfoLabel = nullptr;
};
