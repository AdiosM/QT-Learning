#include "camerawindow.h"

#include <QCameraDevice>
#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>
#include <QVideoWidget>

CameraWindow::CameraWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("定时拍照"));
    resize(1040, 720);
    QString pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (pictures.isEmpty())
        pictures = QDir::homePath();
    outputFolder_ = settings_.value(QStringLiteral("outputFolder"),
                                    QDir(pictures).filePath(QStringLiteral("定时拍照"))).toString();

    imageCapture_.setFileFormat(QImageCapture::JPEG);
    captureSession_.setImageCapture(&imageCapture_);
    timer_.setTimerType(Qt::PreciseTimer);
    buildUi();
    captureSession_.setVideoOutput(videoWidget_);

    connect(&mediaDevices_, &QMediaDevices::videoInputsChanged,
            this, &CameraWindow::refreshCameras);
    connect(&imageCapture_, &QImageCapture::readyForCaptureChanged,
            this, [this](bool) { refreshControls(); startFirstPhotoIfReady(); });
    connect(&imageCapture_, &QImageCapture::imageSaved,
            this, &CameraWindow::onImageSaved);
    connect(&imageCapture_, &QImageCapture::errorOccurred,
            this, &CameraWindow::onCaptureError);
    connect(&timer_, &QTimer::timeout, this, &CameraWindow::onTimer);
    refreshCameras();
}

CameraWindow::~CameraWindow()
{
    timer_.stop();
    if (camera_)
        camera_->stop();
    captureSession_.setCamera(nullptr);
}

void CameraWindow::buildUi()
{
    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(18);
    auto *title = new QLabel(QStringLiteral("定时拍照"), central);
    title->setObjectName(QStringLiteral("title"));
    auto *subtitle = new QLabel(QStringLiteral("连接电脑相机，设置间隔后即可连续保存照片。"), central);
    subtitle->setObjectName(QStringLiteral("subtitle"));
    root->addWidget(title);
    root->addWidget(subtitle);

    auto *previewFrame = new QFrame(central);
    previewFrame->setObjectName(QStringLiteral("previewFrame"));
    auto *previewLayout = new QVBoxLayout(previewFrame);
    previewLayout->setContentsMargins(4, 4, 4, 4);
    videoWidget_ = new QVideoWidget(previewFrame);
    videoWidget_->setMinimumHeight(320);
    previewLayout->addWidget(videoWidget_);
    root->addWidget(previewFrame, 1);

    auto *settingsFrame = new QFrame(central);
    settingsFrame->setObjectName(QStringLiteral("settingsFrame"));
    auto *form = new QFormLayout(settingsFrame);
    form->setContentsMargins(20, 18, 20, 18);
    form->setHorizontalSpacing(20);
    form->setVerticalSpacing(14);

    auto *cameraRow = new QWidget(settingsFrame);
    auto *cameraLayout = new QHBoxLayout(cameraRow);
    cameraLayout->setContentsMargins(0, 0, 0, 0);
    cameraCombo_ = new QComboBox(cameraRow);
    cameraCombo_->setMinimumWidth(280);
    auto *refreshButton = new QPushButton(QStringLiteral("刷新设备"), cameraRow);
    cameraLayout->addWidget(cameraCombo_, 1);
    cameraLayout->addWidget(refreshButton);
    form->addRow(QStringLiteral("相机"), cameraRow);
    connect(cameraCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { selectCamera(); });
    connect(refreshButton, &QPushButton::clicked, this, &CameraWindow::refreshCameras);

    auto *intervalRow = new QWidget(settingsFrame);
    auto *intervalLayout = new QHBoxLayout(intervalRow);
    intervalLayout->setContentsMargins(0, 0, 0, 0);
    intervalSpin_ = new QDoubleSpinBox(intervalRow);
    intervalSpin_->setRange(0.5, 3600.0);
    intervalSpin_->setDecimals(1);
    intervalSpin_->setSingleStep(0.5);
    intervalSpin_->setSuffix(QStringLiteral(" 秒"));
    bool valid = false;
    const double savedInterval = settings_.value(QStringLiteral("intervalSeconds"), 5.0)
                                     .toDouble(&valid);
    intervalSpin_->setValue(valid ? savedInterval : 5.0);
    intervalLayout->addWidget(intervalSpin_);
    intervalLayout->addStretch();
    form->addRow(QStringLiteral("拍照间隔"), intervalRow);
    connect(intervalSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double seconds) {
                settings_.setValue(QStringLiteral("intervalSeconds"), seconds);
                if (autoRunning_ && timer_.isActive()) {
                    timer_.start(intervalMilliseconds());
                    statusLabel_->setText(QStringLiteral("拍照间隔已改为 %1 秒。").arg(seconds));
                }
            });

    auto *folderRow = new QWidget(settingsFrame);
    auto *folderLayout = new QHBoxLayout(folderRow);
    folderLayout->setContentsMargins(0, 0, 0, 0);
    folderLabel_ = new QLabel(QDir::toNativeSeparators(outputFolder_), folderRow);
    folderLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    folderLabel_->setWordWrap(true);
    auto *chooseButton = new QPushButton(QStringLiteral("选择文件夹"), folderRow);
    auto *openButton = new QPushButton(QStringLiteral("打开文件夹"), folderRow);
    folderLayout->addWidget(folderLabel_, 1);
    folderLayout->addWidget(chooseButton);
    folderLayout->addWidget(openButton);
    form->addRow(QStringLiteral("保存位置"), folderRow);
    connect(chooseButton, &QPushButton::clicked, this, &CameraWindow::chooseFolder);
    connect(openButton, &QPushButton::clicked, this, &CameraWindow::openFolder);
    root->addWidget(settingsFrame);

    auto *actions = new QHBoxLayout;
    startButton_ = new QPushButton(QStringLiteral("开始自动拍照"), central);
    startButton_->setObjectName(QStringLiteral("primaryButton"));
    stopButton_ = new QPushButton(QStringLiteral("停止"), central);
    manualButton_ = new QPushButton(QStringLiteral("手动拍一张"), central);
    actions->addWidget(startButton_);
    actions->addWidget(stopButton_);
    actions->addWidget(manualButton_);
    actions->addStretch();
    root->addLayout(actions);
    connect(startButton_, &QPushButton::clicked, this, &CameraWindow::startAuto);
    connect(stopButton_, &QPushButton::clicked, this, &CameraWindow::stopAuto);
    connect(manualButton_, &QPushButton::clicked, this, &CameraWindow::takePhoto);

    auto *footer = new QHBoxLayout;
    statusLabel_ = new QLabel(QStringLiteral("正在查找相机…"), central);
    statusLabel_->setObjectName(QStringLiteral("statusLabel"));
    countLabel_ = new QLabel(QStringLiteral("本次已保存 0 张"), central);
    footer->addWidget(statusLabel_, 1);
    footer->addWidget(countLabel_);
    root->addLayout(footer);
    setStyleSheet(R"(
        QMainWindow, QWidget { background: #f5f7fb; color: #172133; font-size: 14px; }
        QLabel#title { font-size: 27px; font-weight: 700; }
        QLabel#subtitle { color: #65738a; }
        QFrame#previewFrame { background: #111827; border-radius: 12px; }
        QVideoWidget { background: #111827; }
        QFrame#settingsFrame { background: white; border: 1px solid #dce3ed; border-radius: 10px; }
        QPushButton { background: white; border: 1px solid #cbd5e1; border-radius: 7px; padding: 9px 14px; }
        QPushButton:hover { background: #edf2f9; }
        QPushButton:disabled { color: #99a4b5; background: #eef1f5; }
        QPushButton#primaryButton { background: #2563eb; border-color: #2563eb; color: white; font-weight: 600; }
        QPushButton#primaryButton:hover { background: #1d4ed8; }
        QComboBox, QDoubleSpinBox { background: white; border: 1px solid #cbd5e1; border-radius: 6px; padding: 7px; }
        QLabel#statusLabel { color: #475569; }
    )");
}

void CameraWindow::refreshCameras()
{
    const QByteArray previousId = cameraCombo_->currentData().toByteArray();
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    {
        QSignalBlocker blocker(cameraCombo_);
        cameraCombo_->clear();
        for (const QCameraDevice &device : cameras)
            cameraCombo_->addItem(device.description(), device.id());
        int index = cameraCombo_->findData(previousId);
        if (index < 0)
            index = cameraCombo_->findData(QMediaDevices::defaultVideoInput().id());
        if (index < 0 && !cameras.isEmpty())
            index = 0;
        cameraCombo_->setCurrentIndex(index);
    }
    selectCamera();
}

void CameraWindow::selectCamera()
{
    const QByteArray selectedId = cameraCombo_->currentData().toByteArray();
    QCameraDevice selected;
    for (const QCameraDevice &device : QMediaDevices::videoInputs()) {
        if (device.id() == selectedId) {
            selected = device;
            break;
        }
    }
    if (selected.isNull()) {
        stopAuto();
        if (camera_) {
            QCamera *oldCamera = camera_;
            camera_ = nullptr;
            oldCamera->stop();
            captureSession_.setCamera(nullptr);
            delete oldCamera;
        }
        statusLabel_->setText(QStringLiteral("未检测到相机。请连接设备后点击“刷新设备”。"));
        refreshControls();
        return;
    }
    if (camera_ && camera_->cameraDevice().id() == selected.id()) {
        refreshControls();
        return;
    }
    stopAuto();
    pendingId_ = -1;
    if (camera_) {
        QCamera *oldCamera = camera_;
        camera_ = nullptr;
        oldCamera->stop();
        captureSession_.setCamera(nullptr);
        delete oldCamera;
    }
    camera_ = new QCamera(selected, this);
    connect(camera_, &QCamera::errorOccurred, this,
            [this](QCamera::Error error, const QString &message) {
                if (error == QCamera::NoError)
                    return;
                stopAuto();
                statusLabel_->setText(QStringLiteral("相机错误：%1")
                                          .arg(message.isEmpty()
                                                   ? QStringLiteral("请检查设备连接和系统相机权限")
                                                   : message));
                refreshControls();
            });
    captureSession_.setCamera(camera_);
    statusLabel_->setText(QStringLiteral("正在启动相机：%1").arg(selected.description()));
    camera_->start();
    refreshControls();
}

void CameraWindow::refreshControls()
{
    const bool ready = camera_ && imageCapture_.isReadyForCapture();
    startButton_->setEnabled(camera_ && !autoRunning_);
    stopButton_->setEnabled(autoRunning_);
    manualButton_->setEnabled(ready && pendingId_ < 0);
    cameraCombo_->setEnabled(cameraCombo_->count() > 0 && pendingId_ < 0);
}

int CameraWindow::intervalMilliseconds() const
{
    return qRound(intervalSpin_->value() * 1000.0);
}

bool CameraWindow::ensureOutputFolder()
{
    if (!QDir().mkpath(outputFolder_) || !QFileInfo(outputFolder_).isDir()) {
        QMessageBox::warning(this, QStringLiteral("无法保存照片"),
                             QStringLiteral("无法使用保存文件夹：\n%1").arg(outputFolder_));
        statusLabel_->setText(QStringLiteral("保存文件夹不可用。"));
        return false;
    }
    return true;
}

void CameraWindow::chooseFolder()
{
    const QString folder = QFileDialog::getExistingDirectory(
        this, QStringLiteral("选择照片保存文件夹"), outputFolder_);
    if (folder.isEmpty())
        return;
    outputFolder_ = folder;
    folderLabel_->setText(QDir::toNativeSeparators(outputFolder_));
    settings_.setValue(QStringLiteral("outputFolder"), outputFolder_);
}

void CameraWindow::openFolder()
{
    if (ensureOutputFolder() && !QDesktopServices::openUrl(QUrl::fromLocalFile(outputFolder_)))
        statusLabel_->setText(QStringLiteral("无法打开保存文件夹。"));
}

void CameraWindow::startAuto()
{
    if (!camera_ || !ensureOutputFolder())
        return;
    autoRunning_ = true;
    waitingForFirstPhoto_ = true;
    statusLabel_->setText(QStringLiteral("自动拍照已启动，等待相机就绪…"));
    refreshControls();
    startFirstPhotoIfReady();
}

void CameraWindow::startFirstPhotoIfReady()
{
    if (!autoRunning_ || !waitingForFirstPhoto_ || pendingId_ >= 0 ||
        !imageCapture_.isReadyForCapture())
        return;
    waitingForFirstPhoto_ = false;
    takePhoto();
    if (autoRunning_)
        timer_.start(intervalMilliseconds());
}

void CameraWindow::stopAuto()
{
    const bool wasRunning = autoRunning_;
    autoRunning_ = false;
    waitingForFirstPhoto_ = false;
    timer_.stop();
    if (startButton_)
        refreshControls();
    if (wasRunning)
        statusLabel_->setText(QStringLiteral("自动拍照已停止；正在保存的照片会继续完成。"));
}

void CameraWindow::onTimer()
{
    if (!autoRunning_ || pendingId_ >= 0)
        return;
    if (!imageCapture_.isReadyForCapture()) {
        statusLabel_->setText(QStringLiteral("相机暂时未就绪，已跳过本次拍照。"));
        return;
    }
    takePhoto();
}

void CameraWindow::takePhoto()
{
    if (!camera_ || pendingId_ >= 0 || !imageCapture_.isReadyForCapture())
        return;
    if (!ensureOutputFolder()) {
        if (autoRunning_)
            stopAuto();
        return;
    }
    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz"));
    const QString name = QStringLiteral("photo_%1_%2.jpg")
                             .arg(timestamp)
                             .arg(++fileSequence_, 4, 10, QLatin1Char('0'));
    const int requestId = imageCapture_.captureToFile(QDir(outputFolder_).filePath(name));
    if (requestId < 0) {
        if (autoRunning_)
            stopAuto();
        statusLabel_->setText(QStringLiteral("拍照请求失败，请检查相机和保存位置。"));
        return;
    }
    pendingId_ = requestId;
    statusLabel_->setText(QStringLiteral("正在拍照并保存…"));
    refreshControls();
}

void CameraWindow::onImageSaved(int id, const QString &fileName)
{
    if (id != pendingId_)
        return;
    pendingId_ = -1;
    ++savedCount_;
    countLabel_->setText(QStringLiteral("本次已保存 %1 张").arg(savedCount_));
    statusLabel_->setText(QStringLiteral("已保存：%1").arg(QFileInfo(fileName).fileName()));
    refreshControls();
    startFirstPhotoIfReady();
}

void CameraWindow::onCaptureError(int id, QImageCapture::Error, const QString &message)
{
    if (id != pendingId_)
        return;
    pendingId_ = -1;
    if (autoRunning_)
        stopAuto();
    statusLabel_->setText(QStringLiteral("拍照失败：%1")
                              .arg(message.isEmpty() ? QStringLiteral("未知错误") : message));
    refreshControls();
}
