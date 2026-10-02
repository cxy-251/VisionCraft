#include "F407WorkbenchPage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QSerialPortInfo>
#include <QFileDialog>
#include <QDateTime>
#include <QImage>
#include <QPixmap>
#include <QScrollBar>
#include <QStandardPaths>
#include <QDir>
#include <opencv2/imgproc.hpp>

F407WorkbenchPage *F407WorkbenchPage::s_instance = nullptr;

F407WorkbenchPage *F407WorkbenchPage::instance() {
    return s_instance;
}

F407WorkbenchPage::F407WorkbenchPage(QWidget *parent)
    : IToolPage(parent),
      m_serialPort(new QSerialPort(this)),
      m_openocdProcess(new QProcess(this)) {
    s_instance = this;
    setupUI();

    connect(m_serialPort, &QSerialPort::readyRead, this, &F407WorkbenchPage::onSerialDataReady);
    connect(m_openocdProcess, &QProcess::readyReadStandardOutput, this, &F407WorkbenchPage::onProcessOutput);
    connect(m_openocdProcess, &QProcess::readyReadStandardError, this, &F407WorkbenchPage::onProcessOutput);
    connect(m_openocdProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &F407WorkbenchPage::onProcessFinished);

    refreshPorts();
    generateTestPattern();
}

F407WorkbenchPage::~F407WorkbenchPage() {
    if (s_instance == this) {
        s_instance = nullptr;
    }
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
    }
    if (m_openocdProcess->state() != QProcess::NotRunning) {
        m_openocdProcess->kill();
        m_openocdProcess->waitForFinished(500);
    }
}

void F407WorkbenchPage::onActivated() {
    refreshPorts();
}

void F407WorkbenchPage::onDeactivated() {
    // 离开页面时保持连接或保留日志
}

void F407WorkbenchPage::setupUI() {
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(12);

    // ==========================================
    // 左栏：串口交互与终端
    // ==========================================
    auto *serialGroup = new QGroupBox("📡 串口通信控制台", this);
    auto *serialLayout = new QVBoxLayout(serialGroup);
    serialLayout->setSpacing(8);

    auto *portConfigRow = new QHBoxLayout();
    m_portSelector = new QComboBox(serialGroup);
    m_portSelector->setMinimumWidth(140);

    m_baudSelector = new QComboBox(serialGroup);
    m_baudSelector->addItems({"115200", "921600", "460800", "57600", "9600"});

    m_refreshPortsBtn = new QPushButton("🔄 刷新", serialGroup);
    connect(m_refreshPortsBtn, &QPushButton::clicked, this, &F407WorkbenchPage::refreshPorts);

    m_connectBtn = new QPushButton("🔌 连接串口", serialGroup);
    connect(m_connectBtn, &QPushButton::clicked, this, &F407WorkbenchPage::toggleSerialPort);

    portConfigRow->addWidget(new QLabel("端口:"));
    portConfigRow->addWidget(m_portSelector);
    portConfigRow->addWidget(new QLabel("波特率:"));
    portConfigRow->addWidget(m_baudSelector);
    portConfigRow->addWidget(m_refreshPortsBtn);
    portConfigRow->addWidget(m_connectBtn);
    serialLayout->addLayout(portConfigRow);

    m_serialLog = new QTextEdit(serialGroup);
    m_serialLog->setReadOnly(true);
    m_serialLog->setStyleSheet("background-color: #0f172a; color: #38bdf8; font-family: monospace; font-size: 11px;");
    serialLayout->addWidget(m_serialLog, 1);

    auto *sendRow = new QHBoxLayout();
    m_sendInput = new QLineEdit(serialGroup);
    m_sendInput->setPlaceholderText("输入指令回车发送...");
    connect(m_sendInput, &QLineEdit::returnPressed, this, &F407WorkbenchPage::sendSerialCommand);

    m_sendBtn = new QPushButton("发送", serialGroup);
    connect(m_sendBtn, &QPushButton::clicked, this, &F407WorkbenchPage::sendSerialCommand);

    auto *clearSerialBtn = new QPushButton("清屏", serialGroup);
    connect(clearSerialBtn, &QPushButton::clicked, this, &F407WorkbenchPage::clearSerialLog);

    sendRow->addWidget(m_sendInput, 1);
    sendRow->addWidget(m_sendBtn);
    sendRow->addWidget(clearSerialBtn);
    serialLayout->addLayout(sendRow);

    // 快捷控制按钮面板
    auto *quickGroup = new QGroupBox("🎮 硬件快捷控制面板", serialGroup);
    auto *quickLayout = new QVBoxLayout(quickGroup);
    quickLayout->setSpacing(6);

    auto *quickBtnRow = new QHBoxLayout();
    auto *beepBtn = new QPushButton("🔊 蜂鸣测试", quickGroup);
    connect(beepBtn, &QPushButton::clicked, this, &F407WorkbenchPage::sendBeep);

    auto *sensorBtn = new QPushButton("🌡️ 读传感器", quickGroup);
    connect(sensorBtn, &QPushButton::clicked, this, &F407WorkbenchPage::readSensors);

    auto *syncTimeBtn = new QPushButton("🕒 同步电脑时间", quickGroup);
    connect(syncTimeBtn, &QPushButton::clicked, this, &F407WorkbenchPage::syncTime);

    auto *rebootBtn = new QPushButton("🔄 软复位", quickGroup);
    connect(rebootBtn, &QPushButton::clicked, this, &F407WorkbenchPage::sendReboot);

    quickBtnRow->addWidget(beepBtn);
    quickBtnRow->addWidget(sensorBtn);
    quickBtnRow->addWidget(syncTimeBtn);
    quickBtnRow->addWidget(rebootBtn);
    quickLayout->addLayout(quickBtnRow);

    // DAC 设定行
    auto *dacRow = new QHBoxLayout();
    dacRow->addWidget(new QLabel("⚡ DAC 输出 (PA4):", quickGroup));
    m_dacSpinBox = new QSpinBox(quickGroup);
    m_dacSpinBox->setRange(0, 3300);
    m_dacSpinBox->setValue(1500);
    m_dacSpinBox->setSuffix(" mV");
    m_dacSpinBox->setSingleStep(100);
    auto *dacApplyBtn = new QPushButton("设定", quickGroup);
    connect(dacApplyBtn, &QPushButton::clicked, this, &F407WorkbenchPage::onDacApplyClicked);
    dacRow->addWidget(m_dacSpinBox, 1);
    dacRow->addWidget(dacApplyBtn);
    quickLayout->addLayout(dacRow);

    // 背光调节行
    auto *blRow = new QHBoxLayout();
    blRow->addWidget(new QLabel("☀️ 屏幕背光:", quickGroup));
    m_blSlider = new QSlider(Qt::Horizontal, quickGroup);
    m_blSlider->setRange(5, 100);
    m_blSlider->setValue(80);
    m_blLabel = new QLabel("80%", quickGroup);
    m_blLabel->setFixedWidth(40);
    connect(m_blSlider, &QSlider::valueChanged, this, &F407WorkbenchPage::onBacklightChanged);
    blRow->addWidget(m_blSlider, 1);
    blRow->addWidget(m_blLabel);
    quickLayout->addLayout(blRow);

    serialLayout->addWidget(quickGroup);

    mainLayout->addWidget(serialGroup, 1);

    // ==========================================
    // 右栏：OpenOCD 与 屏幕图传控制
    // ==========================================
    auto *rightCol = new QVBoxLayout();
    rightCol->setSpacing(12);

    // 1. OpenOCD 烧录与调试
    auto *ocdGroup = new QGroupBox("⚡ OpenOCD 硬件调试 / 烧录", this);
    auto *ocdLayout = new QVBoxLayout(ocdGroup);
    ocdLayout->setSpacing(8);

    auto *ocdBtnRow = new QHBoxLayout();
    m_probeBtn = new QPushButton("🔍 探测芯片", ocdGroup);
    connect(m_probeBtn, &QPushButton::clicked, this, &F407WorkbenchPage::probeMcu);

    m_flashBtn = new QPushButton("💾 选择固件烧录", ocdGroup);
    connect(m_flashBtn, &QPushButton::clicked, this, &F407WorkbenchPage::flashFirmware);

    m_resetBtn = new QPushButton("🔄 目标复位", ocdGroup);
    connect(m_resetBtn, &QPushButton::clicked, this, &F407WorkbenchPage::resetMcu);

    ocdBtnRow->addWidget(m_probeBtn);
    ocdBtnRow->addWidget(m_flashBtn);
    ocdBtnRow->addWidget(m_resetBtn);
    ocdLayout->addLayout(ocdBtnRow);

    m_openocdLog = new QTextEdit(ocdGroup);
    m_openocdLog->setReadOnly(true);
    m_openocdLog->setFixedHeight(150);
    m_openocdLog->setStyleSheet("background-color: #020617; color: #a5f3fc; font-family: monospace; font-size: 11px;");
    ocdLayout->addWidget(m_openocdLog);

    rightCol->addWidget(ocdGroup);

    // 2. 屏幕驱动与图像流
    auto *screenGroup = new QGroupBox("🖥️ F407 屏幕图像流控制 (RGB565)", this);
    auto *screenLayout = new QVBoxLayout(screenGroup);
    screenLayout->setSpacing(8);

    auto *screenBtnRow = new QHBoxLayout();
    m_genPatternBtn = new QPushButton("🎨 生成测试图", screenGroup);
    connect(m_genPatternBtn, &QPushButton::clicked, this, &F407WorkbenchPage::generateTestPattern);

    m_pushFrameBtn = new QPushButton("🚀 推送到屏幕", screenGroup);
    connect(m_pushFrameBtn, &QPushButton::clicked, this, &F407WorkbenchPage::sendFrameToScreen);

    screenBtnRow->addWidget(m_genPatternBtn);
    screenBtnRow->addWidget(m_pushFrameBtn);
    screenLayout->addLayout(screenBtnRow);

    m_imgPreview = new QLabel(screenGroup);
    m_imgPreview->setFixedSize(320, 180);
    m_imgPreview->setAlignment(Qt::AlignCenter);
    m_imgPreview->setStyleSheet("background-color: #1e293b; border: 1px solid #334155; border-radius: 4px;");
    screenLayout->addWidget(m_imgPreview, 0, Qt::AlignCenter);

    m_imgInfoLabel = new QLabel("分辨率: 320x240 | 格式: RGB565 | 单帧大小: 153.6 KB", screenGroup);
    m_imgInfoLabel->setAlignment(Qt::AlignCenter);
    m_imgInfoLabel->setStyleSheet("color: #94a3b8; font-size: 11px;");
    screenLayout->addWidget(m_imgInfoLabel);

    rightCol->addWidget(screenGroup, 1);

    mainLayout->addLayout(rightCol, 1);
}

void F407WorkbenchPage::refreshPorts() {
    m_portSelector->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    for (const auto &port : ports) {
        QString desc = QString("%1 (%2)").arg(port.portName(), port.description());
        m_portSelector->addItem(desc, port.systemLocation());
    }
    if (m_portSelector->count() == 0) {
        m_portSelector->addItem("无可用串口设备", "");
    }
}

void F407WorkbenchPage::toggleSerialPort() {
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
        m_connectBtn->setText("🔌 连接串口");
        m_portSelector->setEnabled(true);
        m_baudSelector->setEnabled(true);
        m_serialLog->append("[系统] 串口已关闭。\n");
        return;
    }

    QString portPath = m_portSelector->currentData().toString();
    if (portPath.isEmpty()) {
        m_serialLog->append("[错误] 未选中有效串口设备。\n");
        return;
    }

    qint32 baud = m_baudSelector->currentText().toInt();
    m_serialPort->setPortName(portPath);
    m_serialPort->setBaudRate(baud);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (m_serialPort->open(QIODevice::ReadWrite)) {
        m_connectBtn->setText("❌ 断开串口");
        m_portSelector->setEnabled(false);
        m_baudSelector->setEnabled(false);
        m_serialLog->append(QString("[系统] 成功打开串口 %1, 波特率 %2\n").arg(portPath).arg(baud));
    } else {
        m_serialLog->append(QString("[错误] 无法打开串口: %1 (错误码: %2)\n")
                                .arg(m_serialPort->errorString())
                                .arg(m_serialPort->error()));
    }
}

void F407WorkbenchPage::onSerialDataReady() {
    QByteArray data = m_serialPort->readAll();
    m_serialLog->insertPlainText(QString::fromUtf8(data));
    m_serialLog->verticalScrollBar()->setValue(m_serialLog->verticalScrollBar()->maximum());
}

void F407WorkbenchPage::sendSerialCommand() {
    if (!m_serialPort->isOpen()) {
        m_serialLog->append("[提示] 请先连接串口。\n");
        return;
    }
    QString text = m_sendInput->text().trimmed();
    if (text.isEmpty()) return;

    QByteArray data = text.toUtf8();
    data.append("\r\n");
    m_serialPort->write(data);
    m_serialLog->append(QString(">> %1\n").arg(text));
    m_sendInput->clear();
}

void F407WorkbenchPage::clearSerialLog() {
    m_serialLog->clear();
}

void F407WorkbenchPage::runOpenocdCommand(const QStringList &args) {
    if (m_openocdProcess->state() != QProcess::NotRunning) {
        m_openocdLog->append("[错误] 另一个 OpenOCD 任务正在执行中。\n");
        return;
    }
    m_openocdLog->clear();
    m_openocdLog->append("[执行] openocd " + args.join(" ") + "\n");

    QString program = QStandardPaths::findExecutable("openocd");
    if (program.isEmpty()) {
        program = QDir::homePath() + "/Applications/openocd/usr/bin/openocd";
    }
    m_openocdProcess->start(program, args);
}

void F407WorkbenchPage::probeMcu() {
    QStringList args = {
        "-f", "interface/stlink.cfg",
        "-f", "target/stm32f4x.cfg",
        "-c", "init; targets; exit"
    };
    runOpenocdCommand(args);
}

void F407WorkbenchPage::flashFirmware() {
    QString file = QFileDialog::getOpenFileName(this, "选择 STM32 固件", "", "固件文件 (*.elf *.bin *.hex)");
    if (file.isEmpty()) return;

    QString cmd = QString("program \"%1\" verify reset exit").arg(file);
    QStringList args = {
        "-f", "interface/stlink.cfg",
        "-f", "target/stm32f4x.cfg",
        "-c", cmd
    };
    runOpenocdCommand(args);
}

void F407WorkbenchPage::resetMcu() {
    QStringList args = {
        "-f", "interface/stlink.cfg",
        "-f", "target/stm32f4x.cfg",
        "-c", "init; reset run; exit"
    };
    runOpenocdCommand(args);
}

void F407WorkbenchPage::onProcessOutput() {
    QByteArray stdoutData = m_openocdProcess->readAllStandardOutput();
    QByteArray stderrData = m_openocdProcess->readAllStandardError();
    if (!stdoutData.isEmpty()) m_openocdLog->insertPlainText(QString::fromUtf8(stdoutData));
    if (!stderrData.isEmpty()) m_openocdLog->insertPlainText(QString::fromUtf8(stderrData));
    m_openocdLog->verticalScrollBar()->setValue(m_openocdLog->verticalScrollBar()->maximum());
}

void F407WorkbenchPage::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    m_openocdLog->append(QString("\n[完成] 进程退出，退出码: %1\n").arg(exitCode));
}

void F407WorkbenchPage::generateTestPattern() {
    m_currentFrame = cv::Mat(240, 320, CV_8UC3, cv::Scalar(20, 24, 39));
    cv::circle(m_currentFrame, cv::Point(60, 60), 35, cv::Scalar(0, 165, 255), -1);
    cv::rectangle(m_currentFrame, cv::Rect(140, 30, 70, 70), cv::Scalar(0, 255, 128), -1);
    cv::putText(m_currentFrame, "F407 RGB565 Stream", cv::Point(20, 140),
                cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(255, 255, 255), 2);

    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss");
    cv::putText(m_currentFrame, timeStr.toStdString(), cv::Point(20, 180),
                cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(56, 189, 248), 2);

    // OpenCV BGR -> QImage RGB888
    cv::Mat rgb;
    cv::cvtColor(m_currentFrame, rgb, cv::COLOR_BGR2RGB);
    QImage qimg(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);
    m_imgPreview->setPixmap(QPixmap::fromImage(qimg.scaled(m_imgPreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));
}

void F407WorkbenchPage::sendFrameToScreen() {
    if (!m_serialPort->isOpen()) {
        m_serialLog->append("[错误] 屏幕推流失败：请先连接串口。\n");
        return;
    }
    if (m_currentFrame.empty()) {
        generateTestPattern();
    }

    // 转换色彩空间为单片机屏幕常用的 RGB565
    cv::Mat rgb565;
    cv::cvtColor(m_currentFrame, rgb565, cv::COLOR_BGR2BGR565);

    // 打包帧协议: 帧头 0xAA55 (2B) + 宽 (2B) + 高 (2B) + 像素负载
    QByteArray packet;
    packet.append(static_cast<char>(0xAA));
    packet.append(static_cast<char>(0x55));
    uint16_t w = static_cast<uint16_t>(rgb565.cols);
    uint16_t h = static_cast<uint16_t>(rgb565.rows);
    packet.append(reinterpret_cast<const char*>(&w), 2);
    packet.append(reinterpret_cast<const char*>(&h), 2);
    packet.append(reinterpret_cast<const char*>(rgb565.data), rgb565.total() * 2);

    qint64 written = m_serialPort->write(packet);
    m_serialLog->append(QString("[推流] 已通过串口发送测试帧：%1 字节 (RGB565 %2x%3)\n")
                            .arg(written).arg(w).arg(h));
}

void F407WorkbenchPage::sendRawCommand(const QString &cmd) {
    if (!m_serialPort->isOpen()) {
        m_serialLog->append("[错误] 串口未连接，无法发送: " + cmd + "\n");
        return;
    }
    QByteArray payload = cmd.toUtf8() + "\r\n";
    m_serialPort->write(payload);
    m_serialLog->append("[发送] " + cmd + "\n");
}

void F407WorkbenchPage::sendBeep() {
    sendRawCommand("beep 30");
}

void F407WorkbenchPage::readSensors() {
    sendRawCommand("temp");
    sendRawCommand("light");
}

void F407WorkbenchPage::syncTime() {
    QDateTime now = QDateTime::currentDateTime();
    QString timeCmd = QString("time %1 %2 %3")
                          .arg(now.time().hour())
                          .arg(now.time().minute())
                          .arg(now.time().second());
    int week = now.date().dayOfWeek();
    QString dateCmd = QString("date %1 %2 %3 %4")
                          .arg(now.date().year() % 100)
                          .arg(now.date().month())
                          .arg(now.date().day())
                          .arg(week);
    sendRawCommand(timeCmd);
    sendRawCommand(dateCmd);
}

void F407WorkbenchPage::onDacApplyClicked() {
    if (m_dacSpinBox) {
        sendRawCommand(QString("dac %1").arg(m_dacSpinBox->value()));
    }
}

void F407WorkbenchPage::onBacklightChanged(int val) {
    if (m_blLabel) {
        m_blLabel->setText(QString("%1%").arg(val));
    }
    sendRawCommand(QString("bl %1").arg(val));
}

void F407WorkbenchPage::sendReboot() {
    sendRawCommand("reboot");
}

void F407WorkbenchPage::sendVisionJudgeResult(bool passed, const QString &details) {
    if (passed) {
        m_serialLog->append("[视觉判定] 合格 (OK) -> 触发单片机蜂鸣确认: " + details + "\n");
        sendRawCommand("beep 25");
    } else {
        m_serialLog->append("[视觉判定] 缺陷 (NG) -> 触发单片机报警长音: " + details + "\n");
        sendRawCommand("beep 300");
    }
}
