#include "DeviceLink.h"
#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <memory>
class TstRuntimeUi : public QObject {
    Q_OBJECT
    static bool textExists(QQuickItem *root,const QString &text) {
        if(root->property("text").toString()==text) return true;
        for(QQuickItem *item:root->childItems()) if(textExists(item,text)) return true;
        return false;
    }
private slots:
    void pollPauseManualRefreshAndDisconnect() {
        QCoreApplication::setOrganizationName("VisionCraftTests");
        QCoreApplication::setOrganizationDomain("tests.visioncraft");
        QQuickStyle::setStyle(QStringLiteral("Basic"));
        DeviceLink link;
        qmlRegisterSingletonInstance("VisionCraft",1,0,"DeviceLink",&link);
        qmlRegisterSingletonType(QUrl::fromLocalFile(QStringLiteral(VC_SOURCE_DIR "/qml/Theme.qml")),"VisionCraft",1,0,"Theme");
        QQmlEngine engine;
        QQmlComponent component(&engine,QUrl::fromLocalFile(QStringLiteral(VC_SOURCE_DIR "/qml/device/RuntimePanel.qml")));
        QTRY_VERIFY(component.status()!=QQmlComponent::Loading);
        QVERIFY2(component.isReady(),qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        auto *panel=qobject_cast<QQuickItem *>(object.get());QVERIFY(panel);
        QQuickWindow window;panel->setParentItem(window.contentItem());panel->setWidth(1100);panel->setHeight(1000);
        window.resize(1100,1000);window.show();link.connectSimulator();
        QTRY_VERIFY(panel->property("available").toBool());
        QTRY_VERIFY(textExists(panel,QStringLiteral("stationTask")));
        QTRY_VERIFY(link.monitorHistory().size()>=2);
        auto *automatic=panel->findChild<QObject *>("runtimeAuto");auto *refresh=panel->findChild<QObject *>("runtimeRefresh");
        auto *poll=panel->findChild<QObject *>("runtimePoll");auto *expand=panel->findChild<QObject *>("runtimeExpand");
        QVERIFY(automatic && refresh && poll && expand);
        automatic->setProperty("checked",false);QVERIFY(!poll->property("running").toBool());
        QTest::qWait(100);const auto count=link.monitorHistory().size();QTest::qWait(1100);QCOMPARE(link.monitorHistory().size(),count);
        QVERIFY(QMetaObject::invokeMethod(refresh,"clicked"));QTRY_COMPARE(link.monitorHistory().size(),count+1);
        QTest::qWait(100);
        const QString file=qEnvironmentVariable("VC_RUNTIME_TEST_SNAPSHOT");if(!file.isEmpty()) QVERIFY(window.grabWindow().save(file));
        QVERIFY(QMetaObject::invokeMethod(expand,"clicked"));QVERIFY(panel->property("allMetrics").toBool());
        automatic->setProperty("checked",true);panel->setVisible(false);QVERIFY(!poll->property("running").toBool());
        QTest::qWait(100);const auto hiddenCount=link.monitorHistory().size();QTest::qWait(1100);QCOMPARE(link.monitorHistory().size(),hiddenCount);
        panel->setVisible(true);QTRY_VERIFY(link.monitorHistory().size()>hiddenCount);
        link.disconnectDevice();QVERIFY(!panel->property("available").toBool());QTRY_VERIFY(!textExists(panel,QStringLiteral("stationTask")));
        QVERIFY(link.monitorHistory().isEmpty());
    }
};
QTEST_MAIN(TstRuntimeUi)
#include "tst_runtime_ui.moc"
