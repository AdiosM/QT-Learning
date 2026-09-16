#ifndef BACKUPMANAGER_H
#define BACKUPMANAGER_H

#include <QString>
#include <QVector>

#include "PasswordEntry.h"

class BackupManager
{
public:
    //把 QVector<PasswordEntry> 中的所有密码记录转换成 JSON，然后写入文件
    //static关键字表示这个函数属于类本身，不依赖某个具体对象
    static bool exportToJson(const QString &filePath,const QVector<PasswordEntry>&entries);

    //从 JSON 文件读取数据，并重新创建一组 PasswordEntry
    static QVector<PasswordEntry>importFromJson(const QString &filePath);

};






#endif // BACKUPMANAGER_H
