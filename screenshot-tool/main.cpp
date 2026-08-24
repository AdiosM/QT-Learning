// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QScreen>

#include "screenshot.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);//每个 Qt GUI 程序必须且只能有一个 QApplication 对象。它负责初始化图形系统、处理事件分发等底层工作。

    Screenshot screenshot; //实例化对象，会调用它的构造函数
    screenshot.move(screenshot.screen()->availableGeometry().topLeft() + QPoint(20, 20));//无论你的任务栏在哪里，窗口都会出现在屏幕左上角附近，且不会被任务栏遮挡，同时留出了一点边距，
//move()是QWiget类的一个成员函数，功能是移动窗口部件到屏幕上的指定位置
//screenshot.screen(): 获取当前窗口所在的物理屏幕对象。如果是多屏环境，它会识别窗口属于哪个显示器。
//->availableGeometry(): 获取该屏幕的可用区域。与 geometry() 不同，它会排除任务栏、Dock 栏或菜单栏占用的空间。
//.topLeft(): 获取可用区域的左上角坐标（通常是 (0, 0)，但如果任务栏在顶部，y 坐标会下移）
//+ QPoint(20, 20): 在左上角的基础上向右、向下各偏移 20 像素。
    screenshot.show();//将窗口从隐藏状态变为可见状态

    return app.exec();//将控制权交给 Qt 的事件循环。
//程序现在处于“待命”状态，等待用户的鼠标点击、键盘输入或窗口拉伸等操作。如果没有这一行，程序运行完 show() 后会立即退出，用户根本来不及看界面
}
