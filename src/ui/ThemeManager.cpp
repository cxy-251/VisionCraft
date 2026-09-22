#include "ThemeManager.h"
#include <QGuiApplication>
#include <QStyleHints>
#include <QSettings>

ThemeManager& ThemeManager::instance() {
    static ThemeManager s_instance;
    return s_instance;
}

ThemeManager::ThemeManager(QObject *parent) : QObject(parent) {}

void ThemeManager::setThemeMode(ThemeMode mode) {
    if (m_mode != mode) {
        m_mode = mode;
        emit themeChanged(isDarkMode());
    }
}

bool ThemeManager::isDarkMode() const {
    if (m_mode == ThemeMode::Dark) return true;
    if (m_mode == ThemeMode::Light) return false;
    return detectWindowsDarkTheme();
}

bool ThemeManager::detectWindowsDarkTheme() const {
    // 1. 优先使用 Qt 6 官方 cross-platform styleHints
    if (QGuiApplication::styleHints()) {
        if (QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark) {
            return true;
        }
    }

    // 2. Windows 注册表兜底检测
#ifdef Q_OS_WIN
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", QSettings::NativeFormat);
    if (settings.contains("AppsUseLightTheme")) {
        return settings.value("AppsUseLightTheme").toInt() == 0;
    }
#endif

    return false;
}

QString ThemeManager::cardStyleSheet() const {
    if (isDarkMode()) {
        return 
            "#ToolCard, #PanelCard {"
            "   background-color: #1e293b;"
            "   border: 1px solid #334155;"
            "   border-radius: 12px;"
            "}"
            "#ToolCard:hover {"
            "   background-color: #24344d;"
            "   border: 1px solid #38bdf8;"
            "}";
    } else {
        return 
            "#ToolCard, #PanelCard {"
            "   background-color: #ffffff;"
            "   border: 1px solid #e2e8f0;"
            "   border-radius: 12px;"
            "}"
            "#ToolCard:hover {"
            "   background-color: #f8fafc;"
            "   border: 1px solid #3b82f6;"
            "}";
    }
}

QString ThemeManager::currentGlobalStyleSheet() const {
    if (isDarkMode()) {
        // 高对比度深色模式（绝无背景相近、看不清的问题！）
        return R"(
            QMainWindow { background-color: #0f172a; }
            QWidget { 
                font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', 'Microsoft YaHei', sans-serif;
                color: #f8fafc;
            }
            QLabel { color: #f8fafc; }
            QScrollArea { background: transparent; border: none; }
            QTreeWidget { 
                background-color: #1e293b; 
                border: 1px solid #334155; 
                border-radius: 8px; 
                color: #f8fafc; 
                font-size: 13px; 
            }
            QTreeWidget::item { padding: 8px 6px; border-radius: 4px; }
            QTreeWidget::item:hover { background-color: #334155; }
            QTreeWidget::item:selected { background-color: #2563eb; color: #ffffff; font-weight: bold; }
            
            QLineEdit, QTextEdit { 
                background-color: #1e293b; 
                color: #f8fafc; 
                border: 1px solid #475569; 
                border-radius: 6px; 
                padding: 6px; 
            }
            QLineEdit:focus, QTextEdit:focus { border: 1px solid #38bdf8; }
            
            QComboBox { 
                background-color: #1e293b; 
                color: #f8fafc; 
                border: 1px solid #475569; 
                border-radius: 6px; 
                padding: 6px; 
            }
            QComboBox QAbstractItemView { 
                background-color: #1e293b; 
                color: #f8fafc; 
                selection-background-color: #2563eb; 
            }
            
            QCheckBox { color: #f8fafc; font-size: 13px; }
            
            QPushButton { 
                background-color: #2563eb; 
                color: #ffffff; 
                border: none; 
                border-radius: 6px; 
                padding: 8px 16px; 
                font-size: 13px; 
                font-weight: 600; 
            }
            QPushButton:hover { background-color: #3b82f6; }
            QPushButton:pressed { background-color: #1d4ed8; }
        )";
    } else {
        // 高对比度浅色模式
        return R"(
            QMainWindow { background-color: #f8fafc; }
            QWidget { 
                font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', 'Microsoft YaHei', sans-serif;
                color: #0f172a;
            }
            QLabel { color: #0f172a; }
            QScrollArea { background: transparent; border: none; }
            QTreeWidget { 
                background-color: #ffffff; 
                border: 1px solid #cbd5e1; 
                border-radius: 8px; 
                color: #0f172a; 
                font-size: 13px; 
            }
            QTreeWidget::item { padding: 8px 6px; border-radius: 4px; }
            QTreeWidget::item:hover { background-color: #f1f5f9; }
            QTreeWidget::item:selected { background-color: #dbeafe; color: #1e40af; font-weight: bold; }
            
            QLineEdit, QTextEdit { 
                background-color: #ffffff; 
                color: #0f172a; 
                border: 1px solid #cbd5e1; 
                border-radius: 6px; 
                padding: 6px; 
            }
            QLineEdit:focus, QTextEdit:focus { border: 1px solid #2563eb; }
            
            QComboBox { 
                background-color: #ffffff; 
                color: #0f172a; 
                border: 1px solid #cbd5e1; 
                border-radius: 6px; 
                padding: 6px; 
            }
            QComboBox QAbstractItemView { 
                background-color: #ffffff; 
                color: #0f172a; 
                selection-background-color: #dbeafe; 
                selection-color: #1e40af; 
            }
            
            QCheckBox { color: #0f172a; font-size: 13px; }
            
            QPushButton { 
                background-color: #2563eb; 
                color: #ffffff; 
                border: none; 
                border-radius: 6px; 
                padding: 8px 16px; 
                font-size: 13px; 
                font-weight: 600; 
            }
            QPushButton:hover { background-color: #1d4ed8; }
            QPushButton:pressed { background-color: #1e40af; }
        )";
    }
}
