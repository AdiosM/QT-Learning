#include "LoginDialog.h"

#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSettings>
#include <QCryptographicHash>


LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
{
    firstRun = isFirstRun();

    setupUi();
}


void LoginDialog::setupUi()
{
    setWindowTitle(
        firstRun
            ? "设置主密码"
            : "主密码登录"
        );

    setFixedSize(350, 220);


    QVBoxLayout *mainLayout =
        new QVBoxLayout(this);


    titleLabel =
        new QLabel(
            firstRun
                ? "首次运行，请设置主密码"
                : "请输入主密码",
            this
            );


    passwordEdit =  new QLineEdit(this);

    passwordEdit->setEchoMode(QLineEdit::Password);

    passwordEdit->setPlaceholderText(  "输入主密码");

    confirmLabel = new QLabel("确认主密码：", this );


    confirmPasswordEdit = new QLineEdit(this);

    confirmPasswordEdit->setEchoMode(QLineEdit::Password );

    confirmPasswordEdit->setPlaceholderText( "再次输入主密码");


    // 不是第一次运行时，
    // 不需要确认密码
    if (!firstRun)
    {
        confirmLabel->hide();
        confirmPasswordEdit->hide();
    }

    confirmButton = new QPushButton(firstRun ? "设置" : "登录", this );

    cancelButton =  new QPushButton( "取消", this );


    QHBoxLayout *buttonLayout = new QHBoxLayout;

    buttonLayout->addStretch();

    buttonLayout->addWidget(cancelButton);

    buttonLayout->addWidget( confirmButton);


    mainLayout->addWidget(  titleLabel);

    mainLayout->addWidget(  passwordEdit );

    mainLayout->addWidget( confirmLabel );

    mainLayout->addWidget( confirmPasswordEdit );

    mainLayout->addStretch();

    mainLayout->addLayout(  buttonLayout);


    connect(
        confirmButton,
        &QPushButton::clicked,
        this,
        &LoginDialog::handleConfirm
        );


    connect(
        cancelButton,
        &QPushButton::clicked,
        this,
        &QDialog::reject
        );

    connect(
        passwordEdit,
        &QLineEdit::returnPressed,
        this,
        &LoginDialog::handleConfirm
        );//按回车键可以登录
}

bool LoginDialog::isFirstRun() const
{
    QSettings settings(
        "QtPassword",
        "QtPassword"
        );

    return !settings.contains(
        "masterPasswordHash"
        );
}

QString LoginDialog::passwordHash(const QString &password) const
{
    QByteArray hash =
        QCryptographicHash::hash(
            password.toUtf8(),
            QCryptographicHash::Sha256
            );

    return QString(  hash.toHex() );
}


void LoginDialog::handleConfirm()
{
    QString password = passwordEdit->text();


    if (password.isEmpty())
    {
        QMessageBox::warning(
            this,
            "提示",
            "主密码不能为空"
            );

        return;
    }


    QSettings settings(  "QtPassword",  "QtPassword"  );


    // =====================
    // 第一次运行
    // =====================

    if (firstRun)
    {
        QString confirmPassword =
            confirmPasswordEdit->text();


        if (password != confirmPassword)
        {
            QMessageBox::warning(
                this,
                "提示",
                "两次输入的主密码不一致"
                );

            return;
        }


        settings.setValue(
            "masterPasswordHash",
            passwordHash(password)
            );

        accept();

        return;
    }


    // =====================
    // 正常登录
    // =====================

    QString savedHash =settings.value("masterPasswordHash" ).toString();

    if (  passwordHash(password) == savedHash )
    {   accept();}
    else
    {
        QMessageBox::warning(
            this,
            "登录失败",
            "主密码错误"
            );

        passwordEdit->clear();

        passwordEdit->setFocus();
    }
}

