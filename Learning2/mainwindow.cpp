#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    this->setWindowTitle("Qt Widgets"); //给窗口起个名字
    resize(640,480);//设定窗口初始大小
    centralWidget=new QWidget(this);//创建一个中央容器（地基）
    this->setCentralWidget(centralWidget);//把这个容器交给主窗口管理

    /*这两行代码决定软件界面整体是从左到右排列三列内容*/
    hbox=new QHBoxLayout; // 创建一个水平排队管理器
    centralWidget->setLayout(hbox); // 把它设为中央容器的布局
    /* 只要把控件加入到布局中，这个控件就会自动成为该控件所属Widget的子对象 */

    buttonGroup =new QGroupBox("Button GroUp");// 带标题的盒子
    pushButton =new QPushButton("PushButton");// 按钮
    radioButton=new QRadioButton("RadioButton");// 单选框
    checkBox=new QCheckBox("CheckBox"); // 复选框

    /* 垂直排队(vboxButtonGroup)  */
    vboxButtonGroup=new  QVBoxLayout;
    vboxButtonGroup->addWidget(pushButton); // 按钮排第一
    vboxButtonGroup->addWidget(radioButton); // 单选框排第二
    vboxButtonGroup->addWidget(checkBox);   // 复选框排第三
/*hbox 是绑定在 centralWidget（中央容器）上的布局管理器。当你调用 addWidget 时，Qt 才会把这些盒子真正地“画”到中央容器里。*/

    buttonGroup->setLayout(vboxButtonGroup);/*把排好队的零件装进盒子里 */
    hbox->addWidget(buttonGroup);//把整个盒子放进水平大框架的最左边，在界面里显示按钮

    textInputGroup=new QGroupBox("Text Input");//把布局塞进 QGroupBox。
    lineEdit=new QLineEdit;
    textEdit=new QTextEdit;

    vboxtextInputGroup=new QVBoxLayout; //用 QVBoxLayout让QLineEdit,QComboBox上下排列。
    vboxtextInputGroup->addWidget(lineEdit);
    vboxtextInputGroup->addWidget(textEdit);

    textInputGroup->setLayout(vboxtextInputGroup);
    hbox->addWidget(textInputGroup);

    otherInputGroup=new QGroupBox("Other Input");
    comboBox=new QComboBox;
    comboBox->addItems({"Item 1","Item 2","Item 3"});//这是 C++11 的初始化列表语法，非常简洁地给下拉框添加了三个选项。
    dial=new QDial;
    slider=new QSlider;

    vboxOtherInputGroup=new QVBoxLayout; //布局管理器，不是QOBject，只是一个管理员，没有父子关系
    vboxOtherInputGroup->addWidget(comboBox);
    vboxOtherInputGroup->addWidget(dial);
    vboxOtherInputGroup->addWidget(slider);

    otherInputGroup->setLayout(vboxOtherInputGroup);
    hbox->addWidget(otherInputGroup);
    //hbox->addWidget(otherInputGroup,2);可以让Other Input窗口大一点

/* 对象树的构成（父子关系）
1.  直接在构造函数中建立关系 new QWidget(parent)
    比如：QPushButton *btn = new QPushButton(this);//this（主窗口）是btn的爸爸

2.使用布局管理器（Layout）确定关系
    比如：将控件A放进布局B:layout->addWidget(A);然后把布局B交给容器C：C->setLayout(B);
    结果就是A自动成为C的儿子。
    例如：
        QHBoxLayout *hbox = new QHBoxLayout;
        hbox->addWidget(pushButton); //pushButton暂时没有爸爸
        centralWidget->setLayout(hbox); //此时，pushButton自动成为了centralWidget的儿子

3.显示设置父对象（较少用）
    如果你先创建了对象，后来才想给它找个爸爸，可以使用 setParent()。
    例如：
        QLabel *label = new QLabel();//刚创建，暂时没有爸爸
        label->setParent(this); //现在它认主窗口做爸爸
 */

}

MainWindow::~MainWindow() {}

/*
 * 对象树，族谱。 QHBoxLayout 和 QVBoxLayout这两类没有Qt对象树意义上的父子关系，它们是布局管理者，负责界面的逻辑关系
MainWindow (根节点 / this)
│
└── centralWidget (QWidget)  <--- [关键节点：所有界面内容的容器]
     │
     ├── QHBoxLayout (hbox)  <--- [布局中介：负责水平排列]
     │    │
     │    ├── QGroupBox "Button Group" (buttonGroup)
     │    │    │
     │    │    └── QVBoxLayout (vboxButtonGroup) <--- [布局中介：负责垂直排列]
     │    │         │
     │    │         ├── QPushButton "PushButton" (pushButton)
     │    │         ├── QRadioButton "RadioButton" (radioButton)
     │    │         └── QCheckBox "CheckBox" (checkBox)
     │    │
     │    ├── QGroupBox "Text Input" (textInputGroup)
     │    │    │
     │    │    └── QVBoxLayout (vboxtextInputGroup) <--- [布局中介：负责垂直排列]
     │    │         │
     │    │         ├── QLineEdit (lineEdit)
     │    │         └── QTextEdit (textEdit)
     │    │
     │    └── QGroupBox "Other Input" (otherInputGroup)
     │         │
     │         └── QVBoxLayout (vboxOtherInputGroup) <--- [布局中介：负责垂直排列]
     │              │
     │              ├── QComboBox (comboBox)
     │              ├── QDial (dial)
     │              └── QSlider (slider)
     │
     └── [注意]：QMainWindow 默认还包含菜单栏、工具栏等，但你没用到，所以这里省略。
*/
