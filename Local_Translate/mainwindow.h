#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QElapsedTimer>
#include <QString>

class QComboBox;
class QPushButton;
class QPlainTextEdit;
class QNetworkAccessManager;

class QProcess;
class QTimer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void setupUI();//程序UI
    void translate();//翻译

    // 模型相关函数实现在 mainwindow_model.cpp 中。
    void startModel();
    void checkModelReady();
    void modelFailed(const QString &message);


    QComboBox *sourceLanguage = nullptr;//源语言
    QComboBox *targetLanguage = nullptr;//翻译的目标语言

    QPushButton *translateButton = nullptr;//翻译按钮

    QPlainTextEdit *sourceText = nullptr;//源语言输入面板
    QPlainTextEdit *translatedText = nullptr;//翻译输出面板

    QNetworkAccessManager *networkManager = nullptr;

    QProcess *modelProcess = nullptr;
    QTimer *healthTimer=nullptr;

    QElapsedTimer loadingClock;
    QString modelLog;

    bool modelReady = false;
    bool healthChecking = false;
    bool modelHasFailed = false;
    bool closing = false;


};
#endif // MAINWINDOW_H
