#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H
//实现启动软件后，弹出一个登录对话框

#include<QDialog>

class QLineEdit;
class QPushButton;
class QLabel;

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);

private:
    void setupUi();

    void handleConfirm();

    bool isFirstRun() const;

    QString passwordHash(
        const QString &password
        ) const;

private:
    QLineEdit *passwordEdit;
    QLineEdit *confirmPasswordEdit;

    QLabel *titleLabel;
    QLabel *confirmLabel;

    QPushButton *confirmButton;
    QPushButton *cancelButton;

    bool firstRun;
};

#endif // LOGINDIALOG_H
