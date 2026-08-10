#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QWidget>
#include <QElapsedTimer>      // 高精度计时器
#include <QRandomGenerator>   // Qt6 推荐的随机数类
#include "ui_widget.h"        // UI 头文件



QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget() override;

private:
    Ui::Widget *ui;
    QElapsedTimer m_timer;//成员变量
    bool m_isTesting=false; //状态标志，防止重复点击或提前点击；相当于锁
    bool m_colorChanged=false;//标记是否变色


private slots:
    void onStartClicked();//处理“开始测试”按钮点击
    void onPanelClicked(); //处理左侧色块点击





};
#endif // WIDGET_H
