// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SCREENSHOT_H
#define SCREENSHOT_H
//这两行是C++标准写法，作用是防止同一个头文件被多次包含
//例如，如果 main.cpp 和另一个文件都包含了它），从而避免编译错误。


#include <QPixmap>
#include <QWidget>

QT_BEGIN_NAMESPACE //前置声明
class QCheckBox;
class QGridLayout;
class QGroupBox;
class QHBoxLayout;
class QLabel;
class QPushButton;
class QSpinBox;
class QVBoxLayout;
QT_END_NAMESPACE
//使用前置声明而不是 #include <QCheckBox> 等，可以显著减少编译依赖。
//如果 QCheckBox 的头文件发生了变化，只要它的指针大小没变，你的 screenshot.h 就不需要重新编译，这能极大提高大型项目的构建速度。

/*** //![0]是QT的代码片段标记注释，用于文档自动化截取代码。[0]是片段编号 ***/
//! [0]
class Screenshot : public QWidget
{//继承QWidget类，说明是一个窗口部件
    Q_OBJECT

public:
    Screenshot(); //默认构造函数，声明，定义在screenshot.cpp中

protected:
    void resizeEvent(QResizeEvent *event) override;//事件处理函数
//当用户改变窗口大小时，Qt 会自动调用它。这里对其进行了重载，是为了让图片在窗口缩放时也能保持比例

private slots: //只能在类内部或通过信号连接被调用(private),保证了封闭性
    void newScreenshot();//倒计时逻辑
    void saveScreenshot();//保存图片
    void shootScreen();//截图动作
    void updateCheckBox();

private:
    void updateScreenshotLabel(); //该类提供的一个函数，用来访问原始图片或修改标签(因为内部成员函数是private)

    QPixmap originalPixmap; //一个值类型变量，用来存储截取到的屏幕图像数据
//QPixmap 是 Qt 中用于处理图像数据的类。它专为在屏幕上显示而优化，通常存储在显存（显卡内存）中，因此绘制速度非常快。
    QLabel *screenshotLabel; //UI控件，下面的也是。QLabel控件是用于展示文本或图片，还能为其他控件设置焦点快捷键.这里是展示截图
    QSpinBox *delaySpinBox;//上下调节数字大小
    QCheckBox *hideThisWindowCheckBox;
    QPushButton *newScreenshotButton;
//可以发现，这里创建的大部分都是指针，因为当你创建一个控件并指定父对象时（例如 new QLabel(this)），这个子对象会被添加到父对象的“孩子列表”中。
//自动销毁：当父对象（比如你的 Screenshot 窗口）被销毁时，它会自动遍历并销毁所有的子对象。
//为什么用指针：这种动态创建和销毁的过程必须在堆（Heap）上进行，而堆上的对象只能通过指针来访问。如果你直接在类里声明 QLabel screenshotLabel;（栈对象），Qt 就无法通过对象树来管理它的生命周期了
//Qt 的一些大型对象（如 QWidget、QPixmap 等）如果作为值传递或赋值，可能会涉及大量的数据拷贝，这会降低程序性能。
};
//! [0]

#endif // SCREENSHOT_H
