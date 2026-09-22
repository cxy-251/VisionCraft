#pragma once

#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

class HomePage : public QWidget {
    Q_OBJECT
public:
    explicit HomePage(QWidget *parent = nullptr);
    ~HomePage() override = default;

    // 刷新工具卡片列表
    void reloadTools();

signals:
    void toolSelected(const QString &toolId);

private:
    QWidget* createHeader();
    QWidget* createToolCard(const QString &id, const QString &icon, 
                            const QString &title, const QString &desc);
    QWidget* createFooterInfo();

    QGridLayout *m_cardGrid = nullptr;
};
