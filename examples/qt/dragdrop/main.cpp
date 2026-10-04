// 拖放：MIME 数据、接收方的判断逻辑、列表控件自带的拖放
//
// 运行：QT_QPA_PLATFORM=offscreen ./example_qt_dragdrop               自动部分，输出见 output.txt
//       ./example_qt_dragdrop interactive                            弹出窗口，把文件或文字拖进来，打印收到的内容

#include <QApplication>
#include <QDataStream>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QLabel>
#include <QListWidget>
#include <QMimeData>
#include <QVBoxLayout>
#include <cstdio>

// [region drop-target]
// 只接收图片文件的区域
class ImageDropArea : public QLabel {
public:
    ImageDropArea() : QLabel("把图片拖到这里")
    {
        setAcceptDrops(true);                                 // 不设置的话，拖到上面不会有任何事件
        setAlignment(Qt::AlignCenter);
        setMinimumSize(360, 160);
    }
    QStringList received;
protected:
    void dragEnterEvent(QDragEnterEvent *e) override
    {
        if (imagePaths(e->mimeData()).isEmpty())
            return;                                           // 不接受：光标显示「禁止」，松手也不会触发 dropEvent
        e->acceptProposedAction();
    }
    void dropEvent(QDropEvent *e) override
    {
        received = imagePaths(e->mimeData());
        e->acceptProposedAction();
    }
private:
    static QStringList imagePaths(const QMimeData *m)
    {
        QStringList out;
        if (!m->hasUrls())
            return out;
        for (const QUrl &u : m->urls())
            if (u.isLocalFile() && QStringList{"png", "jpg", "bmp"}.contains(QFileInfo(u.toLocalFile()).suffix().toLower()))
                out << u.toLocalFile();
        return out;
    }
};
// [endregion]

static void dump(const char *title, const QMimeData *m)
{
    std::printf("  %s\n", title);
    for (const QString &f : m->formats())
        std::printf("    %-45s %lld 字节\n", qPrintable(f), qlonglong(m->data(f).size()));
}

// 交互模式：把收到的拖放事件原样打印出来
class Logger : public QLabel {
public:
    Logger() : QLabel("把文件、文字、图片拖到这个窗口里\n（关掉窗口结束）") { setAcceptDrops(true); setAlignment(Qt::AlignCenter); resize(480, 240); }
protected:
    void dragEnterEvent(QDragEnterEvent *e) override
    {
        std::printf("dragEnter：建议动作 %d，可选动作 0x%x\n", int(e->proposedAction()), int(e->possibleActions()));
        e->acceptProposedAction();
    }
    void dropEvent(QDropEvent *e) override
    {
        std::printf("drop：位置 (%.0f,%.0f)\n", e->position().x(), e->position().y());
        dump("MIME 数据：", e->mimeData());
        for (const QUrl &u : e->mimeData()->urls())
            std::printf("    url: %s\n", qPrintable(u.toString()));
        if (e->mimeData()->hasText())
            std::printf("    text: %s\n", qPrintable(e->mimeData()->text().left(200)));
        std::fflush(stdout);
        e->acceptProposedAction();
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    if (argc > 1 && QByteArray(argv[1]) == "interactive") {
        Logger l;
        l.show();
        return app.exec();
    }

    std::printf("==== 1. 拖动携带的数据：QMimeData ====\n");
    {
        // [region mime]
        QMimeData m;
        m.setText("A3-0042");                                 // 纯文本
        m.setUrls({QUrl::fromLocalFile("/data/parts/A3-0042.png")});   // 文件：文件管理器拖出来的就是这种
        QByteArray part;
        {
            QDataStream out(&part, QIODevice::WriteOnly);
            out << QString("A3-0042") << 18 << true;           // 编号、阈值、是否合格
        }
        m.setData("application/x-visioncraft-part", part);     // 自己定义的类型：只有自己的程序认识
        // [endregion]
        dump("同一次拖动，带了三种格式：", &m);
        std::printf("  hasText=%d hasUrls=%d hasImage=%d\n", m.hasText(), m.hasUrls(), m.hasImage());
    }

    std::printf("\n==== 2. 接收方的判断 ====\n");
    {
        ImageDropArea area;
        area.show();
        // [region simulate]
        auto tryDrop = [&](const char *what, QMimeData *m) {
            QDragEnterEvent enter(QPoint(10, 10), Qt::CopyAction, m, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(&area, &enter);
            area.received.clear();
            if (enter.isAccepted()) {                         // 真实拖放里，进入时没被接受就不会有 drop
                QDropEvent drop(QPointF(10, 10), Qt::CopyAction, m, Qt::LeftButton, Qt::NoModifier);
                QApplication::sendEvent(&area, &drop);
            }
            std::printf("  进入时%s，收到 %lld 个文件 %-12s ← %s\n", enter.isAccepted() ? "接受" : "拒绝",
                        qlonglong(area.received.size()), qPrintable(area.received.join(", ")), what);
        };
        // [endregion]
        QMimeData png, txt, mixed, text;
        png.setUrls({QUrl::fromLocalFile("/data/a.png")});
        txt.setUrls({QUrl::fromLocalFile("/data/readme.txt")});
        mixed.setUrls({QUrl::fromLocalFile("/data/b.JPG"), QUrl::fromLocalFile("/data/c.txt"), QUrl("https://example.com/d.png")});
        text.setText("/data/a.png");
        tryDrop("一张 png", &png);
        tryDrop("一个 txt", &txt);
        tryDrop("JPG + txt + 网址上的 png", &mixed);
        tryDrop("只是一段文字（内容像路径）", &text);
    }

    std::printf("\n==== 3. 列表控件自带的拖放 ====\n");
    {
        // [region list]
        QListWidget source, target;
        source.addItems({"工件 A3", "工件 B7", "工件 C1"});
        source.setDragEnabled(true);                          // 能拖出
        target.setAcceptDrops(true);                          // 能放入
        target.setDragDropMode(QAbstractItemView::DropOnly);
        source.setDragDropMode(QAbstractItemView::DragOnly);
        // [endregion]
        source.item(1)->setSelected(true);
        // [region list-mime]
        QMimeData *m = source.model()->mimeData(source.selectionModel()->selectedIndexes());
        // [endregion]
        dump("拖动「工件 B7」时，列表生成的数据：", m);
        const bool ok = target.model()->dropMimeData(m, Qt::CopyAction, 0, 0, QModelIndex());
        std::printf("  目标列表 dropMimeData 返回 %s，现在有 %d 项：%s\n", ok ? "true" : "false", target.count(),
                    target.count() ? qPrintable(target.item(0)->text()) : "");
        delete m;
    }
    return 0;
}
