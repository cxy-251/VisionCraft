// 动态属性；事件从哪来、经过谁、在哪被处理；事件过滤器
//
// 运行：QT_QPA_PLATFORM=offscreen ./example_qt_events     输出见 output.txt（不需要显示器）

#include <QApplication>
#include <QDynamicPropertyChangeEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <cstdio>

static void say(const QString &s) { std::printf("  %s\n", qPrintable(s)); std::fflush(stdout); }

// [region dynamic]
// 普通的 QObject 子类：没有声明任何 Q_PROPERTY，但可以随时挂上「动态属性」
class Probe : public QObject {
protected:
    bool event(QEvent *e) override
    {
        if (e->type() == QEvent::DynamicPropertyChange)
            say(QStringLiteral("收到 DynamicPropertyChange：%1")
                    .arg(QString::fromLatin1(static_cast<QDynamicPropertyChangeEvent *>(e)->propertyName())));
        return QObject::event(e);
    }
};
// [endregion]

// [region chain]
// 一个输入框：打印自己收到的按键事件，并说明是否「接受」
class Edit : public QLineEdit {
public:
    using QLineEdit::QLineEdit;
protected:
    void keyPressEvent(QKeyEvent *e) override
    {
        QLineEdit::keyPressEvent(e);
        say(QStringLiteral("输入框 keyPressEvent：%1，处理后 isAccepted = %2").arg(e->text().isEmpty() ? e->key() == Qt::Key_F5 ? "F5" : "?" : e->text())
                .arg(e->isAccepted() ? "true" : "false"));
    }
};
// 输入框的父窗口：只有子控件不要的事件，才会「冒泡」到这里
class Panel : public QWidget {
protected:
    void keyPressEvent(QKeyEvent *e) override { say(QStringLiteral("父窗口 keyPressEvent：收到冒泡上来的 %1").arg(e->key() == Qt::Key_F5 ? "F5" : e->text())); }
};
// 事件过滤器：装在输入框上，在输入框之前先看到它的所有事件
class Filter : public QObject {
public:
    bool blockDigits = false;
protected:
    bool eventFilter(QObject *watched, QEvent *e) override
    {
        if (e->type() == QEvent::KeyPress) {
            auto *k = static_cast<QKeyEvent *>(e);
            say(QStringLiteral("过滤器先看到 %1 的按键 %2").arg(watched->objectName(), k->key() == Qt::Key_F5 ? "F5" : k->text()));
            if (blockDigits && k->text().size() == 1 && k->text()[0].isDigit()) {
                say(QStringLiteral("过滤器拦下数字 %1（返回 true，输入框收不到）").arg(k->text()));
                return true;
            }
        }
        return false;   // false：继续交给目标对象
    }
};
// [endregion]

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    std::printf("==== 1. 动态属性 ====\n");
    // [region dynamic-use]
    Probe p;
    p.setProperty("station", "A3");                          // 没有声明过的名字：变成动态属性
    p.setProperty("retries", 2);
    say(QStringLiteral("读回：station=%1 retries=%2").arg(p.property("station").toString()).arg(p.property("retries").toInt()));
    say(QStringLiteral("dynamicPropertyNames：%1").arg(QString::fromLatin1(p.dynamicPropertyNames().join(", "))));
    p.setProperty("retries", QVariant());                   // 设成无效的 QVariant：删除这个动态属性
    say(QStringLiteral("删除后 dynamicPropertyNames：%1").arg(QString::fromLatin1(p.dynamicPropertyNames().join(", "))));
    // [endregion]

    std::printf("\n==== 2. 按键事件的旅程 ====\n");
    // [region send]
    Panel panel;
    auto *edit = new Edit(&panel);
    edit->setObjectName("edit");
    (new QVBoxLayout(&panel))->addWidget(edit);
    panel.show();
    edit->setFocus();
    Filter filter;
    edit->installEventFilter(&filter);

    auto press = [&](int key, const QString &text) {
        QKeyEvent e(QEvent::KeyPress, key, Qt::NoModifier, text);
        QApplication::sendEvent(edit, &e);   // 同步：现在就派发，返回时已处理完。没被接受的按键，Qt 自动往父窗口传
    };
    press(Qt::Key_A, "a");                                   // 输入框会用掉普通字符
    press(Qt::Key_F5, "");                                   // 输入框不处理 F5
    filter.blockDigits = true;
    press(Qt::Key_7, "7");
    say(QStringLiteral("输入框现在的内容：「%1」").arg(edit->text()));
    // [endregion]

    std::printf("\n==== 3. sendEvent 与 postEvent ====\n");
    // [region post]
    filter.blockDigits = false;
    QApplication::postEvent(edit, new QKeyEvent(QEvent::KeyPress, Qt::Key_B, Qt::NoModifier, "b"));   // 排进队列，堆上分配，Qt 负责删
    say(QStringLiteral("postEvent 刚返回，内容：「%1」").arg(edit->text()));
    QCoreApplication::processEvents();
    say(QStringLiteral("processEvents 之后，内容：「%1」").arg(edit->text()));
    // [endregion]
    return 0;
}
