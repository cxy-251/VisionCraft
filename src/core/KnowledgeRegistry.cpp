#include "KnowledgeRegistry.h"

KnowledgeRegistry& KnowledgeRegistry::instance() {
    static KnowledgeRegistry s_instance;
    return s_instance;
}

KnowledgeRegistry::KnowledgeRegistry() {
    registerOpenCVVisionTopics();
    registerOpenCVIndustrialTopics();
    registerQtCoreTopics();
    registerQtArchitectureTopics();
}

const QList<KnowledgeTopic>& KnowledgeRegistry::allTopics() const {
    return m_topics;
}

const KnowledgeTopic* KnowledgeRegistry::findTopic(const QString &id) const {
    auto it = m_topicMap.find(id);
    if (it != m_topicMap.end()) {
        return &it.value();
    }
    return nullptr;
}

QMap<QString, QList<KnowledgeTopic>> KnowledgeRegistry::topicsByCategory() const {
    QMap<QString, QList<KnowledgeTopic>> grouped;
    for (const auto &t : m_topics) {
        grouped[t.category].append(t);
    }
    return grouped;
}

void KnowledgeRegistry::registerTopic(const KnowledgeTopic &topic) {
    m_topics.append(topic);
    m_topicMap[topic.id] = topic;
}