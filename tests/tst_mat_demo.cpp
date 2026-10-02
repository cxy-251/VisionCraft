#include "MatDemo.h"

#include <QtTest>

// 手册「cv::Mat 内存模型」里写的每一条，都在这里用真实的 OpenCV 验证一遍
class TstMatDemo : public QObject {
    Q_OBJECT

    static QVariantMap mat(const MatDemo &d, const QString &name)
    {
        for (const QVariant &v : d.mats())
            if (v.toMap().value("name") == QVariant(name))
                return v.toMap();
        return {};
    }

private slots:
    void sharingAndRefcounts()
    {
        MatDemo d;
        QCOMPARE(mat(d, "A").value("refcount").toInt(), 1);

        d.assignB();   // B = A：共用数据
        QCOMPARE(mat(d, "A").value("refcount").toInt(), 2);
        QCOMPARE(mat(d, "B").value("addr"), mat(d, "A").value("addr"));
        QCOMPARE(mat(d, "B").value("block"), mat(d, "A").value("block"));

        d.cloneC();    // C = A.clone()：新数据
        QCOMPARE(mat(d, "C").value("refcount").toInt(), 1);
        QVERIFY(mat(d, "C").value("block") != mat(d, "A").value("block"));
        QCOMPARE(mat(d, "A").value("refcount").toInt(), 2);

        d.roiR();      // R = A(roi)：同一块数据，起点不同
        QCOMPARE(mat(d, "A").value("refcount").toInt(), 3);
        QCOMPARE(mat(d, "R").value("block"), mat(d, "A").value("block"));
        QVERIFY(mat(d, "R").value("addr") != mat(d, "A").value("addr"));

        d.paintA();    // 改 A：B 跟着变，C 不变
        const QImage a = mat(d, "A").value("image").value<QImage>();
        const QImage b = mat(d, "B").value("image").value<QImage>();
        const QImage c = mat(d, "C").value("image").value<QImage>();
        QCOMPARE(qGray(a.pixel(3, 3)), 255);
        QCOMPARE(qGray(b.pixel(3, 3)), 255);
        QVERIFY(qGray(c.pixel(3, 3)) != 255);

        d.paintR();    // 改 R = 改 A 的 (2,2)~(5,5)
        QCOMPARE(qGray(mat(d, "A").value("image").value<QImage>().pixel(2, 2)), 90);

        d.releaseA();  // A 放手，数据还在（B、R 还引用着）
        QVERIFY(mat(d, "A").value("empty").toBool());
        QCOMPARE(mat(d, "B").value("refcount").toInt(), 2);
        QCOMPARE(qGray(mat(d, "B").value("image").value<QImage>().pixel(2, 2)), 90);
    }
};

QTEST_GUILESS_MAIN(TstMatDemo)
#include "tst_mat_demo.moc"
