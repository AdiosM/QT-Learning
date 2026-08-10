#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include<QMediaPlayer>
#include<QVideoWidget>
#include <QImageCapture> // 1. 引入拍照类
#include <QCamera>       // 2. 引入相机类（拍照必须依赖相机对象）

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

private slots:
    void onImageCaptured(int id,const QImage &image);//当照片捕获完成时调用

private:
    Ui::Widget *ui;
    QMediaPlayer *player;//声明播放器指针

    QCamera *camera; //声明相机指针
    QImageCapture *imageCapture; //声明拍照指针
};
#endif // WIDGET_H
