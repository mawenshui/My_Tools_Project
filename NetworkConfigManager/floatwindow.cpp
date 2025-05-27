#include "floatwindow.h"
#include <QPainter>
#include <QMouseEvent>
#include <QMenu>
#include <QApplication>
#include <QCursor>
#include <QDebug>

/**
 * @brief FloatWindow构造函数
 * @param parent 父窗口指针
 *
 * 初始化浮动窗口，设置无边框、置顶和透明背景属性
 */
FloatWindow::FloatWindow(QWidget *parent)
    : QWidget(parent),
      m_dragging(false),        //初始状态：未拖动
      m_draggable(true),        //默认可拖动
      m_backgroundColor(QColor(61, 219, 255)),  //默认背景色(浅蓝色)
      m_textColor(Qt::black),   //默认文本颜色
      m_displayText("IP"),      //默认显示文本
      m_hovered(false),          //初始状态：未悬停
      m_hasBackgroundPixmap(false)  //初始无背景图标
{
    //设置窗口标志：无边框、置顶、工具窗口
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    //设置透明背景
    setAttribute(Qt::WA_TranslucentBackground);
    //固定窗口大小
    setFixedSize(64, 64);
}

void FloatWindow::setBackgroundPixmap(const QPixmap &pixmap)
{
    m_backgroundPixmap = pixmap.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_hasBackgroundPixmap = !pixmap.isNull();
    update();  //触发重绘
}

void FloatWindow::clearBackgroundPixmap()
{
    m_backgroundPixmap = QPixmap();
    m_hasBackgroundPixmap = false;
    update();  //触发重绘
}

bool FloatWindow::hasBackgroundPixmap() const
{
    return m_hasBackgroundPixmap;
}

QPixmap FloatWindow::getBackgroundPixmap()
{
    return m_backgroundPixmap;
}

/**
 * @brief 设置背景颜色
 * @param color 新的背景颜色
 *
 * 如果颜色有变化则更新窗口显示
 */
void FloatWindow::setBackgroundColor(const QColor &color)
{
    if(m_backgroundColor != color)
    {
        m_backgroundColor = color;
        update();  //触发重绘
    }
}

/**
 * @brief 设置显示文本
 * @param text 要显示的文本
 *
 * 如果文本有变化则更新窗口显示
 */
void FloatWindow::setText(const QString &text)
{
    if(m_displayText != text)
    {
        m_displayText = text;
        update();  //触发重绘
    }
}

/**
 * @brief 设置文本颜色
 * @param color 新的文本颜色
 *
 * 如果颜色有变化则更新窗口显示
 */
void FloatWindow::setTextColor(const QColor &color)
{
    if(m_textColor != color)
    {
        m_textColor = color;
        update();  //触发重绘
    }
}

/**
 * @brief 设置窗口是否可拖动
 * @param enabled true-可拖动, false-不可拖动
 */
void FloatWindow::setDraggable(bool enabled)
{
    m_draggable = enabled;
}

/**
 * @brief 检查窗口是否置顶
 * @return 是否置顶
 */
bool FloatWindow::isOnTop() const
{
    return windowFlags() & Qt::WindowStaysOnTopHint;
}

/**
 * @brief 检查窗口是否可拖动
 * @return 是否可拖动
 */
bool FloatWindow::isDraggable() const
{
    return m_draggable;
}

/**
 * @brief 获取当前背景颜色
 * @return 背景颜色
 */
QColor FloatWindow::backgroundColor() const
{
    return m_backgroundColor;
}

/**
 * @brief 获取当前显示文本
 * @return 显示文本
 */
QString FloatWindow::text() const
{
    return m_displayText;
}

/**
 * @brief 获取当前文本颜色
 * @return 文本颜色
 */
QColor FloatWindow::textColor() const
{
    return m_textColor;
}

/**
 * @brief 绘制事件处理
 * @param event 绘制事件
 *
 * 绘制圆形背景和居中文本，悬停时有高亮效果
 */
void FloatWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);  //启用抗锯齿
    //如果有背景图标，则绘制背景图标
    if(m_hasBackgroundPixmap)
    {
        //计算居中位置
        QRect pixmapRect;
        pixmapRect.setSize(m_backgroundPixmap.size());
        pixmapRect.moveCenter(rect().center());
        painter.drawPixmap(pixmapRect, m_backgroundPixmap);
    }
    else
    {
        //否则绘制默认背景(圆形)
        QColor bgColor = m_backgroundColor;
        if(m_hovered)
        {
            bgColor = bgColor.lighter(110); //悬停时颜色变亮10%
        }
        painter.setBrush(bgColor);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(rect().adjusted(5, 5, -5, -5));  //比窗口小10px的圆
        //这里保持原有文本绘制逻辑
        painter.setPen(m_textColor);
        QFont font = painter.font();
        font.setPointSize(12);    //设置字体大小
        font.setBold(true);      //设置粗体
        painter.setFont(font);
        painter.drawText(rect(), Qt::AlignCenter, m_displayText);
    }
}

/**
 * @brief 鼠标按下事件处理
 * @param event 鼠标事件
 *
 * 记录拖动起始位置(仅处理左键)
 */
void FloatWindow::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton && m_draggable)
    {
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        m_dragging = true;
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

/**
 * @brief 鼠标移动事件处理
 * @param event 鼠标事件
 *
 * 处理窗口拖动(仅处理左键拖动)
 */
void FloatWindow::mouseMoveEvent(QMouseEvent *event)
{
    if(m_dragging && (event->buttons() & Qt::LeftButton))
    {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

/**
 * @brief 鼠标释放事件处理
 * @param event 鼠标事件
 *
 * 结束拖动状态(仅处理左键)
 */
void FloatWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton && m_dragging)
    {
        m_dragging = false;
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

/**
 * @brief 鼠标双击事件处理
 * @param event 鼠标事件
 *
 * 触发双击信号(仅处理左键)
 */
void FloatWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        emit doubleClicked();
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

/**
 * @brief 上下文菜单事件处理
 * @param event 菜单事件
 *
 * 触发显示上下文菜单信号
 */
void FloatWindow::contextMenuEvent(QContextMenuEvent *event)
{
    emit showContextMenu(event->globalPos());  //传递全局坐标
    event->accept();
}

/**
 * @brief 鼠标进入事件处理
 * @param event 进入事件
 *
 * 更新悬停状态并改变鼠标指针形状
 */
void FloatWindow::enterEvent(QEvent *event)
{
    Q_UNUSED(event);
    updateHoverState(true);
    setCursor(Qt::PointingHandCursor);  //设置为手形指针
}

/**
 * @brief 鼠标离开事件处理
 * @param event 离开事件
 *
 * 更新悬停状态并恢复默认鼠标指针
 */
void FloatWindow::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    updateHoverState(false);
    unsetCursor();  //恢复默认指针
}

/**
 * @brief 更新悬停状态
 * @param hovered 是否悬停
 *
 * 更新悬停状态并触发重绘
 */
void FloatWindow::updateHoverState(bool hovered)
{
    if(m_hovered != hovered)
    {
        m_hovered = hovered;
        update();  //触发重绘以更新视觉效果
    }
}
