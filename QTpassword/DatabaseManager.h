#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H
//负责SQLite数据操作

#include <QString>
#include <QVector>
#include <QPair>

#include "PasswordEntry.h"



class DatabaseManager
{
public:
    DatabaseManager();

    bool openDatabase(); //打开QtPassword使用的SQLite数据库
    bool createTables();//创建 QtPassword 需要的数据表

    bool insertEntry(const PasswordEntry &entry);//把一条密码记录插入SQLite数据库
    int insertEntryAndGetId(const PasswordEntry &entry);

    QVector<PasswordEntry>loadEntries();//从SQLite数据库中，把已经保存的密码记录全部读取出来，并转换成 QVector<PasswordEntry> 返回给程序。

    QVector<QPair<int, PasswordEntry>>loadEntriesWithIds();//从SQLite数据库中读取所有密码记录，并且把每条记录的数据库 id 和对应的PasswordEntry一起返回。
    //QVector<QPair<int, PasswordEntry>>表示：一个 QVector，里面存放很多“整数 + PasswordEntry”组成的二元组。

    bool updateEntry(int id,const PasswordEntry &entry);
    bool deleteEntry(int id);

    bool replaceAllEntries(const QVector<PasswordEntry>&entries,QVector<int>&newIds);


private:
    QString databasePath;//保存 SQLite 数据库文件 passwords.db 的完整路径。
};



#endif // DATABASEMANAGER_H
