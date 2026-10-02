#include "ConcurrencyDemo.h"

#include "Inspector.h"
#include "PartGenerator.h"

#include <QThreadPool>
#include <QtConcurrent/QtConcurrentMap>

ConcurrencyDemo::ConcurrencyDemo(QObject *parent)
    : QObject(parent)
{
    // [region watcher]
    // 先连接、再 setFuture：顺序反过来，任务如果瞬间做完，finished 信号可能在连接之前就发了
    connect(&m_watcher, &QFutureWatcher<int>::finished, this, [this] {
        m_poolMs = m_clock.nsecsElapsed() / 1e6;
        int ng = 0;
        for (int defect : m_watcher.future().results())
            ng += defect != VC_DEFECT_NONE;
        m_lastRun = tr("线程池：%1 张，%2 ms，判不合格 %3 张").arg(m_images.size()).arg(m_poolMs, 0, 'f', 0).arg(ng);
        emit changed();
    });
    // [endregion]
}

int ConcurrencyDemo::threads() const
{
    return QThreadPool::globalInstance()->maxThreadCount();
}

void ConcurrencyDemo::prepare()
{
    if (int(m_images.size()) == m_count)
        return;
    PartGenerator gen(99);
    m_images.clear();
    for (int i = 0; i < m_count; ++i)
        m_images.push_back(gen.next(0.3).image);
}

void ConcurrencyDemo::runBlocking()
{
    if (running())
        return;
    prepare();
    m_clock.start();
    // [region blocking]
    Inspector inspector;
    int ng = 0;
    for (const cv::Mat &img : m_images)        // 在界面线程里一张一张做：这段时间界面完全不响应
        ng += !inspector.inspect(img).ok;
    // [endregion]
    m_blockingMs = m_clock.nsecsElapsed() / 1e6;
    m_lastRun = tr("界面线程：%1 张，%2 ms，判不合格 %3 张").arg(m_images.size()).arg(m_blockingMs, 0, 'f', 0).arg(ng);
    emit changed();
}

void ConcurrencyDemo::runPool()
{
    if (running())
        return;
    prepare();
    m_clock.start();
    // [region pool]
    // mapped：对容器里的每个元素调用函数，分给线程池里的多个线程同时做，立即返回一个 QFuture
    m_watcher.setFuture(QtConcurrent::mapped(m_images, [](const cv::Mat &img) {
        static thread_local Inspector inspector;   // 每个线程一个，避免多个线程共用同一个对象
        return int(inspector.inspect(img).defect);
    }));
    // [endregion]
    emit changed();
}
