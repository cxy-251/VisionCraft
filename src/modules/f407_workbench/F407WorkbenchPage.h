#pragma once

#include "core/IToolPage.h"
#include <QSerialPort>
#include <QProcess>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QSpinBox>
#include <QSlider>
#include <opencv2/core.hpp>

class F407WorkbenchPage : public IToolPage {
    Q_OBJECT
public:
    explicit F407WorkbenchPage(QWidget *parent = nullptr);
    ~F407WorkbenchPage() override;

    static F407WorkbenchPage *instance();

    QString id() const override { return "f407_workbench"; }
    QString title() const override { return "STM32F407 硬件工作台"; }
    QString description() const override {
        return "直接控制 STM32F407：串口通信监视、OpenOCD 在线探测/烧录/复位与屏幕 RGB565 图像推流。";
    }
    QString icon() const override { return "🎛️"; }

    void onActivated() override;
    void onDeactivated() override;

public slots:
    void sendRawCommand(const QString &cmd);
    void sendVisionJudgeResult(bool passed, const QString &details = QString());

private slots:
    void refreshPorts();
    void toggleSerialPort();
    void onSerialDataReady();
    void sendSerialCommand();
    void clearSerialLog();

    // 快捷动作槽函数
    void sendBeep();
    void readSensors();
    void syncTime();
    void onDacApplyClicked();
    void onBacklightChanged(int val);
    void sendReboot();

    void probeMcu();
    void flashFirmware();
    void resetMcu();
    void onProcessOutput();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

    void generateTestPattern();
    void sendFrameToScreen();

private:
    void setupUI();
    void runOpenocdCommand(const QStringList &args);
    static F407WorkbenchPage *s_instance;

    // 快捷动作面板控件
    QSpinBox *m_dacSpinBox = nullptr;
    QSlider *m_blSlider = nullptr;
    QLabel *m_blLabel = nullptr;

    // 串口控件
    QSerialPort *m_serialPort = nullptr;
    QComboBox *m_portSelector = nullptr;
    QComboBox *m_baudSelector = nullptr;
    QPushButton *m_connectBtn = nullptr;
    QPushButton *m_refreshPortsBtn = nullptr;
    QTextEdit *m_serialLog = nullptr;
    QLineEdit *m_sendInput = nullptr;
    QPushButton *m_sendBtn = nullptr;

    // OpenOCD 进程与控件
    QProcess *m_openocdProcess = nullptr;
    QPushButton *m_probeBtn = nullptr;
    QPushButton *m_flashBtn = nullptr;
    QPushButton *m_resetBtn = nullptr;
    QTextEdit *m_openocdLog = nullptr;

    // 屏幕图像预览与推流
    cv::Mat m_currentFrame;
    QLabel *m_imgPreview = nullptr;
    QLabel *m_imgInfoLabel = nullptr;
    QPushButton *m_genPatternBtn = nullptr;
    QPushButton *m_pushFrameBtn = nullptr;
};
