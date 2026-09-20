#ifndef MAINWINDOW_H
#define MAINWINDOW_H
//主界面及交互逻辑


#include <QMainWindow>
#include <QModelIndex>
#include "PasswordFilterProxyModel.h"

class QSplitter; //前向声明（Forward Declaration）
class QTreeWidget;
class QTreeWidgetItem;
class QTableView;
class PasswordModel;//自定义的类
class QLineEdit;
class QPushButton;

class QWidget;
class QLabel;
class QTextEdit;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void setupUi();
    void createNewEntry();//新建密码窗口，用户点击“+ 新建”按钮时执行
    void showEntryDetails(const QModelIndex &index);//当用户选择密码列表中的某一条记录时，更新右侧密码详情区域
    void editEntry();//编辑密码信息
    void deleteEntry();//删除密码信息
    void toggleFavorite();

    void exportBackup();//用户点击“导出备份”以后，处理整个导出流程
    void importBackup();//用户选择一个 JSON 备份文件，然后把里面的密码记录读回来


private:
    QSplitter *mainSplitter; //主界面的水平分割器。左边：搜索、分组、新建；右边：密码列表、密码详情

    QWidget *leftPanel;//左面板
    QWidget *rightPanel;

    QLineEdit *searchEdit; //搜索框

    QTreeWidget *groupTree; //目录树
    QTableView *passwordTable;//密码列表，显示密码条目
    QPushButton *newButton; //“+ 新建” 按钮

    QPushButton *editButton;
    QPushButton *deleteButton;
    QPushButton *showPasswordButton;//显示密码
    QPushButton *copyPasswordButton;//复制密码按钮
    QPushButton *favoriteButton;//收藏按钮

    QLabel *detailTitleLabel;//"密码详情"标题
    QLabel *detailNameLabel;//"名称"对应的值
    QLabel *detailUsernameLabel;//"用户名"对应的值
    QLabel *detailUrlLabel;//“URL”对应的值
    QLabel *detailNotesLabel;//“备注”对应的值
    QLabel *detailPasswordLabel;//“密码”对应的值

    PasswordModel *passwordModel; //保存所有密码条目
    PasswordFilterProxyModel *proxyModel;//proxyModel 是 PasswordModel 和 QTableView 之间的“过滤/排序中间层
    //proxyModel告诉界面“现在只显示哪些数据”。

    bool passwordVisible = false;
    QString currentPassword;

private slots:
    void categoryClicked(QTreeWidgetItem *item,int column);
    //编译器只需要指导QTreeWidgetItem是一个类即可，所以只需要前向声明



};
#endif // MAINWINDOW_H
