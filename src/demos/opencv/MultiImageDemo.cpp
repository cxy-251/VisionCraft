#include "MultiImageDemo.h"
#include "core/ScreenCapture.h"
#include <QtConcurrent>
MultiImageDemo::MultiImageDemo(QObject *parent) : QObject(parent) {
    connect(&m_watcher,&QFutureWatcher<teaching::MultiImageResult>::finished,this,[this] {
        m_busy=false;
        if(m_request!=m_revision) {
            m_status=tr("参数已改变或计算已停止，未采用旧任务结果；请重新计算。"); emit changed(); return;
        }
        const auto result=m_watcher.result();
        if(!result.error.empty()) {
            m_status=tr("本次计算未完成：%1。此前结果保留。").arg(QString::fromStdString(result.error));
        } else {
            m_images.clear(); m_names.clear();
            for(const auto &view:result.images) {
                m_names.append(QString::fromStdString(view.first));
                m_images.append(ScreenCapture::matToQImage(view.second));
            }
            m_index=m_images.size()-1;
            m_summary=QString::fromStdString(result.summary);
            m_status=tr("结果对应本次提交的参数；下方可查看输入、掩膜、权重和结果。");
        }
        emit changed();
    });
}
MultiImageDemo::~MultiImageDemo() { if(m_cancel) m_cancel->store(true); }
void MultiImageDemo::configure(int preset,int method,int levels,bool estimate) {
    if(m_input.preset==preset && m_input.method==method && m_input.levels==levels && m_input.estimateAlignment==estimate) return;
    m_input={preset,method,levels,estimate}; ++m_revision;
    if(m_cancel) m_cancel->store(true);
    m_status=tr("参数已修改，显示的仍是此前结果。点击计算提交新参数。"); emit changed();
}
// [region snapshot]
void MultiImageDemo::run() {
    if(running()) return;
    m_busy=true;
    m_request=m_revision;
    const auto snapshot=m_input;
    m_cancel=std::make_shared<std::atomic_bool>(false);
    const auto cancel=m_cancel;
    m_status=tr("计算中；更改参数会使本次结果失效。");
    m_watcher.setFuture(QtConcurrent::run([snapshot,cancel] {
        return teaching::composeLesson(snapshot,cancel); // worker owns values, never this QObject
    }));
    emit changed();
}
// [endregion]
void MultiImageDemo::selectView(int index) { if(index>=0 && index<m_images.size()) { m_index=index; emit changed(); } }
void MultiImageDemo::cancel() {
    ++m_revision; if(m_cancel) m_cancel->store(true);
    m_status=tr("已停止接收结果；正在执行的算子会在步骤边界结束。"); emit changed();
}
