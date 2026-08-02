#include "sendemailthread.h"
#include "smtpmime.h"

#pragma execution_character_set("utf-8")
#define TIMEMS qPrintable(QTime::currentTime().toString("hh:mm:ss zzz"))

QScopedPointer<SendEmailThread> SendEmailThread::self;//在类外初始化静态成员变量self
//QScopedPointer 是 Qt 的智能指针，当程序退出或对象销毁时，会自动 delete 指向的 SendEmailThread 对象，防止内存泄漏。

SendEmailThread *SendEmailThread::Instance()//单例。 邮件发送是持续服务。如果每次点发送都 new 一个线程，开销大且容易冲突。单例确保全局只有一个“邮递员”。
{
    if (self.isNull()) {//第一次检查】无锁快速判断，如果已有实例直接返回
        static QMutex mutex;//【静态互斥锁】保证多线程下只创建一个实例
        QMutexLocker locker(&mutex); // 【加锁】RAII 风格，出作用域自动解锁，防止死锁
        if (self.isNull()) {// 【第二次检查】防止多个线程同时通过第一次检查后重复创建。
            self.reset(new SendEmailThread);
        }//QMutexLocker 是一个专门用于管理 QMutex 的辅助类：它在构造时加锁，当执行到 QMutexLocker locker(&mutex); 这一行时，它的构造函数会立即调用 mutex.lock()
    }//析构时解锁：当 locker 对象离开当前作用域（即 if (self.isNull()) { ... } 这个花括号块结束）时，它的析构函数会被自动调用，而析构函数的内部实现就是 mutex.unlock()

    return self.data(); // 【返回裸指针】供外部调用成员函数。
}

SendEmailThread::SendEmailThread(QObject *parent) : QThread(parent)
{
    stopped = false;
    emialTitle = "邮件标题";
    //sendEmailAddr = "feiyangqingyun@126.com";
    sendEmailAddr = "2667438242@qq.com";//设置默认发件人
    sendEmailPwd = "123456789";
    //receiveEmailAddr = "feiyangqingyun@163.com;517216493@qq.com";
    receiveEmailAddr = "2667438242@qq.com";//默认收件人
    contents.clear();
    fileNames.clear();
}

SendEmailThread::~SendEmailThread()
{
    this->stop();
    this->wait(1000);
}

void SendEmailThread::run() //run()函数是QT自带的（继承自QThread）,是一个虚函数；但是需要我们重写，以此来实现我们的具体业务逻辑
{
    while (!stopped) {
        int count = contents.count();// 【检查队列】看看有没有待发送的任务。
        if (count > 0) {//有任务
            mutex.lock(); // 【加锁】保护共享数据，防止 UI 线程同时写入导致数据错乱。
            QString content = contents.takeFirst();// 【取出正文】从队列头部拿走任务
            QString fileName = fileNames.takeFirst();// 【取出附件路径】与正文一一对应
            mutex.unlock(); // 【解锁】尽快释放锁，减少 UI 线程等待时间

           // --- 以下为纯业务逻辑，完全在子线程执行，不卡界面 ---
            QString result;
            QStringList list = sendEmailAddr.split("@");//【解析域名】如 "user@qq.com"-> ["user", "qq.com"]，按@分割
            QString tempSMTP = list.at(1).split(".").at(0);//【提取服务商】，“qq.com”
            int tempPort = 25;// 【默认端口】普通 SMTP 端口

            //QQ邮箱端口号为465,必须启用SSL协议.
            if (tempSMTP.toUpper() == "QQ") {
                tempPort = 465;
            }

            //根据端口号决定连接类型
            SmtpClient smtp(QString("smtp.%1.com").arg(tempSMTP),
                            tempPort,
                            tempPort == 25 ? SmtpClient::TcpConnection : SmtpClient::SslConnection);
            smtp.setUser(sendEmailAddr);
            smtp.setPassword(sendEmailPwd);

            //构建邮件主题,包含发件人收件人附件等.
            MimeMessage message; //MineMessage 包含在smtpmime.h中
            message.setSender(new EmailAddress(sendEmailAddr));//发件人

            //逐个添加收件人
            QStringList receiver = receiveEmailAddr.split(';');//分号分割
            for (int i = 0; i < receiver.size(); i++)
            {//逐个添加
                message.addRecipient(new EmailAddress(receiver.at(i)));
            }

            //构建邮件标题
            message.setSubject(emialTitle);

            //构建邮件正文
            MimeHtml text;
            text.setHtml(content);//【设置富文本正文】保留颜色、加粗等格式
            message.addPart(&text);

            //构建附件-报警图像
            if (fileName.length() > 0) {
                QStringList attas = fileName.split(";");
                foreach (QString tempAtta, attas) {
                    QFile *file = new QFile(tempAtta);
                    if (file->exists())//校验文件是否存在，防止崩溃
                    {
                        message.addPart(new MimeAttachment(file));//添加到邮件
                    }
                }
            }

            if (!smtp.connectToHost())//连接服务器
            {
                result = "邮件服务器连接失败";
            }
            else
            {
                if (!smtp.login())//登录
                {
                    result = "邮件用户登录失败";
                }
                else//发送
                {
                    if (!smtp.sendMail(message))
                    {
                        result = "邮件发送失败";
                    }
                    else
                    {
                        result = "邮件发送成功";
                    }
                }
            }

            smtp.quit();//断开连接，释放网络资源
            if (!result.isEmpty()) {
                emit receiveEmailResult(result);//发射信号，通知UI层结果。跨线程通信
            }

            msleep(1000);//发送间隔，防止频繁发送被封号
        }

        msleep(100);//没任务时休息，避免CPU空转100%
    }

    stopped = false;//【重置标志】为下次启动做准备。
}

void SendEmailThread::stop()
{
    stopped = true;
}

void SendEmailThread::setEmailTitle(const QString &emailTitle)
{
    this->emialTitle = emailTitle;
}

void SendEmailThread::setSendEmailAddr(const QString &sendEmailAddr)
{
    this->sendEmailAddr = sendEmailAddr;
}

void SendEmailThread::setSendEmailPwd(const QString &sendEmailPwd)
{
    this->sendEmailPwd = sendEmailPwd;
}

void SendEmailThread::setReceiveEmailAddr(const QString &receiveEmailAddr)
{
    this->receiveEmailAddr = receiveEmailAddr;
}

void SendEmailThread::append(const QString &content, const QString &fileName)
{
    mutex.lock();
    contents.append(content);
    fileNames.append(fileName);
    mutex.unlock();
}
