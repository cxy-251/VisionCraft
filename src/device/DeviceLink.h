#pragma once

#include "vc_protocol.h"

#include <QElapsedTimer>
#include <QImage>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>
#include <functional>

#include "Transport.h"

// 上位机访问 F407 的唯一入口。
//
// 职责：选择并管理通道（模拟器 / ST-Link RTT / 串口）、组帧拆帧、用 seq 把应答配对到请求、
// 超时、把事件和遥测转成 Qt 信号。界面和其他模块只调用这里，不碰通道和字节。
class DeviceLink : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString transportName READ transportName NOTIFY stateChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QVariantMap info READ info NOTIFY infoChanged)
    Q_PROPERTY(QVariantMap telemetry READ telemetry NOTIFY telemetryChanged)
    Q_PROPERTY(QVariantMap stats READ stats NOTIFY statsChanged)

public:
    enum State { Disconnected, Connecting, Connected };
    Q_ENUM(State)

    // 请求完成回调：ok 为 false 时 status 是 -1（超时/断开）或板子返回的错误码
    using Reply = std::function<void(bool ok, int status, const QByteArray &payload)>;

    explicit DeviceLink(QObject *parent = nullptr);
    ~DeviceLink() override;

    State state() const { return m_state; }
    QString transportName() const;
    QString statusText() const { return m_statusText; }
    QVariantMap info() const { return m_info; }
    QVariantMap telemetry() const { return m_telemetry; }
    QVariantMap stats() const;

    // ---- 连接 ----
    Q_INVOKABLE void connectSimulator();
    Q_INVOKABLE void connectRtt();
    Q_INVOKABLE void connectSerial(const QString &port, int baud);
    Q_INVOKABLE void disconnectDevice();
    Q_INVOKABLE QStringList serialPorts() const;

    // ---- 命令（返回本次请求的 seq，-1 表示未连接）----
    Q_INVOKABLE int ping();
    Q_INVOKABLE int getInfo();
    Q_INVOKABLE int beep(int durationMs);
    Q_INVOKABLE int setTimeNow();
    Q_INVOKABLE int subscribeTelemetry(int periodMs);
    Q_INVOKABLE int sendResult(bool ok, int defect, int inspectMs, int total, int ng);

    // 配方：读回来发 recipeReceived（空表示板子上没有），写完发 recipeSaved
    Q_INVOKABLE int getRecipe();
    Q_INVOKABLE int setRecipe(const QVariantMap &recipe);

    // 把图片缩到 160×120 以内、转成 RGB565，分块发给板子显示。正在发上一张时返回 false（不排队）
    bool sendImage(const QImage &image);
    bool imageBusy() const { return m_img.active; }

    // 连续发送 count 个带 payloadSize 字节负载的 PING，最多 window 个同时在途，测往返吞吐
    Q_INVOKABLE void runThroughputTest(int payloadSize, int count, int window = 4);

    // 烧录固件（只有 RTT 通道支持）
    Q_INVOKABLE void flashFirmware(const QString &elfPath);

    // C++ 侧的通用请求接口
    int request(uint8_t type, const QByteArray &payload, Reply reply = {}, int timeoutMs = 1000);

    // 测试用：拿到当前通道（例如模拟器）
    Transport *transport() const { return m_transport; }
    // 测试用：换成任意通道
    void connectWith(Transport *transport);

signals:
    void stateChanged();
    void statusTextChanged();
    void infoChanged();
    void telemetryChanged();
    void statsChanged();
    void eventReceived(const QVariantMap &event);
    void commandFinished(const QString &command, bool ok, const QString &summary);
    void logLine(const QString &kind, const QString &text);
    void throughputFinished(const QVariantMap &result);
    void flashFinished(bool ok, const QString &message);
    void imageSent(bool ok, int bytes, double ms);
    void recipeReceived(const QVariantMap &recipe);   // 空 map：板子上还没有配方
    void recipeSaved(bool ok);

private:
    struct Pending {
        uint8_t type;
        Reply reply;
        QElapsedTimer sent;
        int timeoutMs;
    };

    void setState(State s);
    void setStatus(const QString &text);
    void onBytes(const QByteArray &bytes);
    void onFrame(const vc_frame &f);
    void checkTimeouts();
    void noteStatsChanged();
    void failAllPending(const QString &why);
    void throughputStep();
    int simpleCommand(uint8_t type, const QByteArray &payload, const QString &label);
    static QString commandName(uint8_t type);

    QPointer<Transport> m_transport;
    State m_state = Disconnected;
    QString m_statusText;
    QVariantMap m_info;
    QVariantMap m_telemetry;

    vc_decoder m_decoder;
    uint8_t m_nextSeq = 1;
    bool m_infoPending = false;   // 连上时和收到 HELLO 时都会读信息，避免重复请求
    // 板子运行时间与电脑时间的换算：电脑时间 = 板子 uptime + m_uptimeOffsetMs（由 GET_INFO 应答确定）。
    // 遥测按板子的采样时刻打时间戳，连上时一下子收到的积压数据也能落在正确的时间点上
    qint64 m_uptimeOffsetMs = 0;
    bool m_uptimeOffsetValid = false;
    QHash<uint8_t, Pending> m_pending;
    QTimer m_timeoutTimer;

    quint64 m_txFrames = 0;
    quint64 m_txBytes = 0;
    quint64 m_rxBytes = 0;
    double m_lastRttMs = 0;
    QTimer m_statsTimer;          // 统计数据变化很快，限制刷新频率

    struct ImageTransfer {
        bool active = false;
        QByteArray pixels;     // RGB565，小端
        int next = 0;          // 下一个要发的字节偏移
        int inFlight = 0;
        bool failed = false;
        QElapsedTimer clock;
    } m_img;
    void imageStep();

    struct Throughput {
        bool running = false;
        int payloadSize = 0;
        int remaining = 0;
        int inFlight = 0;
        int window = 1;
        int done = 0;
        int failed = 0;
        QElapsedTimer clock;
        QList<double> rtts;
    } m_tp;
};
