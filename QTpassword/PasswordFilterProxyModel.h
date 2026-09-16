#ifndef PASSWORDFILTERPROXYMODEL_H
#define PASSWORDFILTERPROXYMODEL_H

//专门负责密码搜索逻辑的代理模型。不保存数据

#include <QSortFilterProxyModel>

class PasswordFilterProxyModel:public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit PasswordFilterProxyModel(QObject *parent = nullptr);//

    void setCategoryFilter(const QString &category);
    void setFavoriteOnly(bool favoriteOnly);

protected:
    bool filterAcceptsRow(int sourceRow,const QModelIndex &sourceParent)const override;

private:
    QString currentCategory;//保存用户点击的分类
    bool m_favoriteOnly = false;
};

#endif // PASSWORDFILTERPROXYMODEL_H
