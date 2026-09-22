#include "DevToolboxPage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QGuiApplication>
#include <QScreen>
#include <QSysInfo>
#include <nlohmann/json.hpp>
#include <fmt/core.h>
#include <fmt/format.h>
#include <opencv2/core/version.hpp>

using json = nlohmann::json;

DevToolboxPage::DevToolboxPage(QWidget *parent) : IToolPage(parent) {
    setupUI();
}

void DevToolboxPage::setupUI() {
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(25, 20, 25, 20);
    mainLayout->setSpacing(20);

    // 左侧：JSON 校验与美化器
    auto *leftPanel = new QFrame(this);
    leftPanel->setStyleSheet("QFrame { background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 10px; }");
    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(18, 18, 18, 18);
    leftLayout->setSpacing(12);

    auto *leftTitle = new QLabel("📋 JSON 语法高亮校验与格式化 (nlohmann-json)", leftPanel);
    leftTitle->setStyleSheet("font-size: 15px; font-weight: 700; color: #0f172a; border: none;");
    leftLayout->addWidget(leftTitle);

    m_inputJsonEdit = new QTextEdit(leftPanel);
    m_inputJsonEdit->setPlaceholderText("在此输入或粘贴需要解析/格式化的原始 JSON 文本...");
    m_inputJsonEdit->setPlainText("{\"project\":\"VisionCraft\",\"version\":\"1.0.0\",\"enabled\":true,\"features\":[\"ScreenMatcher\",\"FilterLab\",\"Toolbox\"],\"runtime\":{\"compiler\":\"MSVC\",\"gui\":\"Qt6\"}}");
    m_inputJsonEdit->setStyleSheet("font-family: 'Consolas', 'Courier New', monospace; font-size: 13px; border: 1px solid #cbd5e1; border-radius: 6px;");
    leftLayout->addWidget(m_inputJsonEdit, 1);

    auto *btnRow = new QHBoxLayout();
    auto *formatBtn = new QPushButton("✨ 美化缩进 (4格)", leftPanel);
    formatBtn->setStyleSheet("QPushButton { background-color: #2563eb; color: white; border: none; border-radius: 6px; padding: 8px 14px; font-weight: 600; }"
                             "QPushButton:hover { background-color: #1d4ed8; }");
    connect(formatBtn, &QPushButton::clicked, this, &DevToolboxPage::formatJSON);

    auto *compressBtn = new QPushButton("📦 单行压缩", leftPanel);
    compressBtn->setStyleSheet("QPushButton { background-color: #0d9488; color: white; border: none; border-radius: 6px; padding: 8px 14px; font-weight: 600; }"
                              "QPushButton:hover { background-color: #0f766e; }");
    connect(compressBtn, &QPushButton::clicked, this, &DevToolboxPage::compressJSON);

    btnRow->addWidget(formatBtn);
    btnRow->addWidget(compressBtn);
    btnRow->addStretch();
    leftLayout->addLayout(btnRow);

    m_outputJsonEdit = new QTextEdit(leftPanel);
    m_outputJsonEdit->setReadOnly(true);
    m_outputJsonEdit->setPlaceholderText("解析输出结果将在此展示...");
    m_outputJsonEdit->setStyleSheet("font-family: 'Consolas', 'Courier New', monospace; font-size: 13px; background-color: #f8fafc; border: 1px solid #cbd5e1; border-radius: 6px; color: #0f172a;");
    leftLayout->addWidget(m_outputJsonEdit, 1);

    m_jsonStatusLabel = new QLabel("就绪", leftPanel);
    m_jsonStatusLabel->setStyleSheet("font-size: 12px; color: #64748b; border: none;");
    leftLayout->addWidget(m_jsonStatusLabel);

    mainLayout->addWidget(leftPanel, 3);

    // 右侧：系统信息诊断面板
    auto *rightPanel = new QFrame(this);
    rightPanel->setFixedWidth(300);
    rightPanel->setStyleSheet("QFrame { background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 10px; }");
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(18, 18, 18, 18);
    rightLayout->setSpacing(14);

    auto *rightTitle = new QLabel("🖥️ 系统与硬件状态", rightPanel);
    rightTitle->setStyleSheet("font-size: 15px; font-weight: 700; color: #0f172a; border: none;");
    rightLayout->addWidget(rightTitle);

    m_sysInfoLabel = new QLabel(rightPanel);
    m_sysInfoLabel->setWordWrap(true);
    m_sysInfoLabel->setStyleSheet("font-size: 13px; color: #334155; line-height: 1.6; border: none;");
    rightLayout->addWidget(m_sysInfoLabel);

    rightLayout->addStretch();

    auto *refreshBtn = new QPushButton("🔄 刷新系统指标", rightPanel);
    refreshBtn->setStyleSheet("QPushButton { background-color: #f1f5f9; border: 1px solid #cbd5e1; border-radius: 6px; padding: 8px; font-size: 13px; font-weight: 500; }"
                             "QPushButton:hover { background-color: #e2e8f0; }");
    connect(refreshBtn, &QPushButton::clicked, this, &DevToolboxPage::refreshSystemInfo);
    rightLayout->addWidget(refreshBtn);

    mainLayout->addWidget(rightPanel, 2);

    refreshSystemInfo();
    formatJSON();
}

void DevToolboxPage::onActivated() {
    refreshSystemInfo();
}

void DevToolboxPage::formatJSON() {
    QString raw = m_inputJsonEdit->toPlainText();
    try {
        json parsed = json::parse(raw.toStdString());
        std::string formatted = parsed.dump(4);
        m_outputJsonEdit->setPlainText(QString::fromStdString(formatted));
        m_jsonStatusLabel->setText("✅ JSON 语法验证通过，格式化成功！");
        m_jsonStatusLabel->setStyleSheet("font-size: 12px; color: #16a34a; font-weight: 600; border: none;");
    } catch (const std::exception &e) {
        m_jsonStatusLabel->setText(QString("❌ 语法错误: %1").arg(e.what()));
        m_jsonStatusLabel->setStyleSheet("font-size: 12px; color: #dc2626; font-weight: 600; border: none;");
    }
}

void DevToolboxPage::compressJSON() {
    QString raw = m_inputJsonEdit->toPlainText();
    try {
        json parsed = json::parse(raw.toStdString());
        std::string minified = parsed.dump();
        m_outputJsonEdit->setPlainText(QString::fromStdString(minified));
        m_jsonStatusLabel->setText("✅ 单行压缩成功！");
        m_jsonStatusLabel->setStyleSheet("font-size: 12px; color: #16a34a; font-weight: 600; border: none;");
    } catch (const std::exception &e) {
        m_jsonStatusLabel->setText(QString("❌ 语法错误: %1").arg(e.what()));
        m_jsonStatusLabel->setStyleSheet("font-size: 12px; color: #dc2626; font-weight: 600; border: none;");
    }
}

void DevToolboxPage::refreshSystemInfo() {
    QScreen *screen = QGuiApplication::primaryScreen();
    QSize screenSize = screen ? screen->size() : QSize(0, 0);
    qreal dpr = screen ? screen->devicePixelRatio() : 1.0;

    std::string infoStr = fmt::format(
        "<b>操作系统:</b> {}<br>"
        "<b>CPU 架构:</b> {}<br>"
        "<b>主机名称:</b> {}<br>"
        "<b>主屏分辨率:</b> {} x {} (DPI 缩放: {:.1f}x)<br>"
        "<b>Qt 版本:</b> {}<br>"
        "<b>OpenCV 引擎:</b> {}<br>"
        "<b>C++ 标准:</b> C++17 MSVC<br>",
        QSysInfo::prettyProductName().toStdString(),
        QSysInfo::currentCpuArchitecture().toStdString(),
        QSysInfo::machineHostName().toStdString(),
        screenSize.width(), screenSize.height(), dpr,
        QT_VERSION_STR,
        CV_VERSION
    );

    m_sysInfoLabel->setText(QString::fromStdString(infoStr));
}
