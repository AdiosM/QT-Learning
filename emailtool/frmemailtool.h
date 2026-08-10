#ifndef FRMEMAILTOOL_H
#define FRMEMAILTOOL_H

#include <QWidget>

namespace Ui
{
class frmEmailTool;//前置声明
//在 namespace Ui 中只声明了类，没有包含具体的 .h 文件。这是为了减少编译依赖。
//如果直接 #include "ui_frmemailtool.h"，每次修改 UI 布局都会导致所有包含此头文件的 cpp 重新编译
}

class frmEmailTool : public QWidget
{
	Q_OBJECT

public:
    explicit frmEmailTool(QWidget *parent = 0);
    ~frmEmailTool();

private:
    Ui::frmEmailTool *ui; //这是 Qt Creator 自动生成的标准模式。
//所有的按钮、输入框、标签等控件都封装在这个 ui 指针指向的对象里。
//在.cpp 文件中通过 ui->btnSend 来访问控件，保持了头文件的整洁。

private:
    bool check();//在点击发送前，强制校验必填项（发件人、密码、收件人、标题）。

private slots:
    void initForm();//初始化设置，构造函数中调用。设置下拉框默认值、连接信号槽、启动后台线程
    void receiveEmailResult(QString result);//受到线程信号时触发；结果反馈，接收后台线程发来的“成功/失败”消息

private slots:
    void on_btnSend_clicked();//业务触发器。它不做具体的网络请求，只做两件事：①校验数据(check())；②把参数传给单例线程(append())。
    void on_btnSelect_clicked();//文件选择；调用QFileDialog获取文件路径
	void on_cboxServer_currentIndexChanged(int index);
};

#endif // FRMEMAILTOOL_H
