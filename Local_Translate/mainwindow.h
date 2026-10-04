#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QElapsedTimer>
#include <QString>

class QComboBox;
class QPushButton;
class QPlainTextEdit;
class QNetworkAccessManager;

class QProcess;
class QTimer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void setupUI();//程序UI，创建控件并安排布局
    void translate();//翻译。发送翻译请求、处理响应和显示结果

    void startModel();//查找服务和模型路径，检测端口，启动llama-server
    void checkModelReady();//请求本地/health接口，确认模型是否加载完成
    void modelFailed(const QString &message);//统一处理模型服务故障，更新状态、禁用按钮并显示错误

    //声明组件的原则：
    //只有在其他函数中需要再次访问的组件，才需要声明为类成员变量。
    //这里没有声明QLabel组件，而是在mainwindow.cpp中声明和定义。因为它只会在ui界面中被创造，后续不会有任何函数或类去访问或读取它

    QComboBox *sourceLanguage = nullptr;//源语言
    QComboBox *targetLanguage = nullptr;//翻译的目标语言

    QPushButton *translateButton = nullptr;//翻译按钮

    QPlainTextEdit *sourceText = nullptr;//源语言输入面板
    QPlainTextEdit *translatedText = nullptr;//翻译输出面板

    QNetworkAccessManager *networkManager = nullptr;//一个网络请求管理器指针，用于让Qt程序通过HTTP与本地模型服务通信
    //主要负责两件事：
    //networkManager->get(request)：	请求 /health，检查模型是否加载完成
    //networkManager->post(request, data)：	把原文和翻译要求发送到模型接口

    QProcess *modelProcess = nullptr;//管理外部的llama-server.exe进程，用于：
    //1。启动模型服务并传入参数。 2. 获取进程运行状态。
    //3. 读取进程输出。 4. 接收启动失败和退出通知。 5. 在软件关闭时结束对应进程。

    QTimer *healthTimer=nullptr;//定时检查模型是否加载完成的计时器指针，在mainwindow_model.cpp中使用

    QElapsedTimer loadingClock;//测量模型加载已经经过了多次时间
    //loadingClock.start(); 记录起点
    //loadingClock.elapsed(); 获取经过的毫秒数

    QString modelLog;//保存模型进程输出的日志文本，主要用于发生错误时提供详细信息

    bool modelReady = false; //模型是否加载就绪；用途：决定是否允许翻译
    bool healthChecking = false; //true表示已有一次健康检查请求尚未处理完成；用途：避免定时器重复发出重叠请求
    bool modelHasFailed = false;//true表示模型服务已经发生故障；用途：用于避免重复报错，并阻止后续就绪处理
    bool closing = false; //true表示窗口对象正执行退出清理；用途：防止清理期间继续处理模型错误和更新界面


};
#endif // MAINWINDOW_H
