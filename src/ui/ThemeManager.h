#pragma once

#include <QObject>
#include <QString>

class ThemeManager : public QObject {
    Q_OBJECT
public:
    static ThemeManager& instance();

    enum ThemeMode {
        System,     // 跟随系统 (默认)
        Dark,       // 强制深色模式
        Light       // 强制浅色模式
    };

    void setThemeMode(ThemeMode mode);
    ThemeMode themeMode() const { return m_mode; }
    bool isDarkMode() const;

    // 获取当前全套高对比度全局 QSS 样式表
    QString currentGlobalStyleSheet() const;

    // 获取当前模式下的卡片样式
    QString cardStyleSheet() const;

signals:
    void themeChanged(bool isDark);

private:
    explicit ThemeManager(QObject *parent = nullptr);
    bool detectWindowsDarkTheme() const;

    ThemeMode m_mode = ThemeMode::System;
};
