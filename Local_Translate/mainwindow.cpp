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
    : QMainWindow(parent)//初始化窗口、网络管理器和信号槽
{
    setupUI();

    //初始化网络模块并禁用代理
    networkManager = new QNetworkAccessManager(this);
    networkManager->setProxy( QNetworkProxy(QNetworkProxy::NoProxy) );
    //程序后续需要向 http://127.0.0.1:8080（本地端口）发送翻译请求。
    //如果用户的电脑上开启了全局代理（如 VPN、Clash 等），Qt 默认可能会尝试通过代理去访问 127.0.0.1，这会导致请求被拦截或超时，直接翻译失败。
    //强制设为 NoProxy 确保了本地通信的绝对畅通。

    connect(translateButton,&QPushButton::clicked,this,&MainWindow::translate);//点击翻译按钮后，去执行translate函数

    //设置初始的UI状态
    translateButton->setEnabled(false);//程序刚打开，按钮不可点击
    translateButton->setText(QStringLiteral("模型加载中……"));

    // 窗口初始化完成后，再启动模型。延迟启动模型。singleShot 表示只触发一次，不会反复启动模型。
    //先安排任务，然后继续执行当前代码，稍后再调用 startModel()
    QTimer::singleShot(0, this, &MainWindow::startModel);
    //0：不设置等待间隔，等事件循环有机会时尽快执行
    //this: 当前窗口对象，也就是调用 startModel() 的对象
    //&MainWindow::startModel :指定要调用的成员函数，注意这里并没有立即调用它
    //它不会创建新线程。这里的 startModel() 仍然在窗口所在的主线程执行

    //如果是直接调用startModel()，会立即执行，执行完才能继续构造函数后面的代码.本例中，可以这样
}

MainWindow::~MainWindow()//退出时请求停止和模型进程
{
    closing = true;

    //如果定时器已经创建，就先停止它
    if (healthTimer)//判断指针是否非空，避免对空指针调用函数。
        healthTimer->stop();//停止定时器

    // 防止关闭期间的请求回调再次操作界面。
    if (networkManager) {
        const auto replies =
            networkManager->findChildren<QNetworkReply *>();
        //findChildren<QNetworkReply *>() 查找网络管理器下面仍然存在的响应对象。
        //它不仅可能找到正在进行的请求，也可能找到已经完成、但尚未删除的对象。

        for (auto *reply : replies) {
            QObject::disconnect(reply, nullptr, this, nullptr);//断开连接
            reply->abort();//强制终止
        }
    }

    // 仅停止本窗口创建的子进程。modelProcess是QProcess 对象，用来启动和管理外部的 llama-server.exe
    if (modelProcess) {
        QObject::disconnect(modelProcess, nullptr, this, nullptr);

        if (modelProcess->state() != QProcess::NotRunning) {
            modelProcess->kill();//强制杀死进程
            modelProcess->waitForFinished(3000);//主程序最多等待3秒，确认llama-server真的死透了，然后主程序才退出
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

    auto *mainLayout = new QVBoxLayout(central);//程序主布局为垂直布局,布局名称为mainLayout
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(12);//设置组件之间的最小间距，但不是固定死的间距

    auto *toolbar = new QFrame(central);//顶部工具栏面板
    toolbar->setObjectName(QStringLiteral("translationToolbar"));
    // 添加 QSS 样式：浅灰色背景 + 底部 1 像素的分割线
    toolbar->setStyleSheet(
        "QFrame#translationToolbar {"
        "   background-color: #f8f9fa;"        // 浅灰背景
        "   border-bottom: 1px solid #dcdfe6;" // 底部细线分割
        "   border-radius: 4px;"               // 可选：轻微圆角
        "}"
        );


    auto *toolbarLayout = new QHBoxLayout(toolbar);//顶部工具栏的布局为水平布局
    toolbarLayout->setContentsMargins(16, 12, 16, 12);
    toolbarLayout->setSpacing(12);//设置整个布局中，所有相邻组件之间的默认距离。调用一次，全局生效。

    auto *sourceLabel = new QLabel(QStringLiteral("源语言："), toolbar);//源语言文本标签，父对象为toolbar

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
    toolbarLayout->addSpacing(24);//在两组之间，插入一个较大的局部固定间距，组件拉开30像素
    toolbarLayout->addWidget(targetLabel);
    toolbarLayout->addWidget(targetLanguage);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(translateButton);

    // 左右文本区域。
    auto *splitter = new QSplitter(Qt::Horizontal, central);

    sourceText = new QPlainTextEdit;
    sourceText->setPlaceholderText( QStringLiteral("请输入需要翻译的文本……") );
    //QStringLiteral是Qt提供的一个“性能优化宏”，它的作用是告诉编译器：
    //在程序编译时就把这个字符串转换成Qt内部使用的格式，从而让程序在运行时“零开销”地使用它。
    //程序运行时，QStringLiteral 只是返回一个指向那块只读内存的指针，不分配堆内存，不进行任何编码转换。

    translatedText = new QPlainTextEdit;//这里没有指定父对象，可以translatedText = new QPlainTextEdit(splitter);
    translatedText->setPlaceholderText( QStringLiteral("译文将在这里显示……") );
    translatedText->setReadOnly(true);

    splitter->addWidget(sourceText);//addWidget 内部自动让它认 splitter 为父亲
    splitter->addWidget(translatedText);

    splitter->setChildrenCollapsible(false);//禁止把分割器里的子控件拖动到完全收起(不能把左边或右边的文本框拖到宽度为 0（即完全隐藏/消失))
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes(QList<int>{420, 420});//设置子组件的初始绝对像素大小为 420 和 420。

    //将两个子布局加入到主布局中，关键！
    mainLayout->addWidget(toolbar);//拉伸因子默认为0
    mainLayout->addWidget(splitter, 1);//拉伸因子为1，这里表示用户拉伸软件窗口时，由splitter组件填充扩大的部分
}

void MainWindow::translate()
{
    const QString text = sourceText->toPlainText();//获取输入的文本内容，保存到text

    if (!modelReady)
        return;

    if (text.trimmed().isEmpty()) {//排除空文本和只有空白字符的输入。
        QMessageBox::information(
            this,
            QStringLiteral("提示"),
            QStringLiteral("请先输入需要翻译的文本。")
            );
        return;
    }

    const QString source = sourceLanguage->currentText();//源语言下拉框的内容
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
        {"max_tokens", 4096}  //限制最大输出长度
    };

    QNetworkRequest request{
        QUrl(QStringLiteral(
            "http://127.0.0.1:8080/v1/chat/completions"
            ))
    };//请求发送到本机8080端口的翻译模型
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

    // 绑定超时逻辑：如果时间到了，就执行这里
    connect(timer, &QTimer::timeout, reply, [reply]() {
        reply->setProperty("timedOut", true);
        reply->abort();//强制掐断网络连接
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

    timer->start(5 * 60 * 1000);//启动定时器，5分钟
}
