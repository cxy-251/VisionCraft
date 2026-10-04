// 国际化：tr() 标记、.ts/.qm 翻译文件、运行时切换语言、复数、本地化格式
//
// 运行：QT_QPA_PLATFORM=offscreen ./example_qt_i18n     输出见 output.txt
// 翻译文件的生成：lupdate main.cpp -ts i18n_en.ts（提取）→ 填写译文 → lrelease（CMake 自动做）

#include <QApplication>
#include <QDate>
#include <QEvent>
#include <QLabel>
#include <QLocale>
#include <QTranslator>
#include <QVBoxLayout>
#include <cstdio>

// [region static]
// 全局变量在 main() 之前初始化：那时还没有加载任何翻译
static const QString kTitleTooEarly = QObject::tr("视觉检测工位");
// QT_TRANSLATE_NOOP 只做标记（让 lupdate 提取），不翻译；真正用的时候再 tr()
static const char *const kTitleMarked = QT_TRANSLATE_NOOP("QObject", "视觉检测工位");
// [endregion]

// [region panel]
class StatusPanel : public QWidget {
    Q_OBJECT
public:
    StatusPanel()
    {
        auto *col = new QVBoxLayout(this);
        col->addWidget(m_fixed = new QLabel(tr("检测结果")));   // 只在构造时设置一次
        col->addWidget(m_live = new QLabel);
        retranslate();
    }
    QLabel *m_fixed, *m_live;
    int parts = 5;
protected:
    void changeEvent(QEvent *e) override
    {
        if (e->type() == QEvent::LanguageChange)              // 安装或移除翻译器时，每个窗口都会收到
            retranslate();
        QWidget::changeEvent(e);
    }
private:
    void retranslate()
    {
        m_live->setText(tr("已检测 %n 件", nullptr, parts));   // %n：复数形式由译文决定
    }
};
// [endregion]

class Messages : public QObject {
    Q_OBJECT
public:
    static QString ok() { return tr("合格"); }
    static QString okInOtherContext() { return QCoreApplication::translate("Report", "合格"); }   // 另一个上下文
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    StatusPanel panel;
    panel.show();

    std::printf("==== 1. 没有翻译器 ====\n");
    std::printf("  %s / %s / %s\n", qPrintable(Messages::ok()), qPrintable(panel.m_fixed->text()), qPrintable(panel.m_live->text()));

    std::printf("\n==== 2. 安装英文翻译 ====\n");
    // [region install]
    QTranslator english;
    const bool loaded = english.load("i18n_en", QCoreApplication::applicationDirPath());   // 找 i18n_en.qm
    QCoreApplication::installTranslator(&english);
    // [endregion]
    QApplication::processEvents();
    std::printf("  load=%d\n", loaded);
    std::printf("  Messages::ok()             → %s\n", qPrintable(Messages::ok()));
    std::printf("  同一个词，Report 上下文      → %s\n", qPrintable(Messages::okInOtherContext()));
    std::printf("  构造时设置的标签            → %s\n", qPrintable(panel.m_fixed->text()));
    std::printf("  changeEvent 里重设的标签    → %s\n", qPrintable(panel.m_live->text()));
    std::printf("  main 之前 tr() 的全局变量    → %s\n", qPrintable(kTitleTooEarly));
    std::printf("  QT_TRANSLATE_NOOP 标记、现在才 tr() → %s\n", qPrintable(QObject::tr(kTitleMarked)));

    std::printf("\n==== 3. 复数 ====\n");
    for (int n : {0, 1, 2, 21}) {
        panel.parts = n;
        QCoreApplication::removeTranslator(&english);         // 触发一次 LanguageChange，让面板重新取文字
        QCoreApplication::installTranslator(&english);
        QApplication::processEvents();
        std::printf("  n=%-2d → %s\n", n, qPrintable(panel.m_live->text()));
    }

    std::printf("\n==== 4. 移除翻译器，回到中文 ====\n");
    QCoreApplication::removeTranslator(&english);
    QApplication::processEvents();
    std::printf("  %s / %s\n", qPrintable(Messages::ok()), qPrintable(panel.m_live->text()));

    std::printf("\n==== 5. 数字和日期：QLocale ====\n");
    // [region locale]
    const double v = 1234567.891;
    const QDate d(2026, 10, 4);
    for (const char *name : {"zh_CN", "en_US", "de_DE", "fr_FR"}) {
        const QLocale loc(name);
        std::printf("  %-6s %-16s %-28s %s\n", name, qPrintable(loc.toString(v, 'f', 2)),
                    qPrintable(loc.toString(d, QLocale::LongFormat)), qPrintable(loc.toString(d, QLocale::ShortFormat)));
    }
    // [endregion]
    bool ok = false;
    const double parsed = QLocale("de_DE").toDouble("1.234,5", &ok);
    std::printf("  德语格式的 \"1.234,5\"：QLocale(de_DE).toDouble → %g（ok=%d）；QString::toDouble → %g\n",
                parsed, ok, QString("1.234,5").toDouble());
    return 0;
}

#include "main.moc"
