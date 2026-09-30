#include "mainwindow.h"

// --- 界面与对话框 (Widgets & Dialogs) ---
#include <QComboBox>
#include <QFrame>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QLabel>
#include <QHBoxLayout>

// --- 网络 (Network) ---
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>

// --- JSON 处理 (JSON) ---
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

// --- 核心与工具类 (Core & Utilities) ---
#include <QList>
#include <QProcess>
#include <QStringList>
#include <QTimer>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    networkManager = new QNetworkAccessManager(this);
    networkManager->setProxy( QNetworkProxy(QNetworkProxy::NoProxy) );

    connect(translateButton,&QPushButton::clicked,this,&MainWindow::translate);

    translateButton->setEnabled(false);
    translateButton->setText(QStringLiteral("模型加载中……"));

    // 窗口初始化完成后，再启动模型。
    QTimer::singleShot(0, this, &MainWindow::startModel);
}

MainWindow::~MainWindow()
{
    closing = true;

    if (healthTimer)
        healthTimer->stop();

    // 防止关闭期间的请求回调再次操作界面。
    if (networkManager) {
        const auto replies =
            networkManager->findChildren<QNetworkReply *>();

        for (auto *reply : replies) {
            QObject::disconnect(reply, nullptr, this, nullptr);
            reply->abort();
        }
    }

    // 仅停止本窗口创建的子进程。
    if (modelProcess) {
        QObject::disconnect(modelProcess, nullptr, this, nullptr);

        if (modelProcess->state() != QProcess::NotRunning) {
            modelProcess->kill();
            modelProcess->waitForFinished(3000);
        }
    }
}

void MainWindow::setupUI()
{
    setWindowTitle("Local_Translate");
    resize(1100, 720);
    setMinimumSize(900, 600);

    // QMainWindow 的中心控件。
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *mainLayout = new QVBoxLayout(central);//程序主布局为垂直布局
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(12);

    auto *toolbar = new QFrame(central);//顶部工具栏面板
    toolbar->setObjectName(QStringLiteral("translationToolbar"));

    auto *toolbarLayout = new QHBoxLayout(toolbar);//顶部工具栏的布局为水平布局
    toolbarLayout->setContentsMargins(16, 12, 16, 12);
    toolbarLayout->setSpacing(12);

    auto *sourceLabel = new QLabel(QStringLiteral("源语言："), toolbar);//源语言文本标签

    sourceLanguage = new QComboBox(toolbar);
    sourceLanguage->addItems(QStringList{
        QStringLiteral("自动检测"),
        QStringLiteral("中文"),
        QStringLiteral("英语"),
        QStringLiteral("日语"),
        QStringLiteral("韩语")
    });
    sourceLanguage->setMinimumWidth(120);

    auto *targetLabel = new QLabel(QStringLiteral("目标语言："), toolbar);//目标语言标签
    targetLanguage = new QComboBox(toolbar);
    targetLanguage->addItems(QStringList{
        QStringLiteral("中文"),
        QStringLiteral("英语"),
        QStringLiteral("日语"),
        QStringLiteral("韩语")
    });
    targetLanguage->setMinimumWidth(120);

    translateButton = new QPushButton(QStringLiteral("翻译"), toolbar);
    translateButton->setMinimumSize(90, 36);

    //将上面的几个元素（源语言、目标语言，按钮）加入到水平布局中
    toolbarLayout->addWidget(sourceLabel);
    toolbarLayout->addWidget(sourceLanguage);
    toolbarLayout->addSpacing(24);
    toolbarLayout->addWidget(targetLabel);
    toolbarLayout->addWidget(targetLanguage);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(translateButton);

    // 左右文本区域。
    auto *splitter = new QSplitter(Qt::Horizontal, central);

    sourceText = new QPlainTextEdit;
    sourceText->setPlaceholderText( QStringLiteral("请输入需要翻译的文本……") );

    translatedText = new QPlainTextEdit;
    translatedText->setPlaceholderText( QStringLiteral("译文将在这里显示……") );
    translatedText->setReadOnly(true);

    splitter->addWidget(sourceText);
    splitter->addWidget(translatedText);

    splitter->setChildrenCollapsible(false);//禁止把分割器里的子控件拖动到完全收起
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes(QList<int>{420, 420});

    mainLayout->addWidget(toolbar);
    mainLayout->addWidget(splitter, 1);
}

void MainWindow::translate()
{
    const QString text = sourceText->toPlainText();

    if (!modelReady)
        return;

    if (text.trimmed().isEmpty()) {
        QMessageBox::information(
            this,
            QStringLiteral("提示"),
            QStringLiteral("请先输入需要翻译的文本。")
            );
        return;
    }

    const QString source = sourceLanguage->currentText();
    const QString target = targetLanguage->currentText();

    QString prompt;

    if (source != QStringLiteral("自动检测")) {
        prompt += QStringLiteral("源语言为%1。\n").arg(source);
    }

    prompt += QStringLiteral(
                  "将以下文本翻译为%1，"
                  "注意只需要输出翻译后的结果，不要额外解释：\n\n"
                  ).arg(target);

    prompt += text;

    QJsonArray messages;
    messages.append(QJsonObject{
        {"role", "user"},
        {"content", prompt}
    });

    QJsonObject body{
        {"model", "hy-mt"},
        {"messages", messages},
        {"stream", false},
        {"temperature", 0.7},
        {"top_p", 0.6},
        {"top_k", 20},
        {"repeat_penalty", 1.05},
        {"max_tokens", 4096}
    };

    QNetworkRequest request{
        QUrl(QStringLiteral(
            "http://127.0.0.1:8080/v1/chat/completions"
            ))
    };

    request.setHeader(
        QNetworkRequest::ContentTypeHeader,
        QStringLiteral("application/json")
        );

    translateButton->setEnabled(false);
    translateButton->setText(QStringLiteral("翻译中……"));

    // 固定本次输入，避免等待时修改内容造成结果对应不清。
    sourceText->setReadOnly(true);
    sourceLanguage->setEnabled(false);
    targetLanguage->setEnabled(false);
    translatedText->clear();

    QNetworkReply *reply = networkManager->post(
        request,
        QJsonDocument(body).toJson(QJsonDocument::Compact)
        );

    // 整个请求最多等待 5 分钟，可按机器性能调整。
    auto *timer = new QTimer(reply);
    timer->setSingleShot(true);

    connect(timer, &QTimer::timeout, reply, [reply]() {
        reply->setProperty("timedOut", true);
        reply->abort();
    });

    connect(reply, &QNetworkReply::finished,
            this, [this, reply, timer]() {
                timer->stop();

                translateButton->setEnabled(modelReady);
                translateButton->setText(
                    modelReady?QStringLiteral("翻译"):QStringLiteral("模型不可用"));

                sourceText->setReadOnly(false);
                sourceLanguage->setEnabled(true);
                targetLanguage->setEnabled(true);

                const QByteArray response = reply->readAll();
                const int status = reply->attribute(
                                            QNetworkRequest::HttpStatusCodeAttribute
                                            ).toInt();

                const bool timedOut =
                    reply->property("timedOut").toBool();

                const auto networkError = reply->error();
                const QString errorText = reply->errorString();

                reply->deleteLater();

                if (timedOut) {
                    QMessageBox::warning(
                        this,
                        QStringLiteral("翻译超时"),
                        QStringLiteral("本地模型响应超时，请缩短文本后重试。")
                        );
                    return;
                }

                if (networkError != QNetworkReply::NoError
                    || status < 200 || status >= 300) {
                    QString detail = errorText;

                    if (!response.isEmpty()) {
                        detail += QStringLiteral("\n\n")
                        + QString::fromUtf8(response).left(1500);
                    }

                    QMessageBox::warning(
                        this,
                        QStringLiteral("请求失败"),
                        QStringLiteral("HTTP 状态：%1\n%2")
                            .arg(status)
                            .arg(detail)
                        );
                    return;
                }

                QJsonParseError parseError;
                const QJsonDocument document =
                    QJsonDocument::fromJson(response, &parseError);

                if (parseError.error != QJsonParseError::NoError
                    || !document.isObject()) {
                    QMessageBox::warning(
                        this,
                        QStringLiteral("解析失败"),
                        QStringLiteral("本地服务返回的内容不是有效的 JSON 对象。")
                        );
                    return;
                }

                const QJsonArray choices =
                    document.object().value("choices").toArray();

                if (choices.isEmpty()) {
                    QMessageBox::warning(
                        this,
                        QStringLiteral("翻译失败"),
                        QStringLiteral("服务没有返回翻译结果。")
                        );
                    return;
                }

                const QJsonObject choice = choices.at(0).toObject();

                const QString result = choice.value("message")
                                           .toObject()
                                           .value("content")
                                           .toString();

                if (result.trimmed().isEmpty()) {
                    QMessageBox::warning(
                        this,
                        QStringLiteral("翻译失败"),
                        QStringLiteral("模型返回了空译文。")
                        );
                    return;
                }

                translatedText->setPlainText(result);

                if (choice.value("finish_reason").toString()
                    == QStringLiteral("length")) {
                    QMessageBox::warning(
                        this,
                        QStringLiteral("译文可能不完整"),
                        QStringLiteral(
                            "已达到生成长度限制，请将原文拆成较短的段落后重试。"
                            )
                        );
                }
            });

    timer->start(5 * 60 * 1000);
}
