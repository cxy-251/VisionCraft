#include "RttTransport.h"

#include <QDir>
#include <QStandardPaths>

RttTransport::RttTransport(QObject *parent)
    : Transport(parent)
{
    m_process.setProcessChannelMode(QProcess::SeparateChannels);
    connect(&m_process, &QProcess::readyReadStandardError, this, &RttTransport::onStderr);
    connect(&m_process, &QProcess::readyReadStandardOutput, this, [this] { m_process.readAllStandardOutput(); });
    connect(&m_process, &QProcess::finished, this, [this](int code) {
        if (m_step != Step::Idle)
            fail(tr("OpenOCD 退出了（退出码 %1）").arg(code));
    });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart)
            fail(tr("无法启动 OpenOCD：%1").arg(openOcdProgram()));
    });

    connect(&m_tclSocket, &QTcpSocket::connected, this, [this] {
        emit progress(tr("已连上 OpenOCD，检查芯片在运行什么"));
        checkRunningFirmware();
    });
    connect(&m_tclSocket, &QTcpSocket::readyRead, this, &RttTransport::onTclReadyRead);

    connect(&m_dataSocket, &QTcpSocket::connected, this, [this] {
        m_step = Step::Open;
        m_timeout.stop();
        emit opened();
    });
    connect(&m_dataSocket, &QTcpSocket::readyRead, this, [this] { emit bytesReceived(m_dataSocket.readAll()); });
    connect(&m_dataSocket, &QTcpSocket::disconnected, this, [this] {
        if (m_step == Step::Open)
            fail(tr("RTT 数据连接断开"));
    });

    m_timeout.setSingleShot(true);
    connect(&m_timeout, &QTimer::timeout, this, [this] { fail(tr("连接超时")); });
}

RttTransport::~RttTransport()
{
    m_step = Step::Idle;
    if (m_process.state() != QProcess::NotRunning) {
        m_process.terminate();
        if (!m_process.waitForFinished(2000))
            m_process.kill();
    }
}

QString RttTransport::openOcdProgram()
{
    QString p = QStandardPaths::findExecutable(QStringLiteral("openocd"));
    if (p.isEmpty())
        p = QDir::homePath() + QStringLiteral("/Applications/openocd/usr/bin/openocd");
    return p;
}

void RttTransport::open()
{
    m_step = Step::StartingProcess;
    m_log.clear();
    m_controlBlockFound = false;
    m_timeout.start(15000);
    emit progress(tr("启动 OpenOCD"));
    m_process.start(openOcdProgram(), {
        QStringLiteral("-f"), QStringLiteral("interface/stlink.cfg"),
        QStringLiteral("-f"), QStringLiteral("target/stm32f4x.cfg"),
        QStringLiteral("-c"), QStringLiteral("gdb_port disabled"),
        QStringLiteral("-c"), QStringLiteral("telnet_port disabled"),
        QStringLiteral("-c"), QStringLiteral("tcl_port %1").arg(kTclPort),
        QStringLiteral("-c"), QStringLiteral("init"),
    });
}

void RttTransport::close()
{
    const bool wasOpen = m_step != Step::Idle;
    m_step = Step::Idle;
    m_timeout.stop();
    m_dataSocket.abort();
    m_tclSocket.abort();
    m_tclQueue.clear();
    m_tclBusy = false;
    if (m_process.state() != QProcess::NotRunning) {
        m_process.terminate();
        if (!m_process.waitForFinished(2000))
            m_process.kill();
    }
    if (wasOpen)
        emit closed();
}

void RttTransport::write(const QByteArray &bytes)
{
    if (m_step == Step::Open)
        m_dataSocket.write(bytes);
}

void RttTransport::fail(const QString &reason)
{
    if (m_step == Step::Idle)
        return;
    QStringList tail = m_log.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    while (tail.size() > 4)
        tail.removeFirst();
    close();
    emit failed(tail.isEmpty() ? reason : reason + QStringLiteral("\n") + tail.join(QLatin1Char('\n')));
}

void RttTransport::onStderr()
{
    const QString text = QString::fromLocal8Bit(m_process.readAllStandardError());
    m_log += text;
    if (m_log.size() > 20000)
        m_log = m_log.right(10000);

    for (const QString &line : text.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        if (line.contains(QStringLiteral("Listening on port %1 for tcl").arg(QString::number(kTclPort)))) {
            m_step = Step::SettingUpRtt;
            m_tclSocket.connectToHost(QStringLiteral("127.0.0.1"), kTclPort);
        } else if (line.contains(QLatin1String("Control block found"))) {
            m_controlBlockFound = true;
        } else if (line.contains(QLatin1String("Error:")) && m_step == Step::StartingProcess) {
            emit progress(line.trimmed());
        }
    }
}

// ---- TCL：命令以 0x1A 结尾，回复也以 0x1A 结尾，一问一答 ----

void RttTransport::tcl(const QString &command, TclCallback done)
{
    m_tclQueue.enqueue({command, std::move(done)});
    sendNextTcl();
}

void RttTransport::sendNextTcl()
{
    if (m_tclBusy || m_tclQueue.isEmpty() || m_tclSocket.state() != QAbstractSocket::ConnectedState)
        return;
    m_tclBusy = true;
    m_tclSocket.write(m_tclQueue.head().command.toUtf8() + '\x1a');
}

void RttTransport::onTclReadyRead()
{
    m_tclBuffer += m_tclSocket.readAll();
    qsizetype end;
    while ((end = m_tclBuffer.indexOf('\x1a')) >= 0) {
        const QString reply = QString::fromUtf8(m_tclBuffer.left(end)).trimmed();
        m_tclBuffer.remove(0, end + 1);
        if (m_tclQueue.isEmpty())
            continue;
        TclRequest req = m_tclQueue.dequeue();
        m_tclBusy = false;
        // OpenOCD 的 TCL 回复不区分成功失败，失败时一般以 "Error" 开头或包含 "invalid command"
        const bool ok = !reply.startsWith(QLatin1String("Error")) && !reply.contains(QLatin1String("invalid command"));
        if (req.done)
            req.done(ok, reply);
        sendNextTcl();
    }
}

// ---- RTT 启动 ----

// [region boot]
QString RttTransport::bootFromFlashCommand() const
{
    // 板子的 BOOT0 被拉高时，复位后跑的是芯片内置的 bootloader 而不是 Flash 里的程序。
    // 这里用调试器模拟「从 Flash 启动」：复位并停住 → 向量表指到 Flash → 取 Flash 开头
    // 的栈顶和复位入口地址填进 MSP、PC → 继续运行。
    return QStringLiteral("reset halt; mww 0xE000ED08 0x08000000; "
                          "set v [read_memory 0x08000000 32 2]; "
                          "reg msp [lindex $v 0]; reg pc [lindex $v 1]; resume");
}
// [endregion]

// [region check]
// 找到 RTT 控制块不代表固件在运行：芯片复位进了 bootloader 时，RAM 里可能还留着上次运行的控制块，
// 调试器照样「找得到」，可是没有程序在应答（实测过：板子被意外复位后，连接成功但所有命令超时）。
// 所以先让 CPU 停一下读 PC：PC 不在 Flash 里，就先从 Flash 启动固件。
void RttTransport::checkRunningFirmware()
{
    tcl(QStringLiteral("halt; set vc_pc [lindex [reg pc] 2]; resume; set vc_pc"), [this](bool ok, const QString &reply) {
        bool parsed = false;
        const quint32 pc = reply.trimmed().toUInt(&parsed, 16);
        const bool inFlash = ok && parsed && pc >= 0x08000000u && pc < 0x08100000u;
        if (inFlash) {
            startRtt(false);
            return;
        }
        emit progress(tr("芯片没有在运行 Flash 里的程序（PC = %1），先从 Flash 启动固件").arg(reply.trimmed()));
        tcl(bootFromFlashCommand(), [this](bool, const QString &) {
            QTimer::singleShot(800, this, [this] { startRtt(true); });
        });
    });
}
// [endregion]

// [region start]
void RttTransport::startRtt(bool afterBoot)
{
    m_controlBlockFound = false;
    // 每次都重新 setup：OpenOCD 0.12 在 rtt stop 之后直接 rtt start 不会重新搜索控制块
    tcl(QStringLiteral("rtt setup 0x20000000 0x20000 {SEGGER RTT}"));
    // 默认 100 ms 轮询一次，太慢。必须在 rtt setup 之后设置：之前设置会让 OpenOCD 0.12 崩溃
    tcl(QStringLiteral("rtt polling_interval 1"));
    tcl(QStringLiteral("rtt start"), [this, afterBoot](bool, const QString &) {
        // 「找到控制块」写在 OpenOCD 的日志里，稍等日志到达再判断
        QTimer::singleShot(300, this, [this, afterBoot] {
            if (m_step != Step::SettingUpRtt)
                return;
            if (m_controlBlockFound) {
                emit progress(tr("找到 RTT 控制块，打开数据通道"));
                tcl(QStringLiteral("rtt server start %1 0").arg(kRttPort), [this](bool, const QString &) {
                    m_step = Step::ConnectingData;
                    m_dataSocket.connectToHost(QStringLiteral("127.0.0.1"), kRttPort);
                });
                return;
            }
            if (afterBoot) {
                fail(tr("芯片里没有找到 RTT 控制块：Flash 里的程序可能不是 VisionCraft 工位固件"));
                return;
            }
            emit progress(tr("没找到 RTT 控制块，尝试从 Flash 启动固件"));
            tcl(QStringLiteral("rtt stop"));
            tcl(bootFromFlashCommand(), [this](bool, const QString &) {
                QTimer::singleShot(800, this, [this] { startRtt(true); });   // 等固件初始化完 RTT
            });
        });
    });
}
// [endregion]

void RttTransport::flash(const QString &elfPath, std::function<void(bool, const QString &)> done)
{
    if (m_step != Step::Open) {
        done(false, tr("请先连接"));
        return;
    }
    m_step = Step::SettingUpRtt;
    m_dataSocket.abort();
    emit progress(tr("烧录 %1").arg(elfPath));
    tcl(QStringLiteral("rtt server stop %1").arg(kRttPort));
    tcl(QStringLiteral("rtt stop"));
    tcl(QStringLiteral("program {%1} verify").arg(elfPath), [this, done](bool ok, const QString &reply) {
        if (!ok || m_log.right(2000).contains(QLatin1String("** Programming Failed **"))) {
            done(false, reply.isEmpty() ? tr("烧录失败") : reply);
            fail(tr("烧录失败"));
            return;
        }
        tcl(bootFromFlashCommand(), [this, done](bool, const QString &) {
            QTimer::singleShot(800, this, [this, done] {
                done(true, tr("烧录完成，已启动"));
                startRtt(true);
            });
        });
    });
}
