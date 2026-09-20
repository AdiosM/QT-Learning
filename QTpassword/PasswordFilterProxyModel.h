#ifndef PASSWORDFILTERPROXYMODEL_H
#define PASSWORDFILTERPROXYMODEL_H

//专门负责：搜索、分类筛选、收藏筛选、排序。不保存真正的数据。实现搜索和分类过滤。
//决定哪些数据要被显示出来。
/***
 * PasswordModel
        ↓
 * PasswordFilterProxyModel
 *      ↓
 *   QTableView
 ***/



#include <QSortFilterProxyModel>

class PasswordFilterProxyModel:public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit PasswordFilterProxyModel(QObject *parent = nullptr);

    void setCategoryFilter(const QString &category);
    void setFavoriteOnly(bool favoriteOnly);

protected:
    bool filterAcceptsRow(int sourceRow,const QModelIndex &sourceParent)const override;

private:
    QString currentCategory;//保存用户点击的分类
    bool m_favoriteOnly = false;
};

#endif // PASSWORDFILTERPROXYMODEL_H
