#include"EntryDialog.h"

#include<QLineEdit>
#include<QTextEdit>
#include<QPushButton>
#include<QLabel>
#include<QFormLayout> //QFormLayout：非常适合“标签 + 输入框”的布局
#include<QVBoxLayout> //垂直布局
#include<QHBoxLayout>
#include<QFont> //设置字体
#include<QComboBox>
#include<QSizePolicy>

EntryDialog::EntryDialog(QWidget *parent)
{
    setWindowTitle("新建密码"); //设置窗口标题

    resize(500,420);
    setMinimumSize(450,380);

    setupUi();//调用我们自己编写的界面创建函数
}

EntryDialog::~EntryDialog(){}

void EntryDialog::setupUi() //void EntryDialog::setupUi()表示这是EntryDialog 类的 setupUi() 成员函数的实现
{
    //创建标题输入框
    titleEdit = new QLineEdit(this); //将当前的EntryDialog设置成它的parent
    titleEdit->setPlaceholderText("例如：Github");//设置提示文字

    //创建用户名输入框
    usernameEdit = new QLineEdit(this);
    usernameEdit->setPlaceholderText("例如：example@gmail.com");

    //创建密码输入框
    passwordEdit = new QLineEdit(this);
    passwordEdit->setPlaceholderText( "请输入密码");

    //设置密码显示模式
    passwordEdit->setEchoMode( QLineEdit::Password);//QLineEdit::Password表示输入的字符是隐藏显示

    //创建URL输入框
    urlEdit = new QLineEdit(this);
    urlEdit->setPlaceholderText("例如，https://github.com");

    //创建分类选择框
    categoryBox = new QComboBox(this);
    categoryBox->addItem("网站");
    categoryBox->addItem("软件");
    categoryBox->addItem("工作");
    categoryBox->addItem("学习");
    categoryBox->addItem("其他");

    //创建备注输入框
    notesEdit = new QTextEdit(this);
    notesEdit->setPlaceholderText( "输入备注信息...");
    notesEdit->setMinimumHeight(100);//设置备注框的最小高度
    notesEdit->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::MinimumExpanding);

    //创建“确定”和“取消”按钮
    okButton = new QPushButton("确定", this);
    cancelButton = new QPushButton("取消", this);
/***
EntryDialog
    │
    ├── titleEdit
    ├── usernameEdit
    ├── passwordEdit
    ├── urlEdit
    └── notesEdit
 ***/

    // 创建表单布局
    QFormLayout *formLayout = new QFormLayout;
    formLayout->setVerticalSpacing(12);//设置表单布局中控件之间的垂直间距

    //把“标签 + 输入框”加入表单布局
    formLayout->addRow("名称：",titleEdit);
    formLayout->addRow("用户名：",usernameEdit);
    formLayout->addRow("密码：",passwordEdit);
    formLayout->addRow("URL: ",urlEdit);
    formLayout->addRow("分类",categoryBox);

    //添加备注
    formLayout->addRow("备注: ",notesEdit);

    //创建按钮布局，水平布局
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();//加一个弹簧，自动占据剩余空间
    // [弹簧] [取消] [确定]，两个按钮会靠右
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(okButton);

    // 创建整个窗口的垂直布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20,20,20,20);//设置四周的边距，左，上，右，下，单位：像素

    //设置控件之间的间距
    mainLayout->setSpacing(12);

    //把表单布局加入主布局
    mainLayout->addLayout(formLayout);
/***当前结构：
 *  mainLayout
         │
         ├── formLayout
         │
         └── buttonLayout
 ***/

    //添加一个弹簧
 //    mainLayout->addStretch();//让按钮区域靠近窗口底部

     //添加按钮布局
    mainLayout->addLayout(buttonLayout);

    //连接按钮和 QDialog，用户点击取消按钮后，reject会关闭对话框
    connect(cancelButton,&QPushButton::clicked,this,&QDialog::reject);
    connect(okButton,&QPushButton::clicked,this,&QDialog::accept);

}

//--------读取---------
QString EntryDialog::title()const //获取标题
{
    return titleEdit->text();// 获取 QLineEdit 当前输入的文字
}

QString EntryDialog::username() const //获取用户名
{
    return usernameEdit->text();
}

QString EntryDialog::password() const //获取密码
{
    return passwordEdit->text();
}

QString EntryDialog::url() const //获取url
{
    return urlEdit->text();
}

QString EntryDialog::notes()const //获取备注
{
    //QTextEdit 使用 toPlainText()获取普通文本
    return notesEdit->toPlainText();
}

QString EntryDialog::category() const
{
    return categoryBox->currentText();
}
//----------------------------


//---------修改-------------
void EntryDialog::setTitle(const QString &title)
{
    titleEdit->setText(title);//将用户修改后的title信息保存到titleEdit变量中
}

void EntryDialog::setUsername(const QString &username)
{
    usernameEdit->setText(username);
}

void EntryDialog::setPassword(const QString &password)
{
    passwordEdit->setText(password);
}

void EntryDialog::setUrl(const QString &url)
{
    urlEdit->setText(url);
}

void EntryDialog::setNotes(const QString &notes)
{
    notesEdit->setPlainText(notes);
}

void EntryDialog::setCategory(const QString &category)
{
    int index =
        categoryBox->findText(category);

    if(index >= 0)
    {
        categoryBox->setCurrentIndex(index);
    }
}
//--------------------------------

