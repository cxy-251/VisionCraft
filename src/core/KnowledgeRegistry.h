#pragma once

#include "KnowledgeModel.h"
#include <QList>
#include <QMap>

class KnowledgeRegistry {
public:
    static KnowledgeRegistry& instance();

    // 获取所有注册的知识点
    const QList<KnowledgeTopic>& allTopics() const;

    // 按 ID 查询
    const KnowledgeTopic* findTopic(const QString &id) const;

    // 按分类聚合获取
    QMap<QString, QList<KnowledgeTopic>> topicsByCategory() const;

    // 核心算子/机制注册接口
    void registerTopic(const KnowledgeTopic &topic);

private:
    KnowledgeRegistry();

    // 模块化子集构建器 (消灭巨石单体文件)
    void registerOpenCVVisionTopics();
    void registerOpenCVIndustrialTopics();
    void registerQtCoreTopics();
    void registerQtArchitectureTopics();

    QList<KnowledgeTopic> m_topics;
    QMap<QString, KnowledgeTopic> m_topicMap;
};
