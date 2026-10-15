#include "DeviceLink.h"
#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QSettings>

class TstScopeUi : public QObject {
    Q_OBJECT
private slots:
    void capturePauseZoomAndLeave()
    {
        QTemporaryDir preferences;
        QCoreApplication::setOrganizationName("VisionCraftTests");
        QCoreApplication::setOrganizationDomain("tests.visioncraft");
        QCoreApplication::setApplicationName("ScopeUi");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, preferences.path());
        QQuickStyle::setStyle(QStringLiteral("Basic"));
        DeviceLink link;
        qmlRegisterSingletonInstance("VisionCraft", 1, 0, "DeviceLink", &link);
        qmlRegisterSingletonType(QUrl::fromLocalFile(QStringLiteral(VC_SOURCE_DIR "/qml/Theme.qml")),
                                 "VisionCraft", 1, 0, "Theme");
        QQmlEngine engine;
        QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(VC_SOURCE_DIR "/qml/data/ScopePanel.qml")));
        QTRY_VERIFY_WITH_TIMEOUT(component.status() != QQmlComponent::Loading, 3000);
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        auto *panel = qobject_cast<QQuickItem *>(object.get());
        QVERIFY(panel);
        QQuickWindow window;
        panel->setParentItem(window.contentItem());
        panel->setWidth(1100); panel->setHeight(620);
        window.resize(1100, 620); window.show();
        link.connectSimulator();
        QTRY_COMPARE(link.state(), DeviceLink::Connected);
        auto *button = panel->findChild<QObject *>("scopeContinuous");
        auto *zoom = panel->findChild<QObject *>("scopeZoom");
        auto *pan = panel->findChild<QObject *>("scopePan");
        QVERIFY(button && zoom && pan);
        QVERIFY(QMetaObject::invokeMethod(button, "clicked"));
        QTRY_VERIFY_WITH_TIMEOUT(panel->property("frames").toInt() >= 3, 4000);
        QVERIFY(panel->property("continuous").toBool());
        QVERIFY(QMetaObject::invokeMethod(button, "clicked"));
        QVERIFY(!panel->property("continuous").toBool());
        QVERIFY(!link.scopeBusy());
        const auto frame = link.scopeFrame();
        const int frames = panel->property("frames").toInt();
        QVERIFY(zoom->setProperty("value", 4.0));
        QVERIFY(pan->setProperty("value", .5));
        QTest::qWait(250);
        QCOMPARE(panel->property("frames").toInt(), frames);
        QCOMPARE(link.scopeFrame(), frame);
        QTemporaryDir screenshots;
        QVERIFY(window.grabWindow().save(screenshots.filePath("paused-zoom.png")));
        QVERIFY(QMetaObject::invokeMethod(button, "clicked"));
        panel->setVisible(false);
        QVERIFY(!panel->property("continuous").toBool());
        QVERIFY(!link.scopeBusy());
        link.disconnectDevice();
        panel->setParentItem(nullptr);
    }
};
QTEST_MAIN(TstScopeUi)
#include "tst_scope_ui.moc"
