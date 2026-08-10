#include "widget.h"
#include<QPushButton>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    //ctrl+i自动对其代码
/*** 使用connect函数系统自带的信号和槽
    QPushButton *button = new QPushButton("点击关闭窗口",this);
    this->resize(600,400);

    connect(button,&QPushButton::clicked,this,&QWidget::close);//使用connect函数系统自带的信号和槽
    //信号发出者：按钮（button）
    //信号：点击按钮（clicked）
    //信号的接收者：窗口（QWidget下的类）
    //这个信号的任务（功能）：关闭窗口（调用这个窗口的槽函数，因为窗口的QWidget下的类，所以要用域解析符）
    //先确定接收信号的对象是哪个类下面的，然后在调用这个类下面的槽函数
***/

    //自定义槽函数,方式1
    QPushButton *button = new QPushButton("点击",this);
    this->resize(600,400);
    this->teacherA=new Teacher();
    this->stu=new Student();
    connect(teacherA,&Teacher::hungry,stu,&Student::treat);
    //容易写成：connect(teacherA, &Teacher::hungry(), stu, &Student::treat());
    //给hungry加括号()，表示立即执行hungry()函数，但是hungry()不是静态函数，必须通过对象调用，会报错！
    ClassOver(); //手动触发信号


/*** 自定义槽函数2，方式2
    QPushButton *button = new QPushButton("点击",this);
    this->resize(600,400);
    this->teacherA=new Teacher();
    this->stu=new Student();
    connect(button,&QPushButton::clicked,teacherA,&Teacher::hungry);//点击按钮，老师发出饥饿(hungry)的信号
    connect(teacherA,&Teacher::hungry,stu,&Student::treat);//老师发出饥饿信号，学生受到信号后触发treat函数
***/

/***
    //方式3
    QPushButton *button = new QPushButton("点击",this);
    this->resize(600,400);
    this->teacherA=new Teacher(this);//指定父对象为当前窗口，这样关闭窗口就会自动释放内存
    this->stu=new Student(this);
    connect(button,&QPushButton::clicked,stu,&Student::treat);
***/
}
void Widget::ClassOver()
{
    emit teacherA->hungry();
}

Widget::~Widget()
{

}
