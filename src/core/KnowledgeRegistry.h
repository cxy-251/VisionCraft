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

private:
    KnowledgeRegistry();
    void initOpenCVTopics();
    void initQtTopics();

    QList<KnowledgeTopic> m_topics;
    QMap<QString, KnowledgeTopic> m_topicMap;
};
