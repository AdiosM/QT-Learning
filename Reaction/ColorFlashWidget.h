#ifndef COLORFLASHWIDGET_H
#define COLORFLASHWIDGET_H
#pragma once

#include <QWidget>
#include <QColor>
#include <QPaintEvent>
#include <QMouseEvent>
#include<QPixmap>

class ColorFlashWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ColorFlashWidget(QWidget *parent = nullptr);

    // 外部调用此方法触发颜色变化
    void flash(const QColor &color);

    // 给左侧颜色变化的色块设置初始颜色
    void reset();

    //设置反馈文字
    void setFeedbackText(const QString &text);


signals:
    // 用户点击时发出此信号，供主窗口连接计时逻辑
    void panelClicked();


protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    QColor m_currentColor;
};
#endif // COLORFLASHWIDGET_H
