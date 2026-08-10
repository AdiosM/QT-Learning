#include "widget.h"
#include "ui_widget.h"
#include "ColorFlashWidget.h" // 确保包含自定义控件头文件
#include <QTimer>
#include <QDebug>
#include<QPixmap>


Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    setWindowTitle("Go学长自检");
    ui->label_title->setStyleSheet("background-color: rgba(255,0,0,120);");//设置QLabel的背景颜色
    ui->output_value->setStyleSheet("background-color: rgba(255,0,0,120);");

    ui->widget->reset();//初始化左侧色块颜色


    // 2. 连接信号与槽
    connect(ui->pbt_strat, &QPushButton::clicked,
            this, &Widget::onStartClicked);

    connect(ui->widget, &ColorFlashWidget::panelClicked,
            this, &Widget::onPanelClicked);


}

Widget::~Widget()
{
    delete ui;
}

void Widget::onStartClicked()
{
    if (m_isTesting) return; // 如果正在测试中，忽略点击

    ui->output_value->setText("");

    m_isTesting = true;
    m_colorChanged=false;//重置为未变色状态
    ui->pbt_strat->setEnabled(false); // 禁用按钮，防止干扰
    //ui->reminder->setText("等待..."); //reminder不属于主窗口，不建议这样调用
    ui->widget->setFeedbackText("等待...");

    // 重置色块颜色（比如变回红色或灰色）
    ui->widget->reset();


    // 生成 1000ms 到 4000ms 之间的随机延迟
    int randomDelay = QRandomGenerator::global()->bounded(1000, 4000);

    // 使用单次定时器，延迟结束后执行 Lambda 表达式
    QTimer::singleShot(randomDelay, this, [this]() {
        if (!m_isTesting) return; // 安全检查

        // A. 变色！
        ui->widget->flash(Qt::green);

        // B. 启动高精度计时器
        m_timer.start();
        m_colorChanged=true;//变色后，标记为true
        //ui->reminder->setText("快点击！");
        ui->widget->setFeedbackText("快点击！");
    });
}

void Widget::onPanelClicked() //用户点击响应
{
    // 只有在测试进行中（且已经变色后）点击才有效
    // 注意：这里有个细节，如果用户在变色前点击怎么办？
    // 通常反应测试允许“抢跑”，但简单起见，我们只处理变色后的点击
    // 如果需要防抢跑，需要增加一个 m_isWaitingForColor 标志

    if (!m_isTesting) return;

    //抢跑判断逻辑
    if (!m_colorChanged) {
        // 1. 输出错误值或提示
        ui->output_value->setText("抢跑无效！");

        // 2. 在色块中间显示 "抢跑了！"
        ui->widget->setFeedbackText("抢跑了！");

        // 3. 结束本次测试，允许重新开始
        m_isTesting = false;
        ui->pbt_strat->setEnabled(true);
        return; // 直接返回，不计算时间
    }

    // 计算耗时（纳秒转毫秒）
    qint64 nsecs = m_timer.nsecsElapsed();
    double msecs = nsecs / 1000000.0;

    // 显示结果
    ui->output_value->setText(QString("%1 ms").arg(msecs, 0, 'f', 1));

    //给出测试结果
    QString feedback;
    if (msecs < 180) {
        feedback = " 🫡上等马！";
        ui->output_value->setText(QString("上等马! %1 ms").arg(msecs, 0, 'f', 1));

    } else if (msecs < 200 && msecs>=180) {
        feedback = "👍 中等马";
       ui->output_value->setText(QString("中等马! %1 ms").arg(msecs, 0, 'f', 1));

    } else if (msecs > 200) {
       feedback = "😴 下等马";
       ui->output_value->setText(QString("下等马! %1 ms").arg(msecs, 0, 'f', 1));


    }

    ui->widget->setFeedbackText(feedback);


    // 恢复状态
    m_isTesting = false;
    ui->pbt_strat->setEnabled(true);

    // 可选：点击后立即变回红色，或者保持绿色直到下次开始
    // ui->widget->reset();
}





