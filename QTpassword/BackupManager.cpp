#include "BackupManager.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

bool BackupManager::exportToJson(const QString &filePath,const QVector<PasswordEntry> &entries )
{
    QJsonArray array;

    for(const PasswordEntry &entry : entries)
    {

        QJsonObject obj;

        obj["title"] = entry.title();

        obj["username"] = entry.username();

        obj["password"] = entry.password();

        obj["url"] = entry.url();

        obj["notes"] = entry.notes();

        obj["category"] = entry.category();

        obj["favorite"] = entry.favorite();

        array.append(obj);
    }

    QJsonDocument doc(array);

    QFile file(filePath);

    if(!file.open(QIODevice::WriteOnly))
    {
        return false;
    }

    file.write( doc.toJson() );
    file.close();

    return true;
}


QVector<PasswordEntry>BackupManager::importFromJson(const QString &filePath)
{
    QVector<PasswordEntry> entries;

    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
    {
        return entries;
    }

    QByteArray data =file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);

    QJsonArray array =doc.array();

    for(auto value : array)
    {

        QJsonObject obj = value.toObject();

        PasswordEntry entry(
            obj["title"].toString(),
            obj["username"].toString(),
            obj["password"].toString(),
            obj["url"].toString(),
            obj["notes"].toString(),
            obj["category"].toString(),
            obj["favorite"].toBool()
            );

        entries.append(entry);
    }

    return entries;
}