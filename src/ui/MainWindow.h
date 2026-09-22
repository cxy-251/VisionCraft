#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include "HomePage.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

public slots:
    void navigateToHome();
    void navigateToTool(const QString &toolId);

private:
    void setupUI();
    void setupGlobalStyle();

    QStackedWidget *m_stackWidget = nullptr;
    HomePage *m_homePage = nullptr;

    QWidget *m_topBar = nullptr;
    QPushButton *m_backHomeBtn = nullptr;
    QLabel *m_breadcrumbLabel = nullptr;
};
