#ifndef PASSWORDENTRY_H
#define PASSWORDENTRY_H
//密码条目数据结构，以及允许进行的操作（读取、修改）

#include<QString>
#include<QDialog>

class PasswordEntry
{
public:
    PasswordEntry(); //构造函数
    PasswordEntry(
        const QString &title, //不可以修改引用的内容
        const QString &username,
        const QString &password,
        const QString &url,
        const QString &notes,
        const QString &category,
        bool favorite=false
        ); //构造函数

    //仅获取内容
    QString title() const;
    QString username() const;
    QString password() const;
    QString url() const;
    QString notes() const;
    QString category()const;

    //修改内容
    void setTitle(const QString &title);
    void setUsername(const QString &username);
    void setPassword(const QString &password);
    void setUrl(const QString &url);
    void setNotes(const QString &notes);
    void setFavorite(bool favorite);
    bool favorite()const; //判断该密码条目是否被收藏


private:
    //一条密码记录的结构：名称、用户名、密码、url、备注、分类、是否收藏
    QString m_title;
    QString m_username;
    QString m_password;
    QString m_url;
    QString m_notes;
    QString m_category;
    bool m_favorite = false;
};

#endif // PASSWORDENTRY_H
