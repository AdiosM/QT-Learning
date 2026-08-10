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
    update();  // 标记重绘，下一帧统一绘制（非阻塞）
}

void ColorFlashWidget::reset()
{
    m_currentColor = Qt::red;

    update();

}

void ColorFlashWidget::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    // 单色填充，极快，无样式表解析开销
    painter.fillRect(rect(), m_currentColor);
}

void ColorFlashWidget::mousePressEvent(QMouseEvent * /*event*/)
{
    // 立即发出信号，无按钮状态机延迟
    emit panelClicked();
}

void ColorFlashWidget::setFeedbackText(const QString &text)
{
    QLabel *label = findChild<QLabel*>("reminder");//通过 findChild 找到 UI 中拖入的 reminder
    if (!label) return; // 安全检查：如果没找到就什么都不做

    if (text.isEmpty()) {
        label->hide();
    } else {
        label->setText(text);
        label->show();
    }
}