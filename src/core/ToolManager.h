#pragma once

#include <QObject>
#include <QList>
#include <QMap>
#include "IToolPage.h"

/**
 * @brief 工具管理器（注册中心单例）
 * 集中管理所有注册的子工具，提供按 ID 查找与列表枚举能力。
 */
class ToolManager : public QObject {
    Q_OBJECT
public:
    static ToolManager& instance();

    // 注册一个新工具
    void registerTool(IToolPage *tool);

    // 获取所有已注册工具
    const QList<IToolPage*>& tools() const;

    // 根据 ID 获取工具
    IToolPage* findTool(const QString &id) const;

signals:
    void toolRegistered(IToolPage *tool);

private:
    explicit ToolManager(QObject *parent = nullptr);
    ~ToolManager() override = default;
    ToolManager(const ToolManager&) = delete;
    ToolManager& operator=(const ToolManager&) = delete;

    QList<IToolPage*> m_tools;
    QMap<QString, IToolPage*> m_toolMap;
};
