#include "mainwindow.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStringList>
#include <QTcpServer>
#include <QTimer>
#include <QUrl>

namespace {
QString resolveFilePath(const char *environmentVariable, const QStringList &candidates)
{
    const QDir applicationDir(QCoreApplication::applicationDirPath());
    const QString overridePath = qEnvironmentVariable(environmentVariable);
    // 显式指定的路径无效时直接报错，不悄悄使用另一个文件。
    if (!overridePath.isEmpty())
        return applicationDir.absoluteFilePath(overridePath);

    for (const QString &candidate : candidates) {
        if (!candidate.isEmpty() && QFileInfo(candidate).isFile())
            return QFileInfo(candidate).canonicalFilePath();
    }
    return candidates.value(0);
}
} // namespace

void MainWindow::startModel()//程序刚启动，在mainwindow.cpp的构造函数中被调用
{
    const QDir applicationDir(QCoreApplication::applicationDirPath());
    QStringList serverCandidates{
        applicationDir.filePath(QStringLiteral("runtime/llama-server.exe")),
        applicationDir.filePath(QStringLiteral("llama-server.exe")),
        QStandardPaths::findExecutable(QStringLiteral("llama-server.exe"))
    };
    // Qt Creator 可能尚未继承安装 WinGet 软件后更新的 PATH。
    const QString localAppData = qEnvironmentVariable("LOCALAPPDATA");
    if (!localAppData.isEmpty()) {
        const QDir packages(localAppData + QStringLiteral("/Microsoft/WinGet/Packages"));
        const QStringList packageNames = packages.entryList(
            QStringList{QStringLiteral("ggml.llamacpp_*")}, QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &name : packageNames)
            serverCandidates.append(packages.filePath(name + QStringLiteral("/llama-server.exe")));
    }
    const QString serverPath = resolveFilePath("LOCAL_TRANSLATE_SERVER", serverCandidates);

    const QString modelName = QStringLiteral("Hy-MT2-1.8B-Q4_K_M.gguf");
    const QString modelPath = resolveFilePath("LOCAL_TRANSLATE_MODEL", QStringList{
        applicationDir.filePath(QStringLiteral("models/") + modelName),
        applicationDir.filePath(modelName),
        // 兼容当前电脑已有的模型位置，无需移动大文件。
        "D:/AI/Models/Hy-MT2/Hy-MT2-1.8B-Q4_K_M.gguf"
    });

    if (!QFileInfo(serverPath).isFile()) {
        modelFailed(QStringLiteral(
                        "找不到 llama-server.exe。请将完整运行时放入程序目录下的 runtime 文件夹，"
                        "或设置 LOCAL_TRANSLATE_SERVER 环境变量。\n检查路径：\n")
                    + serverPath);
        return;
    }

    if (!QFileInfo(modelPath).isFile()) {
        modelFailed(QStringLiteral(
                        "找不到模型文件。请将模型放入程序目录下的 models 文件夹，"
                        "或设置 LOCAL_TRANSLATE_MODEL 环境变量。\n检查路径：\n")
                    + modelPath);
        return;
    }

    // 避免误连接之前手动启动的服务。
    QTcpServer portCheck;
    if (!portCheck.listen(QHostAddress::LocalHost, 8080)) {
        modelFailed(QStringLiteral(
            "本机 8080 端口已被占用。\n"
            "请先关闭之前手动启动的 llama-server，"
            "或检查是否已经打开了另一个翻译软件窗口，然后重启软件。"
            ));
        return;
    }
    portCheck.close();

    modelProcess = new QProcess(this);
    modelProcess->setWorkingDirectory(
        QFileInfo(serverPath).absolutePath()
        );

    // 合并并持续读取日志，保留末尾内容用于显示错误。
    modelProcess->setProcessChannelMode(QProcess::MergedChannels);

    connect(modelProcess, &QProcess::readyReadStandardOutput,
            this, [this]() {
                modelLog += QString::fromUtf8(
                    modelProcess->readAllStandardOutput()
                    );
                modelLog = modelLog.right(12000);
            });

    connect(modelProcess, &QProcess::errorOccurred,
            this, [this](QProcess::ProcessError error) {
                if (error == QProcess::FailedToStart) {
                    modelFailed(
                        QStringLiteral("无法启动模型服务：\n")
                        + modelProcess->errorString()
                        );
                }
            });

    connect(modelProcess, &QProcess::finished,
            this, [this](int exitCode, QProcess::ExitStatus) {
                if (closing || modelHasFailed)
                    return;

                modelLog += QString::fromUtf8(
                    modelProcess->readAllStandardOutput()
                    );

                modelFailed(
                    QStringLiteral("模型服务已退出，退出码：%1\n\n%2")
                        .arg(exitCode)
                        .arg(modelLog.right(3000))
                    );
            });

    //创建定时器，父对象为当前窗口
    healthTimer = new QTimer(this);
    healthTimer->setInterval(1000);//每隔1秒检查一次

    //通过healthTimer定时器，每隔1秒调用checkModelReady()
    connect(healthTimer, &QTimer::timeout,
            this, &MainWindow::checkModelReady);//定时器触发时（即调用healthTimer->start()），调用检查函数checkModelReady

    //当模型进程成功启动，发出 QProcess::started 信号时，执行Lambda中的两条语句。
    //注意，执行connect()时只是建立连接，还不会执行里面的 healthTimer->start()。
    connect(modelProcess, &QProcess::started, this, [this]() {
        healthTimer->start();
        checkModelReady();
    });

    modelProcess->setProgram(serverPath);
    modelProcess->setArguments(QStringList{
        QStringLiteral("-m"), modelPath,
        QStringLiteral("--alias"), QStringLiteral("hy-mt"),
        QStringLiteral("--host"), QStringLiteral("127.0.0.1"),
        QStringLiteral("--port"), QStringLiteral("8080"),
        QStringLiteral("-c"), QStringLiteral("8192"),

        // 明确指定 RTX 5070
        QStringLiteral("--device"), QStringLiteral("Vulkan0"),

//尽可能把整个模型放入 RTX 5070 显存
        QStringLiteral("-ngl"), QStringLiteral("all"),

        QStringLiteral("--jinja")
    });

    statusBar()->showMessage(QStringLiteral("正在加载本地翻译模型……"));

    loadingClock.start();
    modelProcess->start();//启动llama-server.exe
/*进程成功启动后，才触发前面连接的回调
 * modelProcess->start()
    ↓
        llama-server.exe 进程成功启动
    ↓
          发出 started 信号
    ↓
          执行 Lambda
    ├─ healthTimer->start()：启动每秒一次的检查
    └─ checkModelReady()：立即检查一次
*/
    //进程成功启动，不代表模型已经加载完成，所以这时需要启动定时器，持续检查模型是否准备好。
}

void MainWindow::checkModelReady()
{
    if (closing || modelHasFailed || modelReady)
        return;

    if (loadingClock.elapsed() > 5 * 60 * 1000) {
        modelFailed(
            QStringLiteral("模型加载超过五分钟，请检查运行日志：\n\n")
            + modelLog.right(3000)
            );
        return;
    }

    if (healthChecking
        || modelProcess->state() != QProcess::Running) {
        return;
    }

    healthChecking = true;

    QNetworkRequest request{
        QUrl(QStringLiteral("http://127.0.0.1:8080/health"))
    };

    QNetworkReply *reply = networkManager->get(request);

    // 使用 reply 作为上下文，销毁 reply 后不会再执行此回调。
    QTimer::singleShot(2000, reply, [reply]() {
        if (!reply->isFinished())
            reply->abort();
    });

    connect(reply, &QNetworkReply::finished,
            this, [this, reply]() {
                healthChecking = false;

                const int httpStatus = reply->attribute(
                                                QNetworkRequest::HttpStatusCodeAttribute
                                                ).toInt();

                const bool success =
                    reply->error() == QNetworkReply::NoError
                    && httpStatus == 200;

                const QJsonObject data =
                    QJsonDocument::fromJson(reply->readAll()).object();

                reply->deleteLater();

                if (closing || modelHasFailed || modelReady)
                    return;

                if (modelProcess->state() != QProcess::Running)
                    return;

                if (success
                    && data.value("status").toString() == QStringLiteral("ok")) {
                    modelReady = true;
                    healthTimer->stop();

                    translateButton->setEnabled(true);
                    translateButton->setText(QStringLiteral("翻译"));

                    statusBar()->showMessage(
                        QStringLiteral("本地模型已就绪")
                        );
                }
            });
}

void MainWindow::modelFailed(const QString &message)
{
    if (closing || modelHasFailed)
        return;

    modelHasFailed = true;
    modelReady = false;

    if (healthTimer)
        healthTimer->stop();

    translateButton->setEnabled(false);
    translateButton->setText(QStringLiteral("模型不可用"));

    statusBar()->showMessage(QStringLiteral("模型服务不可用"));

    if (modelProcess
        && modelProcess->state() != QProcess::NotRunning) {
        modelProcess->kill();
    }

    QMessageBox::critical(
        this,
        QStringLiteral("模型服务错误"),
        message
        );
}
