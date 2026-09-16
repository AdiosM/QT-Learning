#ifndef ENTRYDIALOG_H
#define ENTRYDIALOG_H

// QDialog 是 Qt 提供的“对话框”基类。
// 我们的 EntryDialog 将继承 QDialog，
// 因此它本身就是一个可以弹出的对话框窗口。

// ============================================================
// EntryDialog
//
// 作用：
//      用来让用户输入一条密码记录的信息。以一个新的窗口的形式出现
// 例如：
//      名称：GitHub
//      用户名：example@gmail.com
//      密码：123456
//      URL：https://github.com
//      备注：代码仓库
// ============================================================

#include <QDialog>
#include <QString>

class EntryDialog:public QDialog //继承QDialog
{
    Q_OBJECT

public:
    explicit EntryDialog(QWidget *parent = nullptr);//创建EntryDialog时没有指定父窗口，那么parent默认是空指针
    ~EntryDialog();

    //读取用户输入的数据
    QString title()const; //获取“名称”
    QString username()const;//获取“用户名”
    QString password()const;//获取“密码”
    QString url()const;//获取“URL”
    QString notes()const;//获取“备注”

    QString category()const;//获取用户选择的分类

    //修改用户输入的数据
    void setTitle(const QString &title);//修改标题
    void setUsername(const QString &username);
    void setPassword(const QString &password);
    void setUrl(const QString &url);
    void setNotes(const QString &notes);
    void setCategory(const QString &category);

private:
    void setupUi();// 创建界面的函数,把创建控件的代码放进setupUi()

    //界面控件，输入框，前向声明
    class QLineEdit *titleEdit;
    class QLineEdit *usernameEdit;
    class QLineEdit *passwordEdit;
    class QLineEdit *urlEdit;

    class QTextEdit *notesEdit;//输入备注，QTextEdit用于输入多行文本

    class QComboBox *categoryBox;//分类选择框

    //确定按钮 和 取消按钮
    class QPushButton *okButton;
    class QPushButton *cancelButton;

};


#endif // ENTRYDIALOG_H
