#ifndef PASSWORDMODEL_H
#define PASSWORDMODEL_H

//数据和QT界面之间的中间层，告诉QT数据是什么，以及表格应该如何读取这些数据。管理密码记录。
/***主要负责：
 * 保存PasswordEntry数据结构(密码条目)
 * 给QTableView提供数据显示
 * 增加、修改、删除密码条目
 * 加载数据库
 * 同步数据库
 * 管理数据库ID
 ***/

#include <QAbstractTableModel>
#include <QVector>

#include "PasswordEntry.h"
#include "DatabaseManager.h"

class PasswordModel:public QAbstractTableModel
{//QAbstractTableModel为以二维数据项数组形式组织数据的模型提供标准接口，是一个表格型数据模型
    //我们这里设计的表格是一个行 X 列的表格，所以继承QAbstractTableModel
    Q_OBJECT

public:
    explicit PasswordModel(QObject *parent=nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex())const override; //计算有多少行密码条目（表格有多少行）
    int columnCount(const QModelIndex &parent = QModelIndex())const override; //有多少列

    QVariant data(const QModelIndex &index,int role = Qt::DisplayRole)const override;//第几行第几列显示什么数据
    //index表示Model中的某一个数据位置，主要包含（row,column）;role表示想从这个位置获取什么类型的数据
    //QVariant是Qt用来统一保存/传递不同类型数据的一个类型，Qt可以把很多常见类型放进QVariant，是一个通用容器

    QVariant headerData(
        int section,
        Qt::Orientation orientation,
        int role = Qt::DisplayRole
        )const override;//负责告诉表格，表头显示什么

    void addEntry(const PasswordEntry &entry);//添加密码条目
    void updateEntry(int row,const PasswordEntry &entry);//修改密码条目
    void removeEntry(int row);//删除密码

    const PasswordEntry &entryAt(int row)const;//获取某一行的数据

    void loadFromDatabase();//PasswordModel从数据库获取数据的入口，
//把DatabaseManager从SQLite读取出来的数据，加载到 PasswordModel的entries 中。

    bool toggleFavorite(int row);

    QVector<PasswordEntry>getEntries()const;//让备份模块读取当前密码数据，把 PasswordModel 内部保存的所有密码记录提供给外部
    bool replaceEntries(const QVector<PasswordEntry>&newEntries);//恢复数据，用导入的数据整体替换当前Model中的数据


private:
    QVector<PasswordEntry>entries;//用于存储密码信息的容器,保存当前程序正在显示的密码数据
    QVector<int>entryIds;//用来保存每一条PasswordEntry对应的SQLite数据库ID，entries[0]与entriesIds[0]必须一一对应

    DatabaseManager databaseManager;//PasswordModel用来操作SQLite数据库的对象
};


#endif // PASSWORDMODEL_H
