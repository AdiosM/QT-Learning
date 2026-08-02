#ifndef SENDEMAILTHREAD_H
#define SENDEMAILTHREAD_H

#include <QThread>
#include <QMutex>
#include <QStringList>
/***
 * sendemailthread.h 是这个项目的核心引擎头文件。它定义了一个基于单例模式的后台线程类，
 * 负责将耗时的邮件发送任务从主界面（UI）剥离出来，防止程序在发送邮件时出现“假死”现象。
 * ***/


class SendEmailThread : public QThread //继承QThread，这是Qt中创建线程最经典的方式之一；这意味着SendEmailThread的实例本身就是一个独立的线程对象
{
	Q_OBJECT
public:
    static SendEmailThread *Instance(); //静态成员函数；是后台线程的唯一入口（双重检查锁实现），所有UI界面都通过这个静态门把手来操控背后的发送邮件引擎
//可以把静态函数理解为全局工具或类的入口
//Instace()返回指针而不是对象本身，如果Instance() 返回的是对象值（SendEmailThread），每次调用都会触发拷贝构造函数，在栈上创建一个副本
//如果返回对象值，我们拿到的就是一个副本，对他的修改就不会影响到原始单例；
//此外QThread及其父类QObject禁止拷贝，如果返回对象，编译器会报错
//返回指针也告诉我们只是“获取”，而非“创建”。即“这个对象已经存在，我只是给你它的访问权”

    explicit SendEmailThread(QObject *parent = 0);//构造函数私有化或受保护，强制使用单例获取
    ~SendEmailThread();

protected:
	void run();

private:
    static QScopedPointer<SendEmailThread> self; // 静态智能指针，持有唯一实例。当self离开作用域（对于静态成员变量，即程序退出时），它会自动调用delete销毁所管理的SendEmailThread对象
    //QScopedPointer是Qt框架自带的智能指针类。QScopedPointer与C++11标准库中的std::unique_ptr几乎完全一致
    QMutex mutex; //互斥锁
	volatile bool stopped;

	QString emialTitle;         //邮件标题
	QString sendEmailAddr;      //发件人邮箱
	QString sendEmailPwd;       //发件人密码
	QString receiveEmailAddr;   //收件人邮箱,可多个中间;隔开
	QStringList contents;       //正文内容
    QStringList fileNames;      //附件路径

signals:
    void receiveEmailResult(const QString &result);//跨线程反馈信号

public slots:    
    void stop();//用于修改标志位stopped；配合run()中的while(！stopped）实现退出，而非强制终止。

    //----配置参数接口Setters-----
    void setEmailTitle(const QString &emailTitle);
    void setSendEmailAddr(const QString &sendEmailAddr);
    void setSendEmailPwd(const QString &sendEmailPwd);
    void setReceiveEmailAddr(const QString &receiveEmailAddr);
    //-----这些 Setter 没有加锁，依赖于调用顺序保证（必须在 append() 之前调用）。在当前 UI 交互流程下是安全的。---------

    void append(const QString &content, const QString &fileName);//任务投递接口。内部加锁后将数据追加到队列，瞬间返回，绝不阻塞 UI
//参数说明：content:支持HTML富文本（由UI层toHtml（）传入）；filename:多个附件路径用英文分号 ; 分隔的字符串后，台线程会自行 split(";") 解析。
//生产者-消费者模型：UI 线程是“生产者”，通过 append() 往队列塞数据；后台线程是“消费者”，在 run() 中取数据发送。
};

#endif // SENDEMAILTHREAD_H
