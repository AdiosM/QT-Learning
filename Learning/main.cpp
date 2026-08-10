#include "widget.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);//应用程序类（整个后台管理的命脉，处理应用程序的初始化和结束，事件处理调度，注意不管有多少窗口，一个QApplication类就可）
    Widget w;//实例化对象，调用构造函数
    w.show(); //显示图形界面

    return a.exec();//主事件循环，在exec函数中，Qt接受并处理用户和系统的事件，并将它们传递给适当的窗口控件（按钮、窗口...）
}
