#include "widget.h"
#include<QPushButton>
#include "./ui_widget.h" //这个头文件是qt自动生成的，找不到。当我们使用ui界面设计器拖曳按钮，
//Qt会在后台将其翻译成C++代码，并保存到./ui_widget.h中。
//一般情况，窗口的属性、添加控件和对控件的操作都会在类的构造函数中书写

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    /*** 这样一段是一次课程的代码
    ui->setupUi(this);//调用装修工具（ui）,把在.ui界面设计器里面画好的所有控件（按钮、文本框等），全部安装到当前这个窗口（this）上
    //如果没有ui->setupUi(this);这段代码，运行后只会看到一个白框，里面拖拽的控件都不会显示
    //任何涉及ui->xxx（操作UI界面上的控件）的代码，都必须在"ui->setupUi(this);"的后面
    //这里的this指的是：正在被创建的这一个Widget窗口实例

    this->setWindowTitle("第一个窗口"); //窗口标题
    //this->resize(500,500);//设置窗口大小，可拉伸
    this->setFixedSize(500,500);//设置窗口大小， 不可拉伸窗口
    ***/

     /*** 创建按钮的方式1
      **方式1窗口是默认大小，按钮显示在左上角
    QPushButton *button=new QPushButton;
    button->setParent(this);//设置按钮的父对象为窗口
    button->setText("第一个按钮");
    button->move(100,100);//设置按钮的显示位置
    button->setFixedSize(400,400);
    ****/

    /**** 创建按钮的方式2
     **方式2窗口是根据按钮的大小来创建的
    QPushButton *button2=new QPushButton("第二个按钮",this);
    this->resize()
    ***/


}
//Widget(QWidget *parent)：这是构造函数，parent参数代表“父窗口”，在qt中，如果父窗口关闭了，子窗口会自动跟着关闭并释放内存
//:QWidget(parent)初始化列表。意思是：“我（Widget）首先是一个基础的 QWidget 窗口，并且我认 parent 做我的爸爸
//ui(new Ui::Widget)：在内存中实例化（new）一个UI对象。可以理解为准备了一套装修工具

Widget::~Widget() //析构函数
{
    delete ui;
}
