#include "MatDemo.h"

#include <opencv2/imgproc.hpp>

MatDemo::MatDemo(QObject *parent)
    : QObject(parent)
{
    reset();
}

void MatDemo::note(const QString &code, const QString &what)
{
    m_log.prepend(code + QStringLiteral("    // ") + what);
    while (m_log.size() > 8)
        m_log.removeLast();
    emit changed();
}

void MatDemo::reset()
{
    m_b.release();
    m_c.release();
    m_r.release();
    // 8×8 的灰度渐变，方便看出谁被改了
    m_a.create(8, 8, CV_8UC1);
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x)
            m_a.at<uchar>(y, x) = uchar(20 + 12 * (x + y));
    m_log.clear();
    note(QStringLiteral("cv::Mat A(8, 8, CV_8UC1)"), tr("新建 A，分配 64 字节"));
}

void MatDemo::assignB()
{
    m_b = m_a;
    note(QStringLiteral("B = A"), tr("只复制「头」，B 和 A 指向同一块数据，引用计数 +1"));
}

void MatDemo::cloneC()
{
    m_c = m_a.clone();
    note(QStringLiteral("C = A.clone()"), tr("分配新内存并复制全部像素，C 和 A 从此无关"));
}

void MatDemo::roiR()
{
    if (m_a.empty()) {
        note(QStringLiteral("R = A(...)"), tr("A 是空的，取不了 ROI"));
        return;
    }
    m_r = m_a(cv::Rect(2, 2, 4, 4));
    note(QStringLiteral("R = A(cv::Rect(2, 2, 4, 4))"), tr("R 是 A 里的一个窗口：同一块数据，起点偏移，引用计数 +1"));
}

void MatDemo::paintA()
{
    if (m_a.empty()) {
        note(QStringLiteral("A(...) = 255"), tr("A 是空的"));
        return;
    }
    m_a(cv::Rect(3, 3, 2, 2)).setTo(255);
    note(QStringLiteral("A(cv::Rect(3, 3, 2, 2)).setTo(255)"), tr("改 A 的像素：所有共用这块数据的 Mat 都跟着变"));
}

void MatDemo::paintR()
{
    if (m_r.empty()) {
        note(QStringLiteral("R.setTo(90)"), tr("R 还没有，先取 ROI"));
        return;
    }
    m_r.setTo(90);
    note(QStringLiteral("R.setTo(90)"), tr("改 ROI 就是改 A 里对应的那一块"));
}

void MatDemo::releaseA()
{
    m_a.release();
    note(QStringLiteral("A.release()"), tr("A 不再引用数据，引用计数 -1；只要还有别人引用，数据就不会被释放"));
}

QVariantList MatDemo::mats() const
{
    QVariantList out;
    // 每块数据分配一个颜色编号，界面上用同一种颜色标出共用同一块内存的 Mat
    QList<const uchar *> blocks;
    auto blockOf = [&blocks](const cv::Mat &m) -> int {
        if (m.empty())
            return -1;
        const uchar *start = m.datastart;
        int i = int(blocks.indexOf(start));
        if (i < 0) {
            blocks << start;
            i = int(blocks.size()) - 1;
        }
        return i;
    };
    const QList<QPair<QString, const cv::Mat *>> all = {
        {QStringLiteral("A"), &m_a}, {QStringLiteral("B"), &m_b}, {QStringLiteral("C"), &m_c}, {QStringLiteral("R"), &m_r}};
    for (const auto &[name, m] : all) {
        QImage img;
        if (!m->empty()) {
            const cv::Mat c = m->clone();   // ROI 不连续，先拷成连续的再交给 QImage
            img = QImage(c.data, c.cols, c.rows, int(c.step), QImage::Format_Grayscale8).copy();
        }
        out << QVariantMap{
            {QStringLiteral("name"), name},
            {QStringLiteral("empty"), m->empty()},
            {QStringLiteral("addr"), m->empty() ? QString() : QStringLiteral("0x%1").arg(quintptr(m->data), 0, 16)},
            {QStringLiteral("refcount"), m->u ? m->u->refcount : 0},
            {QStringLiteral("size"), m->empty() ? QString() : QStringLiteral("%1×%2").arg(m->cols).arg(m->rows)},
            {QStringLiteral("block"), blockOf(*m)},
            {QStringLiteral("image"), img},
        };
    }
    return out;
}
