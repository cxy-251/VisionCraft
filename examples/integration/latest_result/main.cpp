#include "core/ScreenCapture.h"
#include "vision/ImageOperations.h"
#include <QCoreApplication>
#include <QFutureWatcher>
#include <QThread>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrentRun>
#include <functional>
#include <iostream>
struct Outcome {
    int revision=0;
    double threshold=0;
    bool workerOnGui=false;
    QImage image;
    QString error;
};
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    QThreadPool pool;pool.setMaxThreadCount(1);
    QFutureWatcher<Outcome> watcher;
    int revision=1;
    double threshold=128;
    QImage source(4,1,QImage::Format_Grayscale8);
    source.scanLine(0)[0]=40;source.scanLine(0)[1]=120;
    source.scanLine(0)[2]=180;source.scanLine(0)[3]=220;
    std::function<void()> submit;
    // [region snapshot]
    submit=[&] {
        const auto input=source; // QImage按值快照，之后修改source将触发写时分离。
        const auto requestedRevision=revision;
        const auto requestedThreshold=threshold;
        const auto guiThread=app.thread();
        std::cout << "submit revision=" << requestedRevision << " threshold=" << requestedThreshold << '\n';
        watcher.setFuture(QtConcurrent::run(&pool,[input,requestedRevision,requestedThreshold,guiThread] {
            Outcome out;out.revision=requestedRevision;out.threshold=requestedThreshold;
            out.workerOnGui=QThread::currentThread()==guiThread;
            try {
                ImageOperations::Params params;params.threshold=requestedThreshold;
                const auto result=ImageOperations::run(ScreenCapture::qImageToMat(input),ImageOperations::Operation::BinaryThreshold,params);
                out.error=result.error;
                out.image=ScreenCapture::matToQImage(result.image);
            } catch(const std::exception &e) {out.error=QString::fromUtf8(e.what());}
            return out;
        }));
    };
    // [endregion]
    // [region publish]
    QObject::connect(&watcher,&QFutureWatcher<Outcome>::finished,&app,[&] {
        const auto out=watcher.result();
        if(!out.error.isEmpty() || out.image.isNull()) {
            std::cerr << "processing failed: " << out.error.toStdString() << '\n';app.exit(1);return;
        }
        std::cout << "finished revision=" << out.revision << " threshold=" << out.threshold
                  << " workerOnGui=" << out.workerOnGui
                  << " publishOnGui=" << (QThread::currentThread()==app.thread()) << " pixels=";
        for(int x=0;x<out.image.width();++x)std::cout << out.image.pixelColor(x,0).red() << (x+1==out.image.width()?"\n":",");
        if(out.revision!=revision) {
            std::cout << "discard stale result; current revision=" << revision << '\n';
            submit(); // 只在前一个任务完成后提交最新状态，不同时堆积任务。
            return;
        }
        std::cout << "publish current result\n";app.quit();
    });
    // [endregion]
    // [region change]
    submit();
    // 主事件循环还没开始接收finished；不靠sleep或猜测线程谁先完成。
    threshold=200;source.fill(240);++revision;
    std::cout << "GUI changes source and threshold; revision=" << revision << '\n';
    const int exitCode=app.exec();
    pool.waitForDone();return exitCode;
    // [endregion]
}
