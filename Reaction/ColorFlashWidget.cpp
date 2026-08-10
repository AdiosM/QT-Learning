#include "ColorFlashWidget.h"
#include <QPainter>
#include<QLabel>


ColorFlashWidget::ColorFlashWidget(QWidget *parent)
    : QWidget(parent)
    , m_currentColor(Qt::red)  // 初始红色
{
    // 性能优化：跳过不必要的背景擦除和系统背景绘制
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);

    // 设置最小尺寸，防止在布局中被压缩到看不见
    setMinimumSize(500, 400);

    // 鼠标悬停时显示手型光标，提示可点击
    setCursor(Qt::PointingHandCursor);

}

void ColorFlashWidget::flash(const QColor &color)
{
    m_currentColor = color;
    update();  // 标记重绘，下一帧统一绘制（非阻塞），在<QWidget>z中包含
}

void ColorFlashWidget::reset()
{
    m_currentColor = Qt::red;//初始化色块为红色
    update();//作用是异步请求重绘，让Qt在合适的时机自动调用重写过的paintEvent()

}

void ColorFlashWidget::paintEvent(QPaintEvent * /*event*/)
{ //由 Qt 事件循环（Event Loop） 在特定条件下自动触发的“回调函数”。在事件循环中被接收，由update发送请求
    QPainter painter(this);
    // 单色填充，极快，无样式表解析开销
    painter.fillRect(rect(), m_currentColor);
}

void ColorFlashWidget::mousePressEvent(QMouseEvent * /*event*/)
{//由系统获取鼠标点击信号，qt事件循环QApplication::exec()会收到事件，然后调用这个函数
    // 立即发出信号，无按钮状态机延迟
    emit panelClicked();
    //当操作系统检测到鼠标硬件中断并传递到 Qt 应用程序时，Qt 会将其封装为 QMouseEvent 对象并派发给对应控件。
    //mousePressEvent 只在按下瞬间触发一次。如果你需要检测“按住不放”或“释放”，需要配合 mouseReleaseEvent 或使用定时器。


}

void ColorFlashWidget::setFeedbackText(const QString &text)//显示色块上面的文字
{
    QLabel *label = findChild<QLabel*>("reminder");//通过 findChild 找到 UI 中拖入的reminder控件
    //这个函数可以对任意的控件生效，只需要修改上面这行代码就可以
    if (!label) return; // 安全检查：如果没找到就什么都不做

    if (text.isEmpty()) {
        label->hide();
    } else {
        label->setText(text);
        label->show();
    }
}


/*** 程序启动
    ↓
    窗口显示 → Qt 自动调用 paintEvent() → 涂成红色
    ↓
    点击"开始"
    ↓
    reset() → update() → paintEvent() → 涂成红色
    ↓
    等待 2.5 秒
    ↓
    flash(green) → update() → paintEvent() → 涂成绿色
    ↓
    用户点击
    ↓
（没有直接调用 update，所以保持绿色）
***/



