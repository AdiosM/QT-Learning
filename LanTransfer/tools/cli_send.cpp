// LanTransferSend：命令行发送工具（电脑 ↔ 电脑）
// 用法：
//   LanTransferSend <ip> <port> <文件> [--pin 123456] [--trust]
//     --pin    指定对端显示的 6 位配对确认码（正常使用方式）
//     --trust  不回显确认码，直接信任并自动确认（仅建议用于脚本/测试环境）
// 退出码：0 成功；1 失败
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QTextStream>
#include <QTimer>

#include "device/devicemanager.h"
#include "transfer/transfermanager.h"

using namespace lantransfer;

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("LanTransfer"));
    QCoreApplication::setApplicationName(QStringLiteral("LanTransferSend"));
    QTextStream out(stdout);
    QTextStream err(stderr);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("LanTransfer 命令行发送工具"));
    parser.addHelpOption();
    parser.addPositionalArgument(QStringLiteral("ip"), QStringLiteral("对方 IP"));
    parser.addPositionalArgument(QStringLiteral("port"), QStringLiteral("对方 TCP 端口"));
    parser.addPositionalArgument(QStringLiteral("file"), QStringLiteral("要发送的文件路径"));
    QCommandLineOption pinOpt(QStringLiteral("pin"), QStringLiteral("对端显示的 6 位配对确认码"), QStringLiteral("pin"));
    QCommandLineOption trustOpt(QStringLiteral("trust"), QStringLiteral("信任模式：自动确认配对（仅建议脚本/测试使用）"));
    QCommandLineOption nameOpt(QStringLiteral("name"), QStringLiteral("本机设备名"), QStringLiteral("name"));
    parser.addOption(pinOpt);
    parser.addOption(trustOpt);
    parser.addOption(nameOpt);
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.size() < 3) {
        err << QStringLiteral("用法：LanTransferSend <ip> <port> <文件> [--pin 123456] [--trust]\n");
        return 1;
    }
    const QString ip = args.at(0);
    const quint16 port = quint16(args.at(1).toUInt());
    const QString filePath = args.at(2);
    if (port == 0) {
        err << QStringLiteral("无效端口：") << args.at(1) << "\n";
        return 1;
    }
    const QFileInfo info(filePath);
    if (!info.exists()) {
        err << QStringLiteral("文件不存在：") << filePath << "\n";
        return 1;
    }
    if (quint64(info.size()) > MAX_FILE_SIZE) {
        err << QStringLiteral("文件超过 500 MB 限制\n");
        return 1;
    }

    DeviceManager devices;
    if (parser.isSet(nameOpt))
        devices.setLocalDeviceName(parser.value(nameOpt));
    TransferManager transfers(&devices);

    Device device;
    device.deviceId = ip;
    device.name = ip;
    device.ip = ip;
    device.tcpPort = port;

    bool success = false;
    QString errorText;
    QElapsedTimer clock;
    clock.start();

    QObject::connect(&transfers, &TransferManager::sessionCreated, &app,
                     [&](TransferSession* session) {
        if (session->role() != TransferRole::Sender)
            return;
        QObject::connect(session, &TransferSession::pinRequired, session,
                         [&, session](const QString& pin) {
            if (parser.isSet(trustOpt)) {
                session->userConfirmPin(pin); // 信任模式：直接回显
            } else if (parser.isSet(pinOpt)) {
                session->userConfirmPin(parser.value(pinOpt));
            } else {
                session->userCancel();
            }
        });
        QObject::connect(session, &TransferSession::progressChanged, session,
                         [&](quint64 transferred, quint64 total) {
            out << QStringLiteral("\r进度：%1%")
                       .arg(total > 0 ? transferred * 100 / total : 100)
                << Qt::flush;
        });
    });

    QObject::connect(&transfers, &TransferManager::sessionFinished, &app,
                     [&](TransferSession* session, bool ok) {
        success = ok;
        errorText = session->errorText();
        app.quit();
    });

    const QUuid taskId = transfers.sendFile(
        device, std::make_shared<QFileFileSource>(filePath), info.fileName(), info.size());
    if (taskId.isNull()) {
        err << QStringLiteral("发送任务创建失败（对端忙或文件异常）\n");
        return 1;
    }

    out << QStringLiteral("正在向 %1:%2 发送 %3（%4 字节）…\n")
               .arg(ip).arg(port).arg(info.fileName()).arg(info.size())
        << Qt::flush;

    // 总超时 10 分钟（覆盖大文件慢速场景）
    QTimer::singleShot(10 * 60 * 1000, &app, [&] { app.exit(2); });
    const int code = app.exec();
    out << "\n";

    if (code == 0 && success) {
        out << QStringLiteral("发送成功（%1 秒）\n")
                   .arg(double(clock.elapsed()) / 1000.0, 0, 'f', 1) << Qt::flush;
        return 0;
    }
    err << QStringLiteral("发送失败：%1\n")
               .arg(code == 2 ? QStringLiteral("超时") : (errorText.isEmpty() ? QStringLiteral("未知错误") : errorText));
    return 1;
}
