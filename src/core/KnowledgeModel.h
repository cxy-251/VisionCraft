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
    QStringList options;        // ComboBox 的候选列表（如枚举名称）
    QList<int> optionValues;    // ComboBox 候选对应的整数值
    QString tooltip;            // 悬停提示解释
};

// 知识点与方法元数据（数据驱动的核心单元）
struct KnowledgeTopic {
    QString id;                 // 唯一标识 (如 "cv_gaussian_blur")
    QString category;           // 所属主分类 (如 "OpenCV / 滤波与平滑")
    QString subCategory;        // 子分类 (如 "空间域滤波")
    QString name;               // 方法名 (如 "高斯滤波 (GaussianBlur)")
    QString tag;                // 特性简标 (如 "正态分布降噪")
    QString apiSignature;       // C++ 标准 API 函数签名
    QString docSummary;         // 核心算法原理与功能简述
    QString docParams;          // 核心参数深入剖析

    QList<ParamDescriptor> params; // 该方法需要的可调参数清单

    // 动态生成可复制的 C++ 调用代码
    std::function<QString(const QMap<QString, QVariant>& currentParams)> codeGenerator;

    // OpenCV 算法执行体
    std::function<void(const cv::Mat &src, cv::Mat &dst, 
                       const QMap<QString, QVariant>& params, 
                       QString &execNote)> cvRunner;

    // Qt 原生组件演示执行体（仅当该知识点为 Qt 专属时使用）
    std::function<QWidget*(QWidget *parent, const QMap<QString, QVariant>& params)> qtRunner = nullptr;
};
