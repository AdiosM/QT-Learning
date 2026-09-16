#include "PasswordFilterProxyModel.h"

PasswordFilterProxyModel::PasswordFilterProxyModel(QObject *parent):QSortFilterProxyModel(parent)
{    }

bool PasswordFilterProxyModel::filterAcceptsRow(
    int sourceRow,
    const QModelIndex &sourceParent ) const //作用：Qt每次拿一行数据过来，问我们：“这一行要不要显示？
{
    //sourceRow：第几行。0:第0列。获取密码列表中的列
    QModelIndex titleIndex =sourceModel()->index(sourceRow, 0, sourceParent);
    QModelIndex usernameIndex =sourceModel()->index(sourceRow, 1, sourceParent);
    QModelIndex urlIndex =sourceModel()->index(sourceRow, 2, sourceParent);
    QModelIndex notesIndex =sourceModel()->index(sourceRow, 3, sourceParent);
    QModelIndex categoryIndex = sourceModel()->index(sourceRow,4,sourceParent);

    QString title = sourceModel()->data(titleIndex).toString(); //获取列（表格单元格）中的内容（名称）
    QString username = sourceModel()->data(usernameIndex).toString();//sourceModel()->data(usernameIndex)的意思是向PasswordModel询问第sourceRow 行、第 0 列是什么数据？
    QString url = sourceModel()->data(urlIndex).toString();//sourceModel()->data()意思是去PasswordModel里面拿数据
    QString notes = sourceModel()->data(notesIndex).toString();
    QString category = sourceModel()->data(categoryIndex).toString();

    //QString keyword =filterRegularExpression().pattern();
    //搜索注释（notes）时，keyword输出的是"\\邮\\箱",而不是"邮箱",导致无法匹配

    QRegularExpression regex = filterRegularExpression();

/***调试代码
    qDebug() << regex;
    qDebug() << notes;
    qDebug() << notes.contains(regex);
***/

    //用户没有选择分类，则允许显示；否则，密码分类必须等于选择分类才允许显示
    bool favorite=sourceModel()->data(titleIndex,Qt::UserRole).toBool();

    bool categoryMatch = currentCategory.isEmpty()|| category == currentCategory;

    bool searchMatch =
        regex.match(title).hasMatch()
        || regex.match(username).hasMatch()
        || regex.match(url).hasMatch()
        || regex.match(notes).hasMatch();

    bool favoriteMatch = !m_favoriteOnly || favorite;

    return categoryMatch && searchMatch && favoriteMatch;

}

void PasswordFilterProxyModel::setCategoryFilter(const QString &category)
{
    currentCategory=category;

    invalidateFilter();//通知Qt，重新执行filterAcceptsRow()
}

void PasswordFilterProxyModel::setFavoriteOnly( bool favoriteOnly)
{
    m_favoriteOnly = favoriteOnly;

    invalidateFilter();
}