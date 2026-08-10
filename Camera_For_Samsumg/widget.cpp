#include "widget.h"
#include "ui_widget.h"

#include<QUrl>
#include<QDebug>
#include<QFile>
#include <QMediaCaptureSession> // 核心：会话管理器
#include <QStandardPaths>   // 用于获取标准路径
#include <QDateTime>        // 用于生成时间戳文件名
#include <QDir>

// Android 专用头文件 (仅在编译 Android 时生效)
#ifdef Q_OS_ANDROID
#include <QJniObject>                         // 替代旧的 QAndroidJniObject
#include <QJniEnvironment>                    // JNI 环境
#endif

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
/***
    player=new QMediaPlayer(this);//创建播放器实例
    player->setVideoOutput(ui->Camera_frame);
    QString videoPath = "E:/桌面壁纸/test.mp4";
    // 检查文件是否存在，防止静默失败
    if (!QFile::exists(videoPath)) {
        qDebug() << "错误：找不到视频文件，请检查路径！" << videoPath;
        // 为了演示，如果没有视频，我们可以尝试播放一个在线的或者提示用户
    } else {
        player->setSource(QUrl::fromLocalFile(videoPath));
        player->play(); // 开始播放
        qDebug() << "开始播放视频...";
    }
***/
    // --- 1. 初始化组件 ---
    player = new QMediaPlayer(this);
    camera = new QCamera(this);
    imageCapture = new QImageCapture(this);

    // --- 2. 建立“会话” (关键步骤) ---
    // Qt 6 使用 QMediaCaptureSession 把相机、拍照器、播放器绑在一起
    QMediaCaptureSession *session = new QMediaCaptureSession(this);
    session->setCamera(camera);
    session->setImageCapture(imageCapture);
    session->setVideoOutput(ui->Camera_frame); // 绑定到你的视频控件

    // --- 3. 连接信号槽：监听照片是否拍好 ---
    connect(imageCapture, &QImageCapture::imageCaptured,
            this, &Widget::onImageCaptured);

    // --- 4. 启动相机预览 ---
    // 注意：即使没有真相机，这行代码也能让之前的测试视频继续播放（如果配置正确）
    // 如果有真相机，它会打开摄像头画面
    camera->start();

    // --- 5. 绑定按钮点击事件 ---
    // 假设你的按钮叫 pushButton
    connect(ui->pushButton, &QPushButton::clicked, this, [this]() {
        qDebug() << "正在拍照...";
        // 执行拍照动作
        imageCapture->capture();
    });
}

// --- 6. 处理拍到的照片 ---
void Widget::onImageCaptured(int id, const QImage &image)
{
    qDebug() << "照片已捕获! ID:" << id;

    if (image.isNull()) {
        qDebug() << "图片为空!";
        return;
    }

    // 1. 获取 Android 公共图片目录 (通常是 /storage/emulated/0/Pictures)
    QString saveDir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);

    // 如果获取失败，回退到应用私有目录
    if (saveDir.isEmpty()) {
        saveDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    }

    // 2. 生成唯一文件名 (避免覆盖)
    QString fileName = "CAM_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".jpg";
    QString fullPath = saveDir + "/" + fileName;

    qDebug() << "正在保存到:" << fullPath;

    // 3. 保存图片
    if (image.save(fullPath, "JPG", 90)) {
        qDebug() << "✅ 保存成功!";

// 4. 【关键】通知安卓系统刷新媒体库 (否则相册看不到)
// 这需要用到 JNI 调用安卓原生的 MediaScannerConnection
#ifdef Q_OS_ANDROID
        // 1. 创建媒体扫描 Intent
        QJniObject mediaScanIntent("android/content/Intent",
                                   "(Ljava/lang/String;)V",
                                   QJniObject::fromString("android.intent.action.MEDIA_SCANNER_SCAN_FILE").object<jstring>());

        // 2. 【关键修正】显式创建 java.io.File 对象
        // 必须先指定类名 "java/io/File" 和构造签名 "(Ljava/lang/String;)V"
        QJniObject jFile("java/io/File",
                         "(Ljava/lang/String;)V",
                         QJniObject::fromString(fullPath).object<jstring>());

        // 3. 将 File 对象转换为 URI
        QJniObject fileUri = QJniObject::callStaticObjectMethod(
            "android/net/Uri", "fromFile",
            "(Ljava/io/File;)Landroid/net/Uri;",
            jFile.object());

        // 4. 将 URI 设置到 Intent 中
        mediaScanIntent.callObjectMethod("setData",
                                         "(Landroid/net/Uri;)Landroid/content/Intent;",
                                         fileUri.object());

        // 5. 【关键修正】正确获取 Activity 并发送广播
        // context() 返回的是原生句柄，需要先包装成 QJniObject
        auto activityContext = QNativeInterface::QAndroidApplication::context();

        // 使用 jobject 构造函数创建可调用方法的 QJniObject
        QJniObject activity(activityContext);

        if (activity.isValid()) {
            activity.callMethod<void>("sendBroadcast",
                                      "(Landroid/content/Intent;)V",
                                      mediaScanIntent.object());
            qDebug() << "✅ 已发送相册刷新广播";
        } else {
            qDebug() << "⚠️ 无法获取 Activity 上下文";
        }
#endif

    } else {
        qDebug() << "❌ 保存失败! 请检查 WRITE_EXTERNAL_STORAGE 权限";
    }

}

Widget::~Widget()
{
    delete ui;
}
