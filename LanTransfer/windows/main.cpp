#include <QApplication>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QSettings>

#include "device/devicemanager.h"
#include "discovery/discoverymanager.h"
#include "mainwindow.h"
#include "transfer/transfermanager.h"

using namespace lantransfer;

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("LanTransfer"));
    QCoreApplication::setApplicationName(QStringLiteral("LanTransfer"));
    QApplication::setApplicationDisplayName(QStringLiteral("LanTransfer 局域网文件互传"));
    QApplication::setApplicationVersion(QStringLiteral(LANTRANSFER_APP_VERSION));

    // 命令行参数（端口冲突时可用；也便于脚本化部署）
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("LanTransfer 局域网文件互传（电脑 ↔ 电脑）"));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption tcpPortOpt(QStringLiteral("tcp-port"),
                                  QStringLiteral("TCP 监听端口（默认 50000）"),
                                  QStringLiteral("port"));
    QCommandLineOption udpPortOpt(QStringLiteral("udp-port"),
                                  QStringLiteral("UDP 发现端口（默认 50001）"),
                                  QStringLiteral("port"));
    QCommandLineOption receiveDirOpt(QStringLiteral("receive-dir"),
                                     QStringLiteral("接收目录（默认系统下载目录）"),
                                     QStringLiteral("dir"));
    QCommandLineOption deviceIdOpt(QStringLiteral("device-id"),
                                   QStringLiteral("覆写本机设备 ID（同一台电脑跑多个实例时用于区分）"),
                                   QStringLiteral("id"));
    parser.addOption(tcpPortOpt);
    parser.addOption(udpPortOpt);
    parser.addOption(receiveDirOpt);
    parser.addOption(deviceIdOpt);
    parser.process(app);

    DeviceManager devices;
    if (parser.isSet(deviceIdOpt))
        devices.setLocalDeviceId(parser.value(deviceIdOpt));
    TransferManager transfers(&devices);
    DiscoveryManager discovery(&devices);

    // ---- TCP 监听：命令行 > 配置 > 默认；被占用时回退临时端口（设计文档 §5.1）----
    QSettings settings;
    quint16 configuredPort = DEFAULT_TCP_PORT;
    if (parser.isSet(tcpPortOpt))
        configuredPort = quint16(parser.value(tcpPortOpt).toUInt());
    else
        configuredPort = quint16(settings.value(QStringLiteral("net/tcpPort"), int(DEFAULT_TCP_PORT)).toInt());

    if (!transfers.startServer(configuredPort)) {
        if (!transfers.startServer(0)) {
            QMessageBox::critical(nullptr, QStringLiteral("LanTransfer"),
                                  QStringLiteral("TCP 监听失败：%1\n程序无法启动。")
                                      .arg(transfers.serverError()));
            return 1;
        }
        QMessageBox::information(nullptr, QStringLiteral("LanTransfer"),
                                 QStringLiteral("端口 %1 被占用，本次改用端口 %2。")
                                     .arg(configuredPort).arg(transfers.serverPort()));
    }

    // ---- 接收目录：命令行 > 配置 > 系统下载目录 ----
    if (parser.isSet(receiveDirOpt)) {
        transfers.setReceiveDir(parser.value(receiveDirOpt));
    } else {
        const QString receiveDir = settings.value(QStringLiteral("storage/receiveDir")).toString();
        if (!receiveDir.isEmpty())
            transfers.setReceiveDir(receiveDir);
    }

    // ---- UDP 设备发现 ----
    discovery.setTcpPort(transfers.serverPort());
    const quint16 udpPort = parser.isSet(udpPortOpt)
        ? quint16(parser.value(udpPortOpt).toUInt())
        : quint16(settings.value(QStringLiteral("net/udpPort"), int(DEFAULT_UDP_PORT)).toInt());
    discovery.start(udpPort); // 失败不阻断启动（手动 IP 连接仍可用）

    MainWindow window(&devices, &transfers, &discovery);
    window.show();
    return app.exec();
}
