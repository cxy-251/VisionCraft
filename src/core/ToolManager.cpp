#include "ToolManager.h"

ToolManager& ToolManager::instance() {
    static ToolManager s_instance;
    return s_instance;
}

ToolManager::ToolManager(QObject *parent) : QObject(parent) {}

void ToolManager::registerTool(IToolPage *tool) {
    if (!tool) return;
    QString toolId = tool->id();
    if (!m_toolMap.contains(toolId)) {
        m_tools.append(tool);
        m_toolMap.insert(toolId, tool);
        emit toolRegistered(tool);
    }
}

const QList<IToolPage*>& ToolManager::tools() const {
    return m_tools;
}

IToolPage* ToolManager::findTool(const QString &id) const {
    return m_toolMap.value(id, nullptr);
}
