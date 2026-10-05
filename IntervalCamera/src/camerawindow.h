#pragma once
#include <QCamera>
#include <QImageCapture>
#include <QMainWindow>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QSettings>
#include <QTimer>

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QVideoWidget;

class CameraWindow final : public QMainWindow
{
public:
    explicit CameraWindow(QWidget *parent = nullptr);
    ~CameraWindow() override;

private:
    void buildUi();
    void refreshCameras();
    void selectCamera();
    void refreshControls();
    void startFirstPhotoIfReady();
    void startAuto();
    void stopAuto();
    void takePhoto();
    void onTimer();
    void onImageSaved(int id, const QString &fileName);
    void onCaptureError(int id, QImageCapture::Error error, const QString &message);
    void chooseFolder();
    void openFolder();
    bool ensureOutputFolder();
    int intervalMilliseconds() const;

    QSettings settings_;
    QMediaDevices mediaDevices_;
    QMediaCaptureSession captureSession_;
    QImageCapture imageCapture_;
    QCamera *camera_ = nullptr;
    QTimer timer_;
    QVideoWidget *videoWidget_ = nullptr;
    QComboBox *cameraCombo_ = nullptr;
    QDoubleSpinBox *intervalSpin_ = nullptr;
    QLabel *folderLabel_ = nullptr;
    QLabel *statusLabel_ = nullptr;
    QLabel *countLabel_ = nullptr;
    QPushButton *startButton_ = nullptr;
    QPushButton *stopButton_ = nullptr;
    QPushButton *manualButton_ = nullptr;
    QString outputFolder_;
    int pendingId_ = -1;
    int savedCount_ = 0;
    quint64 fileSequence_ = 0;
    bool autoRunning_ = false;
    bool waitingForFirstPhoto_ = false;
};
