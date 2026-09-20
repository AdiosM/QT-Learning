#include "PasswordModel.h"


PasswordModel::PasswordModel(QObject *parent)
    : QAbstractTableModel(parent)
{//用于存储表格模型数据
    if(!databaseManager.openDatabase())
    {
        return;
    }

    if(!databaseManager.createTables())
    {
        return;
    }
    loadFromDatabase();
}

int PasswordModel::rowCount( const QModelIndex &parent)const
{
    Q_UNUSED(parent);

    return entries.size();
}

int PasswordModel::columnCount(const QModelIndex &parent)const
{
    Q_UNUSED(parent);

    return 5;//5列，我们自己设计的，是固定的
}

QVariant PasswordModel::data( const QModelIndex &index,int role)const
{//负责回答：“某一行，某一列的数据是什么”
    if (!index.isValid())//判断数据位置的有效性，
    {//无效则返回一个空的QVariant
        return QVariant();
    }

    if(role==Qt::UserRole)//该密码条目是否被收藏
    {
        return entries[index.row()].favorite();//返回一个bool值
    }

    if (role != Qt::DisplayRole)//DisplayRole是Qt自带的枚举值
    {//如果调用者请求的不是“用于显示的数据”，并且前面也不是 UserRole，那么这个 Model 不处理。
        return QVariant();//返回空数值
    }

    const PasswordEntry &entry = entries[index.row()];//获取这一行的密码条目信息

    switch (index.column())
    {
    case 0:
        return entry.title();

    case 1:
        return entry.username();

    case 2:
        return entry.url();

    case 3:
        return entry.notes();
    case 4:
        return entry.category();

    default:
        return QVariant();
    }
}

QVariant PasswordModel::headerData(int section,Qt::Orientation orientation,int role)const
{
    if (role != Qt::DisplayRole)
    {
        return QVariant();
    }

    if (orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case 0:
            return "名称";

        case 1:
            return "用户名";

        case 2:
            return "网址";

        case 3:
            return "备注";
        case 4:
            return "分类";

        default:
            return QVariant();
        }
    }

    return QVariant();
}

void PasswordModel::addEntry(const PasswordEntry &entry)
{
    int id = databaseManager.insertEntryAndGetId(entry);

    if(id<0){return;}

    int row = entries.size();

    beginInsertRows( QModelIndex(), row, row );//向Model中插入一行，现在还没添加，只是通知

    entries.append(entry);//正式添加
    entryIds.append(id);

    endInsertRows();//插入完成
}

void PasswordModel::updateEntry(int row,const PasswordEntry &entry)
{
    if (row < 0 || row >= entries.size()){ return; }

    int id = entryIds[row];//获取当前这条记录对应的数据库主键ID
    if (!databaseManager.updateEntry(id, entry))
    {
        return;
    }

    entries[row] = entry;//更新数据
    emit dataChanged(//QT自带的信号，由QAbstractItemModel提供
        index(row, 0), index(row, columnCount() - 1),
        {Qt::DisplayRole} );
}

void PasswordModel::removeEntry(int row)
{
    if (row < 0 || row >= entries.size()) { return; }

    int id = entryIds[row];
    if(!databaseManager.deleteEntry(id)){return;}

    beginRemoveRows( QModelIndex(), row, row );
    entries.removeAt(row);
    entryIds.removeAt(row);//删除对应Id
    endRemoveRows();
}

const PasswordEntry& PasswordModel::entryAt(int row) const
{
    static PasswordEntry empty;
    if (row < 0 || row >= entries.size()) { return empty; }

    return entries[row];
}

void PasswordModel::loadFromDatabase()
{
    //临时保存从SQLite数据库读取出来的所有密码记录
    QVector<QPair<int, PasswordEntry>> databaseEntries =
        databaseManager.loadEntriesWithIds();

    beginResetModel();//Qt提供的函数，告诉 Qt：Model马上要进行一次整体重置。与endResetModel()配合使用
//它只是向Qt发出一个信号，告诉Qt数据要发生大改变

    entries.clear();
    entryIds.clear();

    for (const auto &item : databaseEntries)
    {
        entryIds.append(item.first);
        entries.append(item.second);
    }

    endResetModel();//Qt提供的函数，告诉Qt数据修改完成
}


bool PasswordModel::toggleFavorite(int row)
{
    if (row < 0 || row >= entries.size())
    {
        return false;
    }

    PasswordEntry newEntry = entries[row];

    newEntry.setFavorite(
        !newEntry.favorite()
        );

    int id = entryIds[row];

    if (!databaseManager.updateEntry(id, newEntry))
    {
        return false;
    }

    entries[row] = newEntry;

    emit dataChanged(
        index(row, 0),
        index(row, columnCount() - 1),
        {Qt::DisplayRole}
        );

    return true;
}


QVector<PasswordEntry>PasswordModel::getEntries() const
{
    return entries;
}

bool PasswordModel::replaceEntries(const QVector<PasswordEntry>&newEntries)
{
    QVector<int>newIds;

     // 先真正更新SQLite数据库
    if(!databaseManager.replaceAllEntries(newEntries,newIds))
    {
        return false;
    }

    // 数据库成功以后，
    // 再修改PasswordModel中的内存数据
    beginResetModel();

    entries = newEntries;
    entryIds = newIds;

    endResetModel();

    return true;
}
