#pragma once

#include <QObject>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

class MainWindow;

// 旧版 Widgets 界面（知识实验室、节点管线、批量质检、F407 工作台……）的入口。
// 新界面迁移完成之前，这些功能通过「实验室」页面以独立窗口打开，第一次打开时才创建。
class LegacyLab : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool opened READ opened NOTIFY openedChanged)

public:
    explicit LegacyLab(QObject *parent = nullptr);
    ~LegacyLab() override;

    bool opened() const { return !m_window.isNull(); }

    Q_INVOKABLE void open();

signals:
    void openedChanged();

private:
    QPointer<MainWindow> m_window;
};
