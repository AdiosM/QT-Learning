// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>
//这个文件包含了所有Widgets模块的头文件，会使程序变慢，不推荐全部包含，最好是用了什么就包含什么

#include "screenshot.h"

//! [0]
Screenshot::Screenshot() //构造函数，程序启动时执行的第一个函数
    :  screenshotLabel(new QLabel(this))//在函数体执行前，先创建了一个QLabel控件，this表示QLabel的父对象是当前窗口(初始化screenshotLabel)
{//this的类型是Screenshot*,是指向 Screenshot 对象的指针
    //设置screenshotLabel的属性
    screenshotLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);//让标签尽可能占满可用空间
    //setSizePolicy()函数是继承自QWidget类，Expanding是QSizePolicy中的枚举类
    screenshotLabel->setAlignment(Qt::AlignCenter);//让图片在标签中居中显示，靠左显示：AlignLeft
/***
     在 Qt 中，几乎所有的可视控件都遵循以下继承结构：
        QLabel -> QFrame -> QWidget -> QObject
***/

    const QRect screenGeometry = screen()->geometry();//screen()函数是QWidget类的一个成员函数，这是获取屏幕尺寸信息的经典写法
    //screen()函数的作用是返回当前窗口所在的屏幕对象，类型为QScreen*。Qt会自动判断你的窗口当前落在哪个物理屏幕上，并返回对应的 QScreen 指针。
    //->geometry()是获取该屏幕的完整几何区域（包括任务栏等系统区域），返回一个 QRect 对象。
/***
 * =screen()->geometry()这个写法相当于 =this->screen()->geometry()
 * 因为Screenshot类继承自QWidget,screen()也是QWidget类的成员函数
 * 当你在类的成员函数（比如构造函数）内部直接调用screen()时，编译器会自动把它理解为this->screen()
 ***/
    screenshotLabel->setMinimumSize(screenGeometry.width() / 8, screenGeometry.height() / 8);//根据屏幕大小设置一个最小尺寸，防止显示图片的区域缩得太小看不见

    //布局管理，这里使用了嵌套布局
    //QVBoxLayout (主垂直布局): 包含预览标签、选项组和按钮行
    //QGridLayout (网格布局): 用于“Options”组框内部，整齐排列延迟输入框和复选框
    //QHBoxLayout (水平布局): 用于底部的三个按钮

    QVBoxLayout *mainLayout = new QVBoxLayout(this);//参数this表明这个布局设置为当前窗口(Screenshot)的主布局。这意味着以后所有添加到 mainLayout 的东西都会自动显示在窗口里。
    mainLayout->addWidget(screenshotLabel);//将screenshotLabel截图标签放入布局的第一行

    QGroupBox *optionsGroupBox = new QGroupBox(tr("Options"), this);//QGroupBox: 这是一个带有标题边框的容器控件。它在视觉上把相关的设置项（延迟、隐藏窗口）圈在一起，让界面更有条理。
    //tr()是Qt的国际化翻译函数。它允许用户通过加载不同的语言文件，将 "Options" 翻译成中文“选项”或其他语言。
    delaySpinBox = new QSpinBox(optionsGroupBox);//创建一个数字输入框，并将父对象设置为optionsGroupBox
    delaySpinBox->setSuffix(tr(" s"));//给数字后面加一个单位“秒”。
    delaySpinBox->setMaximum(60);//限制用户最多只能延迟60秒
//delaySpinBox->setMaximumWidth(80);//设置这个数字输入框的最大宽度为80像素

    connect(delaySpinBox, &QSpinBox::valueChanged,
            this, &Screenshot::updateCheckBox);// 当用户修改延迟秒数时，触发更新复选框状态的逻辑。
//如果用户把延迟设为 0，程序会立刻截图，来不及隐藏窗口。所以这个连接是为了在延迟为 0 时，自动禁用“隐藏窗口”复选框。
    hideThisWindowCheckBox = new QCheckBox(tr("Hide This Window"), optionsGroupBox);//创建与一个复选框
//让用户决定在倒计时结束、真正截图的那一瞬间，是否要把这个截图工具自己的窗口藏起来，以免截到工具本身的画面。

    QGridLayout *optionsGroupBoxLayout = new QGridLayout(optionsGroupBox);//创建一个网格布局管理器，这个布局属于optionsGroupBox。
    optionsGroupBoxLayout->addWidget(new QLabel(tr("Screenshot Delay:"), this), 0, 0);//在“选项”组框的网格布局中，第 0 行、第 0 列的位置，放置一个显示文字“Screenshot Delay:”的标签。
    //这里的this先当与当前的Screenshot窗口；这里的第二个参数this表示父对象
    optionsGroupBoxLayout->addWidget(delaySpinBox, 0, 1);
    optionsGroupBoxLayout->addWidget(hideThisWindowCheckBox, 1, 1, 1, 2);
//(1,0,1,2)表示(行坐标，列坐标，跨行数，跨列数)
//1 (Row): 放在第 1 行（注意：Qt 的行列是从 0 开始计数的，所以这是第二行）。
//0 (Column): 放在第 0 列（最左边）
//1 (Row Span): 占据 1 行的高度。
//2 (Column Span): 占据 2 列的宽度。

    mainLayout->addWidget(optionsGroupBox);//将选项边框(optionsGroupBox)加入到当前窗口中布局中

    QHBoxLayout *buttonsLayout = new QHBoxLayout;//创建水平布局管理器，之后添加进去的控件都会从左到右依次排列。
    //这里没有传 this 作为参数，因为它只是一个临时的布局工具，稍后会被添加到主布局 mainLayout 中。
    newScreenshotButton = new QPushButton(tr("New Screenshot"), this);//按钮，父对象为当前窗口
    connect(newScreenshotButton, &QPushButton::clicked, this, &Screenshot::newScreenshot); //点击按钮开始截图流程
    buttonsLayout->addWidget(newScreenshotButton);
    QPushButton *saveScreenshotButton = new QPushButton(tr("Save Screenshot"), this);
    connect(saveScreenshotButton, &QPushButton::clicked, this, &Screenshot::saveScreenshot);
    buttonsLayout->addWidget(saveScreenshotButton);
    QPushButton *quitScreenshotButton = new QPushButton(tr("Quit"), this);
    quitScreenshotButton->setShortcut(Qt::CTRL | Qt::Key_Q);
    connect(quitScreenshotButton, &QPushButton::clicked, this, &QWidget::close);
    buttonsLayout->addWidget(quitScreenshotButton);
    buttonsLayout->addStretch();//它在三个按钮的右侧添加了一个“弹簧”。由于弹簧会尽可能占据剩余空间，它会把左边的三个按钮挤到窗口的最左侧。如果没有这一行，三个按钮会均匀分散在整个窗口底部。
    mainLayout->addLayout(buttonsLayout);//将这个水平布局整体加入到垂直布局中，使其显示在界面的最下方。（在垂直布局中，控件和子布局是按照添加的先后顺序从上到下依次排列的。）

    shootScreen();//真正的截图动作
    delaySpinBox->setValue(5);//设置初始值

    setWindowTitle(tr("Screenshot-屏幕截图"));
    resize(300, 200);//调整程序窗口大小
}
//! [0]

//! [1]
void Screenshot::resizeEvent(QResizeEvent * /* event */)//响应窗口拉伸
{//当用户拖动窗口边缘改变大小时，这个函数会被触发

    QSize scaledSize = originalPixmap.size();
    scaledSize.scale(screenshotLabel->size(), Qt::KeepAspectRatio);//保持比例：它计算新的缩放尺寸，确保图片在放大或缩小窗口时不会变形
    if (scaledSize != screenshotLabel->pixmap().size())
        updateScreenshotLabel();
}
//! [1]

//! [2]
void Screenshot::newScreenshot()//倒计时逻辑
{
    if (hideThisWindowCheckBox->isChecked())
        hide();//如果用户勾选了Hide this window,就先调用hide()把程序窗口藏起来，以免截图截到程序自己
    newScreenshotButton->setDisabled(true);//将按钮变灰，不可点击。防止按钮重复点击

    QTimer::singleShot(delaySpinBox->value() * 1000, this, &Screenshot::shootScreen);
    //这是一个静态函数，它会在指定的毫秒后自动执行一次shootScreen。乘以1000是转换为毫秒
}
//! [2]

//! [3]
void Screenshot::saveScreenshot()//保存图片
{
    const QString format = "png";
    QString initialPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    //智能路径，自动找到系统的“图片”文件夹。QStandardPaths: 这是一个非常实用的工具类，它能跨平台地获取系统标准目录（如桌面、文档、图片等）。这里它自动找到了系统的“图片”文件夹。
    if (initialPath.isEmpty())//如果找不到图片文件夹，就退回到程序当前运行的目录
        initialPath = QDir::currentPath();
    initialPath += tr("/untitled.") + format;//拼接出一个默认的文件名 untitled.png，方便用户直接点击保存。

    QFileDialog fileDialog(this, tr("Save As"), initialPath);//文件对话框，QFileDialog 提供了原生的保存界面，支持选择格式（PNG, JPG 等）
//this:指定父对象为当前的 Screenshot 窗口。这确保了对话框是模态的（Modal），即它会挡在主窗口前面，且位置会相对于主窗口居中。
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);//设置接收模式，告诉对话框这是一个“保存操作”，而不是“打开”操作
    fileDialog.setFileMode(QFileDialog::AnyFile);//设置文件模式，允许用户选择任何文件，包括那些还不存在的文件。因为保存截图时，会重新命名，这个名称通常是不存在的
    fileDialog.setDirectory(initialPath);//再次确认目录，虽然在构造函数里已经传了 initialPath，但显式调用 setDirectory 是一种双重保险，确保对话框一定会定位到我们想要的文件夹。
    QStringList mimeTypes;//这是一个字符串列表，用来存放接下来要动态获取的所有支持的图像格式
    const QList<QByteArray> baMimeTypes = QImageWriter::supportedMimeTypes();//获取Qt当前支持的所有图像格式
    for (const QByteArray &bf : baMimeTypes)
        mimeTypes.append(QLatin1String(bf));
    fileDialog.setMimeTypeFilters(mimeTypes);//在‘保存类型’的下拉菜单里，只显示这些 MIME 类型对应的格式
    fileDialog.selectMimeTypeFilter("image/" + format);//自动在下拉菜单中选中我们定义的默认格式（这里是 "image/png"）。
    fileDialog.setDefaultSuffix(format);//自动补全后缀，如果用户在文件名输入框里只写了 myphoto 而忘了加 .png，Qt 会自动帮他在后面补上 .png
    if (fileDialog.exec() != QDialog::Accepted)
        return;
    const QString fileName = fileDialog.selectedFiles().first();
    if (!originalPixmap.save(fileName)) { //保存操作，将内存中的图形数据写入硬盘
        QMessageBox::warning(this, tr("Save Error"), tr("The image could not be saved to \"%1\".")
                             .arg(QDir::toNativeSeparators(fileName)));
    }
}
//! [3]

//! [4]
void Screenshot::shootScreen()//截图
{
    QScreen *screen = QGuiApplication::primaryScreen();//获取主显示器。如果窗口在多屏环境下，它会尝试获取窗口所在的屏幕
//通过 windowHandle() 获取当前窗口所在的物理屏幕。如果你把程序拖到副屏上运行，它会截取副屏的内容，而不是主屏。
    if (const QWindow *window = windowHandle())//这是一个在 C++17 及更高版本中非常流行的初始化语句（Init-statement）写法，它在一个 if 条件中同时完成了变量声明、赋值和非空判断。
        screen = window->screen();//获取当前程序窗口所在的那个物理屏幕对象。
//window这个对象只在if语句块内有效，不会污染外部命名空间
//windowHandle()是QWidget的成员函数，返回当前窗口对应的底层平台窗口对象指针
    if (!screen)
        return;

    if (delaySpinBox->value() != 0)
        QApplication::beep();//反馈提示：如果设置了延迟，会调用 QApplication::beep() 发出提示音

    originalPixmap = screen->grabWindow(0);//参数0代表整个屏幕桌面。它返回一个QPixmap对象并存储在originalPixmap中
    updateScreenshotLabel();//更新显示：调用 updateScreenshotLabel() 将截到的图缩放后显示在界面上

    newScreenshotButton->setDisabled(false);
    if (hideThisWindowCheckBox->isChecked())
        show();
}
//! [4]

//! [6]
void Screenshot::updateCheckBox()
{
    if (delaySpinBox->value() == 0) {
        hideThisWindowCheckBox->setDisabled(true);
        hideThisWindowCheckBox->setChecked(false);
    } else {
        hideThisWindowCheckBox->setDisabled(false);
    }
}
//! [6]


//! [10]
void Screenshot::updateScreenshotLabel()//从原始图片中提取数据，并根据当前窗口的大小进行缩放，然后显示在界面上：
{
    screenshotLabel->setPixmap(originalPixmap.scaled(screenshotLabel->size(),
                                                     Qt::KeepAspectRatio,
                                                     Qt::SmoothTransformation));
}
//! [10]
