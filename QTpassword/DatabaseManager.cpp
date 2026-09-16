#include "DatabaseManager.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QCoreApplication>

DatabaseManager::DatabaseManager()
{
    databasePath =
        QCoreApplication::applicationDirPath()
        + "/passwords.db";
    //QCoreApplication::applicationDirPath()获取当前 Qt 程序所在的文件夹路径。
}

bool DatabaseManager::openDatabase()
{
    QSqlDatabase database =
        QSqlDatabase::addDatabase("QSQLITE");//QSQLITE告诉Qt使用的是SQLite数据库
    //database的作用是表示QtPassword当前使用的SQLite数据库链接

    database.setDatabaseName(databasePath);//指定数据库文件，连接databasePath中的SQLite文件

    if (!database.open())
    {
        qDebug() << "数据库打开失败："
                 << database.lastError().text();

        return false;
    }

    qDebug() << "数据库打开成功："
             << databasePath;

    return true;
}

bool DatabaseManager::createTables()
{
    QSqlQuery query;//是一个SQL查询对象，在本项目中的作用是负责向SQLite数据库发送SQL命令

    QString sql = R"(
        CREATE TABLE IF NOT EXISTS password_entries (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            username TEXT,
            password TEXT,
            url TEXT,
            notes TEXT,
            category TEXT,
            favorite INTEGER DEFAULT 0)
    )";//sql的字符串变量，保存我们准备给SQLite执行的SQL命令
    //favorite INTEGER DEFAULT 0,在SQLite中，0表示未收藏，1表示已收藏

    if (!query.exec(sql))
    {
        qDebug() << "创建数据表失败："
                 << query.lastError().text();

        return false;
    }

    qDebug() << "数据表创建成功";

    return true;
}

bool DatabaseManager::insertEntry(const PasswordEntry &entry)
{
    QSqlQuery query; //创建SQL查询对象

    query.prepare(R"(
        INSERT INTO password_entries
        (title, username, password, url, notes,category,favorite)
        VALUES
        (:title, :username, :password, :url, :notes,:category,:favorite)
    )");//prepare()用于准备一条即将执行的SQL语句
    //INSERT INTO password_entries表示向password_entries表中插入一条数据（即新增一行）

    query.bindValue(":title", entry.title());//把entry中的名称填入title
    query.bindValue(":username", entry.username());
    query.bindValue(":password", entry.password());
    query.bindValue(":url", entry.url());
    query.bindValue(":notes", entry.notes());
    query.bindValue(":category",entry.category());
    query.bindValue(":favorite",entry.favorite()?1:0);

    if (!query.exec())//执行SQL语句
    {
        qDebug() << "插入密码记录失败："
                 << query.lastError().text();

        return false;
    }

    qDebug() << "密码记录插入成功";

    return true;
}

int DatabaseManager::insertEntryAndGetId(const PasswordEntry &entry)
{
    QSqlQuery query;

    query.prepare(R"(
        INSERT INTO password_entries
        (title, username, password, url, notes,category,favorite)
        VALUES
        (:title, :username, :password, :url, :notes,:category,:favorite)
    )");

    query.bindValue(":title", entry.title());
    query.bindValue(":username", entry.username());
    query.bindValue(":password", entry.password());
    query.bindValue(":url", entry.url());
    query.bindValue(":notes", entry.notes());
    query.bindValue(":category",entry.category());
    query.bindValue(":favorite",entry.favorite()?1:0);

    if (!query.exec())
    {
        qDebug() << "插入密码记录失败："
                 << query.lastError().text();

        return -1;
    }

    return query.lastInsertId().toInt();
}

bool DatabaseManager::updateEntry(int id,const PasswordEntry &entry)
{
    QSqlQuery query;

    query.prepare(R"(
        UPDATE password_entries
        SET
            title = :title,
            username = :username,
            password = :password,
            url = :url,
            notes = :notes,
            category = :category,
            favorite = :favorite
        WHERE id = :id
    )");

    query.bindValue(":id", id);
    query.bindValue(":title", entry.title());
    query.bindValue(":username", entry.username());
    query.bindValue(":password", entry.password());
    query.bindValue(":url", entry.url());
    query.bindValue(":notes", entry.notes());
    query.bindValue(":category",entry.category());
    query.bindValue(":favorite",entry.favorite()?1:0);

    if (!query.exec())
    {
        qDebug() << "更新密码记录失败："
                 << query.lastError().text();

        return false;
    }

    return true;
}

bool DatabaseManager::deleteEntry(int id)
{
    QSqlQuery query;

    query.prepare(R"(
        DELETE FROM password_entries
        WHERE id = :id
    )");

    query.bindValue(":id", id);

    if (!query.exec())
    {
        qDebug() << "删除密码记录失败："
                 << query.lastError().text();

        return false;
    }

    return true;
}

QVector<PasswordEntry> DatabaseManager::loadEntries()
{
    QVector<PasswordEntry> entries; //专门用来保存从数据库读取出来的所有密码记录。

    QSqlQuery query;

    QString sql = R"(
        SELECT title, username, password, url, notes,category,favorite
        FROM password_entries
        ORDER BY id
    )";//读取数据

    if (!query.exec(sql))
    {
        qDebug() << "读取密码记录失败："
                 << query.lastError().text();

        return entries;
    }

    while (query.next())
    {
        PasswordEntry entry(
            query.value("title").toString(),//将SQLite数据记录转换成字符（PasswordEntry支持的数据类型）
            query.value("username").toString(),
            query.value("password").toString(),
            query.value("url").toString(),
            query.value("notes").toString(),
            query.value("category").toString(),
             query.value("favorite").toInt() == 1
            );

        entries.append(entry);
    }

    qDebug() << "读取到密码记录数量："
             << entries.size();

    return entries;
}

//函数返回值是QVector<QPair<int, PasswordEntry>>
QVector<QPair<int, PasswordEntry>>DatabaseManager::loadEntriesWithIds()
{
    QVector<QPair<int, PasswordEntry>> entries;//创建临时容器

    QSqlQuery query;//创建数据库查询对象

    QString sql = R"(
        SELECT id, title, username, password, url, notes,category,favorite
        FROM password_entries
        ORDER BY id
    )";

    if (!query.exec(sql))
    {
        qDebug() << "读取密码记录失败："
                 << query.lastError().text();

        return entries;
    }

    while (query.next())
    {
        int id = query.value("id").toInt();//获取数据id，然后转换成int

        PasswordEntry entry(
            query.value("title").toString(),
            query.value("username").toString(),
            query.value("password").toString(),
            query.value("url").toString(),
            query.value("notes").toString(),
            query.value("category").toString(),
            query.value("favorite").toInt() == 1
            );

        entries.append(qMakePair(id, entry));//qMakePair(id, entry)会创建一个QPair<int,PasswordEntry>
    }

    return entries;
}


bool DatabaseManager::replaceAllEntries(
    const QVector<PasswordEntry> &entries,QVector<int> &newIds)
{
    QSqlDatabase database = QSqlDatabase::database();//得到一个数据库连接对象后，就可以调用database.transaction()

    if (!database.isOpen())
    {
        qDebug() << "数据库未打开";

        return false;
    }


    // 开启事务
    if (!database.transaction())
    {
        qDebug() << "无法开启数据库事务："
                 << database.lastError().text();

        return false;
    }


    // =========================
    // 1. 删除数据库中的旧数据
    // =========================

    QSqlQuery deleteQuery(database);

    if (!deleteQuery.exec(
            "DELETE FROM password_entries"
            ))
    {
        qDebug() << "清空旧密码记录失败："
                 << deleteQuery.lastError().text();

        database.rollback();

        return false;
    }


    // 保存重新插入后生成的新数据库ID
    QVector<int> tempIds;


    // =========================
    // 2. 准备INSERT语句
    // =========================

    QSqlQuery insertQuery(database);

    insertQuery.prepare(R"(
        INSERT INTO password_entries
        (
            title,
            username,
            password,
            url,
            notes,
            category,
            favorite
        )
        VALUES
        (
            :title,
            :username,
            :password,
            :url,
            :notes,
            :category,
            :favorite
        )
    )");


    // =========================
    // 3. 插入导入的数据
    // =========================

    for (const PasswordEntry &entry : entries)
    {
        insertQuery.bindValue( ":title",entry.title() );

        insertQuery.bindValue( ":username", entry.username());

        insertQuery.bindValue( ":password",  entry.password());

        insertQuery.bindValue( ":url", entry.url());

        insertQuery.bindValue( ":notes", entry.notes());

        insertQuery.bindValue( ":category", entry.category() );

        insertQuery.bindValue( ":favorite", entry.favorite() ? 1 : 0);


        if (!insertQuery.exec())
        {
            qDebug()
            << "恢复密码记录失败："
            << insertQuery.lastError().text();

            // 出错时撤销本次所有数据库修改
            database.rollback();

            return false;
        }


        // 保存SQLite刚刚生成的ID
        int id =
            insertQuery.lastInsertId().toInt();

        tempIds.append(id);
    }


    // =========================
    // 4. 提交事务
    // =========================

    if (!database.commit())
    {
        qDebug()
        << "提交恢复数据失败："
        << database.lastError().text();

        database.rollback();

        return false;
    }

    // 数据库全部成功以后，
    // 才把新的ID返回给PasswordModel
    newIds = tempIds;

    return true;
}
