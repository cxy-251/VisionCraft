#include "DeviceLink.h"
#include "ScopeAnalysis.h"
#include "ScopeArchive.h"
#include <memory>
#include <cmath>
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
        QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
        QQuickStyle::setStyle(QStringLiteral("Basic"));
        DeviceLink link;
        qmlRegisterSingletonInstance("VisionCraft", 1, 0, "DeviceLink", &link);
        qmlRegisterSingletonType(QUrl::fromLocalFile(QStringLiteral(VC_SOURCE_DIR "/qml/Theme.qml")),
                                 "VisionCraft", 1, 0, "Theme");
        ScopeAnalysis analysis;
        qmlRegisterSingletonInstance("VisionCraft", 1, 0, "ScopeAnalysis", &analysis);
        ScopeArchive archive;
        qmlRegisterSingletonInstance("VisionCraft",1,0,"ScopeArchive",&archive);
        QQmlEngine engine;
        QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(VC_SOURCE_DIR "/qml/data/ScopePanel.qml")));
        QTRY_VERIFY_WITH_TIMEOUT(component.status() != QQmlComponent::Loading, 3000);
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        auto *panel = qobject_cast<QQuickItem *>(object.get());
        QVERIFY(panel);
        QQuickWindow window;
        panel->setParentItem(window.contentItem());
        panel->setWidth(1100); panel->setHeight(940);
        window.resize(1100, 940); window.show();
        const QString serialPort=qEnvironmentVariable("VC_SCOPE_TEST_SERIAL_PORT");
        if (serialPort.isEmpty()) link.connectSimulator();
        else link.connectSerial(serialPort,115200);
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
        const auto beforeZoomSpectrum = panel->property("spectrum").toMap();
        QVERIFY(zoom->setProperty("value", 4.0));
        QVERIFY(pan->setProperty("value", .5));
        QTest::qWait(250);
        QCOMPARE(panel->property("frames").toInt(), frames);
        QCOMPARE(link.scopeFrame(), frame);
        const auto spectrum = panel->property("spectrum").toMap();
        QCOMPARE(spectrum, beforeZoomSpectrum);
        QVERIFY(spectrum.value("valid").toBool());
        QVERIFY(std::abs(spectrum.value("peakHz").toDouble() - 1000) < spectrum.value("binHz").toDouble());
        auto *range = panel->findChild<QObject *>("scopeSpectrumRange");
        QVERIFY(range);
        QVERIFY(QMetaObject::invokeMethod(range, "clicked"));
        QVERIFY(panel->property("fullSpectrum").toBool());
        QCOMPARE(panel->property("spectrum").toMap(), spectrum);
        QVERIFY(QMetaObject::invokeMethod(range, "clicked"));
        QVERIFY(!panel->property("fullSpectrum").toBool());
        QTest::qWait(100);
        const QString snapshot = qEnvironmentVariable("VC_SCOPE_TEST_SNAPSHOT");
        if (!snapshot.isEmpty()) QVERIFY(window.grabWindow().save(snapshot));
        QTemporaryDir screenshots;
        QVERIFY(window.grabWindow().save(screenshots.filePath("paused-zoom.png")));
        QTemporaryDir records;
        const auto url=QUrl::fromLocalFile(records.filePath("capture.vcscope.json"));
        auto *saveButton=panel->findChild<QObject *>("scopeSave");
        auto *saveDialog=panel->findChild<QObject *>("scopeSaveDialog");
        QVERIFY(saveButton && saveDialog && saveButton->property("enabled").toBool());
        QVERIFY(QMetaObject::invokeMethod(saveButton,"clicked"));
        QVERIFY(saveDialog->setProperty("selectedFile",url));
        QVERIFY(QMetaObject::invokeMethod(saveDialog,"accepted"));
        QVERIFY(QMetaObject::invokeMethod(saveDialog,"close"));
        QVERIFY(QFileInfo::exists(url.toLocalFile()));
        const QString exported=qEnvironmentVariable("VC_SCOPE_TEST_SAVE");
        if (!exported.isEmpty()) QVERIFY(archive.save(QUrl::fromLocalFile(exported),frame));
        QVERIFY(QMetaObject::invokeMethod(button,"clicked"));
        QVERIFY(panel->property("continuous").toBool());
        auto *openButton=panel->findChild<QObject *>("scopeOpen");
        auto *openDialog=panel->findChild<QObject *>("scopeOpenDialog"); QVERIFY(openButton && openDialog);
        QVERIFY(QMetaObject::invokeMethod(openButton,"clicked"));
        QVERIFY(openDialog->setProperty("selectedFile",url));
        QVERIFY(QMetaObject::invokeMethod(openDialog,"accepted"));
        QVERIFY(QMetaObject::invokeMethod(openDialog,"close"));
        QVERIFY(panel->property("replaying").toBool());
        QVERIFY(!panel->property("continuous").toBool()); QVERIFY(!link.scopeBusy());
        QCOMPARE(panel->property("spectrum").toMap(),spectrum);
        const auto replay=panel->property("frame").toMap();
        const auto id=link.scopeFrame().value("id").toUInt();
        QVERIFY(link.startScope(50000,512));
        QTRY_VERIFY(link.scopeFrame().value("id").toUInt()>id);
        QCOMPARE(panel->property("frame").toMap(),replay);
        link.disconnectDevice();
        QVERIFY(panel->property("replaying").toBool());
        QVERIFY(panel->property("spectrum").toMap().value("valid").toBool());
        const QString replaySnapshot=qEnvironmentVariable("VC_SCOPE_REPLAY_SNAPSHOT");
        if (!replaySnapshot.isEmpty()) QVERIFY(window.grabWindow().save(replaySnapshot));
        auto *back=panel->findChild<QObject *>("scopeReturnLive"); QVERIFY(back);
        QVERIFY(QMetaObject::invokeMethod(back,"clicked")); QVERIFY(!panel->property("replaying").toBool());
        if (serialPort.isEmpty()) link.connectSimulator();
        else link.connectSerial(serialPort,115200);
        QTRY_COMPARE(link.state(),DeviceLink::Connected);
        QVERIFY(QMetaObject::invokeMethod(button,"clicked"));
        panel->setVisible(false);
        QVERIFY(!panel->property("continuous").toBool());
        QVERIFY(!link.scopeBusy());
        link.disconnectDevice();
        panel->setParentItem(nullptr);
    }
};
QTEST_MAIN(TstScopeUi)
#include "tst_scope_ui.moc"
