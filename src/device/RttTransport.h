#pragma once

#include "Transport.h"

#include <QProcess>
#include <QQueue>
#include <QTcpSocket>
#include <QTimer>
#include <functional>

// 经 ST-Link 调试口通信（SEGGER RTT 协议）。
//
// 固件在内存里放一个「RTT 控制块」，里面是上行、下行两个环形缓冲区。OpenOCD 通过 SWD
// 在芯片运行时直接读写这块内存，并把 0 号通道转成本机的一个 TCP 端口。所以：
//   OpenOCD 进程   ← 本类启动并管理
//   TCL 端口 6666  ← 给 OpenOCD 发命令（设置 RTT、烧录、复位）
//   TCP 端口 19021 ← RTT 0 号通道的数据，协议帧就在这里收发
// 只要一根 ST-Link 线，烧录、复位和通信都走它。
class RttTransport : public Transport {
    Q_OBJECT
public:
    using TclCallback = std::function<void(bool ok, const QString &reply)>;

    explicit RttTransport(QObject *parent = nullptr);
    ~RttTransport() override;

    QString name() const override { return QStringLiteral("ST-Link（RTT）"); }
    void open() override;
    void close() override;
    void write(const QByteArray &bytes) override;

    // 给 OpenOCD 发一条 TCL 命令，按顺序排队执行
    void tcl(const QString &command, TclCallback done = {});

    // 烧录固件并启动：暂停 RTT → program → 从 Flash 启动 → 重新找 RTT 控制块
    void flash(const QString &elfPath, std::function<void(bool ok, const QString &message)> done);

    static QString openOcdProgram();

private:
    enum class Step { Idle, StartingProcess, SettingUpRtt, ConnectingData, Open };

    void onStderr();
    void onTclReadyRead();
    void sendNextTcl();
    void checkRunningFirmware();
    void startRtt(bool afterBoot);
    void fail(const QString &reason);
    QString bootFromFlashCommand() const;

    QProcess m_process;
    QTcpSocket m_tclSocket;
    QTcpSocket m_dataSocket;
    QTimer m_timeout;
    Step m_step = Step::Idle;

    struct TclRequest {
        QString command;
        TclCallback done;
    };
    QQueue<TclRequest> m_tclQueue;
    bool m_tclBusy = false;
    QByteArray m_tclBuffer;

    QString m_log;            // OpenOCD 的输出，出错时给用户看最后几行
    bool m_controlBlockFound = false;

    static constexpr quint16 kTclPort = 6666;
    static constexpr quint16 kRttPort = 19021;
};
