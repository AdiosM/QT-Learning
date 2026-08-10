#ifndef MAINWINDOW_H //防止同一个文件被重复包含多次，导致编译器报错。
#define MAINWINDOW_H

#include <QMainWindow>
#include<QHBoxLayout>
#include<QGroupBox>
#include<QPushButton> //按钮的定义文件
#include<QRadiobutton> //写成qradioButton也能编译通过，现在编译器能兼容大小写了
#include<QCheckBox>
#include<QVBoxLayout>
#include<QLineEdit>
#include<QTextEdit>
#include<QComboBox>
#include<QDial>
#include<QSlider>

//mainwindow.h文件是这个软件的“蓝图”或“清单”，
//它告诉编译器：我的主窗口（MainWindow）里有哪些“零件”（控件），以及这些零件之间是怎么组装的（布局）

class MainWindow : public QMainWindow
{
    Q_OBJECT
    //QT中的一个宏，作用是启用Qt的元对象系统。
    //任何继承自QObject的类，如果想使用Qt的高级特性，都必须在类的私有部分声明这个宏。

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    QWidget *centralWidget;//QMainWindow 比较特殊，它不能直接放按钮，必须先创建一个QWidget作为“中央部件” (centralWidget)。所有的控件都要放在这个中央部件上。

    QHBoxLayout *hbox;//水平布局管理器。它的作用是把下面的三个大组（按钮组、文本组、其他组）从左到右并排摆放。

    //第一组：按钮区
    QGroupBox *buttonGroup;//一个带标题的边框盒子，用来把相关的控件框在一起，显得整齐。
    QPushButton *pushButton;
    QRadioButton *radioButton;//单选框
    QCheckBox *checkBox;//复选框
    QVBoxLayout *vboxButtonGroup;//垂直布局。它会把上面的按钮、单选框、复选框从上到下排列在 buttonGroup 盒子里。

    //第二组：文本输入区
    QGroupBox *textInputGroup;//第二个边框盒子。
    QLineEdit *lineEdit;//单行输入框（比如输入用户名，不能换行）。
    QTextEdit *textEdit;//多行文本编辑器（比如写日记，可以换行、滚动）。
    QVBoxLayout *vboxtextInputGroup;//垂直布局，把这两个文本框上下排列。

    //第三组：其他控制区
    QGroupBox *otherInputGroup;//第三个边框盒子。
    QComboBox *comboBox;//下拉列表框（点击出现选项列表）
    QDial *dial;//圆形旋钮（像老式收音机的音量调节）
    QSlider *slider;//滑动条（像视频播放器的进度条）。
    QVBoxLayout *vboxOtherInputGroup;//垂直布局，把这三个控件上下排列。

};
#endif // MAINWINDOW_H
