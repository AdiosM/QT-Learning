#include "mainwindow.h"
#include "EntryDialog.h"
#include "PasswordModel.h"
#include "BackupManager.h"

#include <QSplitter>
#include <QTreeWidget>
#include <QTableView>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QFont>
#include <QTextEdit>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QMenuBar>
#include <QFileDialog>

#include<QDebug>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("QtPassword");
    resize(1100,720);
    setMinimumSize(900,600);//设置最小尺寸

    setupUi();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi()//场景程序主界面
{
    //创建中央控件
    QWidget *centralWidget=new QWidget(this);
    setCentralWidget(centralWidget);

    //创建主分割器
    mainSplitter = new QSplitter(Qt::Horizontal,centralWidget);

    //创建左侧面板和右侧面板---------
    leftPanel = new QWidget;
    rightPanel = new QWidget;

    //将左右两个面板加入到主分割器中
    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(rightPanel);
    /***
┌───────────┬─────────────────┐
│           │                 │
│   左侧     │      右侧       │
│           │                 │
└───────────┴─────────────────┘
            ↕
          可以拖动
     * **/


    // 设置 QSplitter 初始大小
    mainSplitter->setSizes( { 250, 750 } );//第一个参数控制左侧区域的初始宽度

    // 创建整个中央区域的布局
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);//水平布局
    mainLayout->addWidget(mainSplitter);//将 splitter 添加到中央布局。

    //左侧面板-----------
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);//创建左侧垂直布局

    //搜索框
    searchEdit=new QLineEdit;
    searchEdit->setPlaceholderText("🔍 搜索密码..."); //设置提示文字
    searchEdit->setClearButtonEnabled(true);
    leftLayout->addWidget(searchEdit);//加入到左侧布局

    //分组树
    groupTree=new QTreeWidget(leftPanel);//将目录树创建在 leftPanel 内部，groupTree 是 leftPanel 的子控件，当 leftPanel 移动、隐藏或销毁时，groupTree 会跟随
    groupTree->setColumnCount(1);//设置 QTreeWidget 的列数,1列
    groupTree->setHeaderHidden(true);//设置这一列的标题，不希望显示列标题，设置了一个空字符

    //创建树节点----
    QTreeWidgetItem *allItem = new QTreeWidgetItem(groupTree);
    allItem->setText(0,"📁全部"); //全部，QTreeWidgetItem是QTreeWidget中的一个项目/节点

    // 收藏
    QTreeWidgetItem *favoriteItem = new QTreeWidgetItem(groupTree);
    favoriteItem->setText(0, "⭐ 收藏");

    // 工作
    QTreeWidgetItem *workItem = new QTreeWidgetItem(groupTree);
    workItem->setText(0, "📂 工作");

    // 学习
    QTreeWidgetItem *studyItem = new QTreeWidgetItem(groupTree);
    studyItem->setText(0, "📂 学习");

    // 网站
    QTreeWidgetItem *websiteItem = new QTreeWidgetItem(groupTree);
    websiteItem->setText(0, "📂 网站");

    // 软件
    QTreeWidgetItem *softwareItem = new QTreeWidgetItem(groupTree);
    softwareItem->setText(0, "📂 软件");
    leftLayout->addWidget(groupTree);//将分组树加入左侧布局
    //---------

    //创建 “+ 新建” 按钮
    newButton = new QPushButton("＋ 新建");
    leftLayout->addWidget(newButton);

    //左侧面板--------------


    //创建右侧面板----------
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);//添加到垂直布局中

    //创建“密码列表”标题
    QLabel *listTitleLabel = new QLabel("密码列表", rightPanel);

    //设置字体，
    QFont listFont = listTitleLabel->font();
    listFont.setPointSize(14);
    listFont.setBold(true);
    listTitleLabel->setFont(listFont);

    rightLayout->addWidget(listTitleLabel);// 把密码列表标题添加到右侧布局。

    //======创建密码表格=======
    passwordModel = new PasswordModel(this); //MainWindow是PasswordModel的父对象，这样关闭主窗口会自动释放资源
    proxyModel = new PasswordFilterProxyModel(this);
    proxyModel->setSourceModel(passwordModel);//设置proxyModel的数据来源--passwordModel
    passwordTable = new QTableView(rightPanel);
    passwordTable->setModel(proxyModel);

    //表格显示设置
    passwordTable->horizontalHeader() ->setSectionResizeMode(QHeaderView::Stretch);//设置表格列宽策略，horizontalHeader()：获取水平表头对象。
    //QHeaderView::Stretch：让每一列自动拉伸，尽量填满整个表格宽度。
    passwordTable->setEditTriggers( QAbstractItemView::NoEditTriggers );//禁止直接编辑表格中的内容
    passwordTable->setSelectionBehavior( QAbstractItemView::SelectRows );//设置选择方式，用户点击单元格时，整行会被选中
    passwordTable->setSelectionMode(QAbstractItemView::SingleSelection);//

     rightLayout->addWidget(passwordTable);//把密码表格加入右侧布局

     //==========密码详情==========
     detailTitleLabel = new QLabel("密码详情", rightPanel);//创建“密码详情”标题

     //设置详情标题字体
     QFont detailFont = detailTitleLabel->font();
     detailFont.setPointSize(14);
     detailFont.setBold(true);
     detailTitleLabel->setFont(detailFont);

     rightLayout->addWidget(detailTitleLabel);//将详情标题加入右侧布局

     //====创建详情区域控件
     detailNameLabel = new QLabel("名称：", rightPanel);//名称
     detailUsernameLabel = new QLabel("用户名：", rightPanel);
     detailPasswordLabel = new QLabel("密码：", rightPanel);
     showPasswordButton = new QPushButton("显示密码",rightPanel);
     copyPasswordButton = new QPushButton("复制密码",rightPanel);
     detailUrlLabel = new QLabel("URL：", rightPanel);
     detailNotesLabel = new QLabel("备注：", rightPanel);

     detailNotesLabel->setWordWrap(true);//设置QLabel显示方式,文字太长时自动换行

     //==========密码区域水平布局==========
     QHBoxLayout *passwordLayout = new QHBoxLayout;
     passwordLayout->addWidget(detailPasswordLabel);
     passwordLayout->addWidget(showPasswordButton);
     passwordLayout->addWidget(copyPasswordButton);
     passwordLayout->addStretch();


     //==========将详情控件加入右侧布局==========
     rightLayout->addWidget(detailNameLabel);
     rightLayout->addWidget(detailUsernameLabel);
     // 密码 + 显示密码按钮
     rightLayout->addLayout(passwordLayout);
     rightLayout->addWidget(detailUrlLabel);
     rightLayout->addWidget(detailNotesLabel);

     //在右侧面板的下方增加两个按钮，“编辑”和“删除”
     QHBoxLayout *actionLayout = new QHBoxLayout;
     favoriteButton =new QPushButton("收藏",rightPanel);
     editButton = new QPushButton("编辑",rightPanel);
     deleteButton = new QPushButton("删除",rightPanel);
     actionLayout->addWidget(favoriteButton);
     actionLayout->addWidget(editButton);
     actionLayout->addWidget(deleteButton);
     rightLayout->addLayout(actionLayout);


     rightLayout->addStretch();//添加一个弹簧，让上面的内容尽可能保持在顶部
     rightPanel->setMinimumHeight(300);//右侧面板最小高度

     //===增加菜单，文件导入与导出
     QMenu *fileMenu = menuBar()->addMenu("文件");
     QAction *exportAction = fileMenu->addAction("导出备份");
     QAction *importAction = fileMenu->addAction("导入备份");
     connect(exportAction,&QAction::triggered,this,&MainWindow::exportBackup);
     connect(importAction,&QAction::triggered,this,&MainWindow::importBackup);


    connect(newButton,&QPushButton::clicked,this,&MainWindow::createNewEntry);// "+ 新建" 按钮的点击事件
    connect( editButton, &QPushButton::clicked, this, &MainWindow::editEntry );//“编辑”按钮
    connect( deleteButton, &QPushButton::clicked, this, &MainWindow::deleteEntry );//“删除”按钮
    connect(showPasswordButton,&QPushButton::clicked,this,[this]()
            {
        passwordVisible = !passwordVisible;

        if (passwordVisible)
        {
            detailPasswordLabel->setText("密码：" + currentPassword);
            showPasswordButton->setText( "隐藏密码" );
        }
        else
        {
            detailPasswordLabel->setText( "密码：******");
            showPasswordButton->setText( "显示密码" );
        }
        });//点击显示密码按钮后，触发此链接

    connect(copyPasswordButton,&QPushButton::clicked,this,
            [this]()
            {
                if(currentPassword.isEmpty()){return;}
                QApplication::clipboard()->setText(currentPassword);
                QMessageBox::information(this,"提示","密码已复制到剪贴板");
            }
            );

    connect(searchEdit,&QLineEdit::textChanged,this,[this](const QString &text)
    {
        proxyModel->setFilterFixedString(text);
    });

    connect(groupTree,&QTreeWidget::itemClicked,this,&MainWindow::categoryClicked);

    connect(favoriteButton,&QPushButton::clicked,this,&MainWindow::toggleFavorite);

    //用户选择一个密码条目后，在密码详情部分显示密码信息
    connect( passwordTable, &QTableView::clicked, this,&MainWindow::showEntryDetails);
    //QTableView::clicked 信号的参数是const QModelIndex &index

    //双击密码条目，直接进入编辑状态
    connect(passwordTable,&QTableView::doubleClicked,this,[this](const QModelIndex &index)
    {
        if(index.isValid()){editEntry();}
     } );



}

void MainWindow::createNewEntry() //打开“新建密码”对话框
{
    EntryDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted)
    {
        PasswordEntry entry(
            dialog.title(),
            dialog.username(),
            dialog.password(),
            dialog.url(),
            dialog.notes(),
            dialog.category()
            );
        passwordModel->addEntry(entry);

        int row = passwordModel->rowCount() - 1;

        passwordTable->setCurrentIndex(passwordModel->index(row,0));
    }
}


void MainWindow::showEntryDetails(const QModelIndex &index)
{
    if (!index.isValid())return;
    QModelIndex sourceIndex = proxyModel->mapToSource(index);
    int row = sourceIndex.row();

    const PasswordEntry &entry = passwordModel->entryAt(row);

    if(entry.favorite())
    {favoriteButton->setText("取消收藏");}
    else
    {favoriteButton->setText("收藏");}

    detailNameLabel->setText( "名称：" + entry.title());
    detailUsernameLabel->setText( "用户名：" + entry.username());

    currentPassword = entry.password();
    passwordVisible =false;
    detailPasswordLabel->setText( "密码：*******" );
    showPasswordButton->setText("显示密码");

    detailUrlLabel->setText( "URL：" + entry.url() );
    detailNotesLabel->setText( "备注：" + entry.notes());
}


void MainWindow::editEntry()
{
    QModelIndex currentIndex = passwordTable->currentIndex();
    if (!currentIndex.isValid()) return;

    QModelIndex sourceIndex = proxyModel->mapToSource(currentIndex);
    int row = sourceIndex.row();//currentIndex.row()是Proxy的行，sourceIndex.row()才是PasswordModel的真实行

    const PasswordEntry &entry = passwordModel->entryAt(row);//将用户当前点击的某一行条目信息赋值到entry中

    EntryDialog dialog(this);//创建一个编辑对话框（右侧面板下方的“密码详情”部分）
    dialog.setTitle(entry.title());//将当前选中的条目的已有的密码记录的标题，显示到编辑对话框里
    dialog.setUsername(entry.username());
    dialog.setPassword(entry.password());
    dialog.setUrl(entry.url());
    dialog.setNotes(entry.notes());
    dialog.setCategory(entry.category());

    //用户点击编辑按钮后，会弹出一个对话框
    if (dialog.exec() == QDialog::Accepted)//获取输入框中的字符，然后更新输入框中的字符（覆盖原来的字符），即给titleEdit变量赋值
    {
        PasswordEntry newEntry(
            dialog.title(), dialog.username(),
            dialog.password(), dialog.url(),
            dialog.notes(),dialog.category());//更新这个密码条目信息

        passwordModel->updateEntry( row, newEntry );//更新数据表

        QModelIndex newSourceIndex = passwordModel->index(row,0);
        QModelIndex newProxyIndex = proxyModel->mapFromSource(newSourceIndex);

        passwordTable->setCurrentIndex(newProxyIndex);
    }
}

void MainWindow::deleteEntry()
{
    QModelIndex currentIndex = passwordTable->currentIndex();//获取用户选中的条目的坐标
    if(!currentIndex.isValid())return;

    int proxyRow = currentIndex.row();

    QModelIndex sourceIndex = proxyModel->mapToSource(currentIndex);
    int sourceRow =sourceIndex.row();//获取当前选中的行

    QMessageBox::StandardButton result =
        QMessageBox::question(
        this, "确认删除", "确定要删除这条密码记录吗？",
        QMessageBox::Yes | QMessageBox::No );

    if (result == QMessageBox::Yes)
    {
        passwordModel->removeEntry(sourceRow);
        if (proxyModel->rowCount() > 0)//删除一条记录后，如果当前表格还有记录，就自动选择其中一条新的记录，并在详情列表中显示
        {
            int newRow = qMin(proxyRow,proxyModel->rowCount()-1);//在 proxyRow 和“当前最后一行的行号”之间，取较小的那个，作为删除后要重新选中的行。
//qMin(a,b)是Qt提供的取较小值的函数

            QModelIndex newIndex = proxyModel->index(newRow,0);

            passwordTable->setCurrentIndex(newIndex);

            showEntryDetails(newIndex);//显示newIndex这一行的密码信息
        }
        else
        {
            detailNameLabel->setText( "名称："  );
            detailUsernameLabel->setText( "用户名：" );
            detailPasswordLabel->setText( "密码：" );
            detailUrlLabel->setText( "URL："  );
            detailNotesLabel->setText( "备注：" );

            currentPassword.clear();
            passwordVisible = false;

            showPasswordButton->setText("显示密码");
        }
    }
}

void MainWindow::categoryClicked(QTreeWidgetItem *item,int column)
{
    QString text = item->text(0);

    if(text.contains("全部"))
    {
         proxyModel->setFavoriteOnly(false);
        proxyModel->setCategoryFilter("");
    }
    else if(text.contains("收藏"))
    {
        proxyModel->setCategoryFilter("");
        proxyModel->setFavoriteOnly(true);
    }
    else if(text.contains("网站"))
    {
        proxyModel->setFavoriteOnly(false);
        proxyModel->setCategoryFilter( "网站");
    }
    else if(text.contains("软件"))
    {
        proxyModel->setFavoriteOnly(false);
        proxyModel->setCategoryFilter( "软件");
    }
    else if(text.contains("工作"))
    {
        proxyModel->setFavoriteOnly(false);
        proxyModel->setCategoryFilter( "工作");
    }
    else if(text.contains("学习"))
    {
        proxyModel->setFavoriteOnly(false);
        proxyModel->setCategoryFilter("学习");
    }
}

void MainWindow::toggleFavorite()
{
    QModelIndex currentIndex =
        passwordTable->currentIndex();

    if (!currentIndex.isValid())
    {
        return;
    }

    QModelIndex sourceIndex =
        proxyModel->mapToSource(currentIndex);

    int row = sourceIndex.row();

    passwordModel->toggleFavorite(row);

    const PasswordEntry &entry =
        passwordModel->entryAt(row);

    if (entry.favorite())
    {
        favoriteButton->setText("取消收藏");
    }
    else
    {
        favoriteButton->setText("收藏");
    }
}

void MainWindow::exportBackup() //导出备份函数
{

    QString filePath =
        QFileDialog::getSaveFileName(
            this,
            "保存备份",
            "",
            "JSON (*.json)"
            );

    if(filePath.isEmpty()) { return;}

    if(BackupManager::exportToJson(filePath,passwordModel->getEntries()) )
    {
        QMessageBox::information( this,"提示", "备份成功");
    }
}

void MainWindow::importBackup()//导入备份函数
{

    QString filePath =
        QFileDialog::getOpenFileName(
            this,
            "选择备份文件",
            "",
            "JSON (*.json)"
            );


    if(filePath.isEmpty()){return;}

    QVector<PasswordEntry> entries =
        BackupManager::importFromJson(filePath);

    if (passwordModel->replaceEntries(entries))
    {
        QMessageBox::information(
            this,
            "提示",
            "备份恢复成功"
            );
    }
    else
    {
        QMessageBox::warning(
            this,
            "错误",
            "备份恢复失败"
            );
    }
}
