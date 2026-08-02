#pragma execution_character_set("utf-8")

#include "frmemailtool.h"
#include <QApplication>
//#include <QTextCodec>
#include<QFont>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QFont font;
    font.setFamily("Microsoft Yahei");
    font.setPixelSize(13);
    a.setFont(font);//全局字体设置

/**#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#if _MSC_VER
    QTextCodec *codec = QTextCodec::codecForName("gbk");
#else
    QTextCodec *codec = QTextCodec::codecForName("utf-8");
#endif
    QTextCodec::setCodecForLocale(codec);
    QTextCodec::setCodecForCStrings(codec);
    QTextCodec::setCodecForTr(codec);
#else
    QTextCodec *codec = QTextCodec::codecForName("utf-8");
    QTextCodec::setCodecForLocale(codec);
#endif
**/
    frmEmailTool w;//实例化
   // w.setWindowTitle("邮件发送工具 V2021 (QQ: 517216493 WX: feiyangqingyun)");
    w.setWindowTitle("邮件发送工具");
    w.show();

    return a.exec();
//a.exec()是一个死循环，它不断检查“有没有鼠标点击？”，“有没有网络消息？”。程序能一直运行就靠这行代码
//只有关闭所有窗口、调用quit()/exit()后，exec()才返回退出码
//exec()必须放在show()之后：窗口创建完成并显示完成，再启动事件循环接收交互
}

/**
 *  其他说明：163邮箱和126邮箱，发送端口都是25，不使用SSL协议，而QQ邮箱必须使用SSL协议，
 *  端口为465。如果是QQ邮箱发送的话，前提要在QQ邮箱设置中将smtp协议开通，否则发送不成功，我就困在这里半个小时，结果收到QQ邮箱发过来的一封邮件，你妹啊，默认QQ邮箱没有开启SMTP服务。
 **/

// 该程序使用了线程（住线程：UI线程；子线程：SendEmailThread），如果不使用线程，结果就是：
// 在你点击“发送”按钮后的几秒钟内，整个软件界面完全冻结，无法最小化、无法关闭、无法输入任何内容，直到邮件发送完成或超时失败。

/****
 * 该程序的主线程：执行main()函数的线程，也是QApplication所在的线程，负责所有图形界面的渲染和用户交互响应，具体工作：
 * 显示窗口、按钮、输入框等控件
 * 响应用户的鼠标点击、键盘输入等操作
 * 执行frmEmailTool类中的所有槽函数。
 * 如果主线程给耗时操作（如网络请求、文件读写）阻塞，界面就会“假死”（无法拖动、按钮点不动）。
 ****/

/****子线程：由SendEmailThread类实例化并通过start()方法启动的独立线程，负责耗时的后台业务逻辑，具体工作：
 * 在run()函数中运行一个常驻循环while(!stopped)
 * 从任务队列(contents,fileNames)中获取待发送的邮件数据
 * 执行SMTP协议通信：连接服务器 (connectToHost)、登录认证 (login)、发送邮件 (sendMail)。
 *处理附件文件的读取和 MIME 编码。
 ****/
