#pragma once

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QList>
#include <QMap>
#include <functional>
#include <opencv2/core.hpp>

class QWidget;

// 参数控件类型
enum class ParamType {
    SliderInt,      // 整数滑块
    SliderDouble,   // 浮点数滑块
    ComboBox,       // 下拉选择框
    CheckBox        // 复选框开关
};

// 单个参数的描述元数据
struct ParamDescriptor {
    QString key;                // 参数变量名（如 "ksize", "sigmaX"）
    QString label;              // 界面显示标签（如 "核大小 (ksize)"）
    ParamType type = ParamType::SliderInt;
    double minVal = 0.0;
    double maxVal = 100.0;
    double step = 1.0;
    double defaultVal = 0.0;
    QStringList options;        // ComboBox 的候选列表
    QList<int> optionValues;    // ComboBox 候选对应的整数值
    QString tooltip;            // 悬停提示解释
};

// 知识点与方法元数据（数据驱动的核心单元）
struct KnowledgeTopic {
    QString id;                 // 唯一标识 (如 "cv_gaussian_blur")
    QString framework;          // "OpenCV" 或 "Qt"
    QString category;           // 所属主分类 (如 "OpenCV / 图像滤波")
    QString name;               // 方法/机制名称 (如 "高斯滤波 (GaussianBlur)")
    QString tag;                // 特性标签 (如 "正态分布降噪")
    QString apiSignature;       // C++ 标准 API 函数声明或核心语法
    
    // 是否为图像实时交互类（true: 双屏视窗+滑块实时演算；false: 无法/无需图像可视化的核心机制，以架构原理、时机与工程代码深度剖析为主）
    bool isVisualInteractive = true;

    QString docSummary;         // 核心算法/机制原理简述
    QString docParams;          // 核心参数/关键要素剖析
    QString usageTiming;        // 什么时候用？最佳应用场景剖析
    QString bestPractices;      // 工程避坑指南与性能注意事项
    QString codeSnippet;        // 完整工程级 C++ 代码示例

    // 参数交互与动态代码生成（当 isVisualInteractive == true 时）
    QList<ParamDescriptor> params;
    std::function<QString(const QMap<QString, QVariant>& currentParams)> codeGenerator;
    std::function<void(const cv::Mat &src, cv::Mat &dst, 
                       const QMap<QString, QVariant>& params, 
                       QString &execNote)> cvRunner;
};
