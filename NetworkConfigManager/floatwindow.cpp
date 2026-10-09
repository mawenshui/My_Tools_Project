#include "floatwindow.h"
#include <QPainter>
#include <QMouseEvent>
#include <QMenu>
#include <QApplication>
#include <QCursor>
#include <QDebug>
#include <QScreen>
#include <QTimer>

/**
 * @brief FloatWindow构造函数
 * @param parent 父窗口指针
 *
 * 初始化浮动窗口，设置无边框、置顶和透明背景属性
 */
FloatWindow::FloatWindow(QWidget *parent)
    : QWidget(parent),
      m_dragging(false),
      m_draggable(true),
      m_backgroundColor(QColor(61, 219, 255)),
      m_textColor(Qt::black),
      m_displayText("IP"),
      m_hovered(false),
      m_hasBackgroundPixmap(false),
      m_hiddenAtEdge(false),
      m_showProgress(false),
      m_progressPercentage(0),
      m_isLoading(false),
      m_loadingAngle(0)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    int baseSize = 64;
    qreal dpiScale = qApp->primaryScreen()->devicePixelRatio();
    int scaledSize = qRound(baseSize * dpiScale);
    setFixedSize(scaledSize, scaledSize);

    m_loadingTimer.reset(new QTimer(this));
    connect(m_loadingTimer.data(), &QTimer::timeout, this, [this]() {
        m_loadingAngle += 15;
        if (m_loadingAngle >= 360) {
            m_loadingAngle = 0;
        }
        update();
    });
    m_loadingTimer->setInterval(30);
}

FloatWindow::~FloatWindow()
{
    stopLoading();
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
        //图片模式下也显示hover效果
        if(m_hovered)
        {
            painter.fillRect(rect(), QColor(255, 255, 255, 30)); //白色半透明叠加
        }
        //图片模式下也绘制文本
        painter.setPen(m_textColor);
        QFont font = painter.font();
        font.setPointSize(12);    //设置字体大小
        font.setBold(true);      //设置粗体
        painter.setFont(font);
        painter.drawText(rect(), Qt::AlignCenter, m_displayText);
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

    //绘制进度条
    if (m_showProgress)
    {
        int barWidth = width() - 16;
        int barHeight = 6;
        int barX = 8;
        int barY = height() - 12;
        QRectF barRect(barX, barY, barWidth, barHeight);

        //绘制背景
        painter.setBrush(QColor(0, 0, 0, 100));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(barRect, 3, 3);

        //绘制进度
        double progressWidth = barWidth * (m_progressPercentage / 100.0);
        if (progressWidth > 0)
        {
            QRectF progressRect(barX, barY, progressWidth, barHeight);
            painter.setBrush(QColor(0, 200, 100));
            painter.drawRoundedRect(progressRect, 3, 3);
        }
    }

    //绘制加载动画
    if (m_isLoading)
    {
        painter.save();

        //绘制半透明遮罩
        painter.setBrush(QColor(0, 0, 0, 80));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(rect().adjusted(5, 5, -5, -5));

        //绘制旋转加载图标
        painter.translate(width() / 2, height() / 2);
        painter.rotate(m_loadingAngle);

        //绘制圆环加载动画
        int ringRadius = qMin(width(), height()) / 2 - 15;
        QRectF ringRect(-ringRadius, -ringRadius, ringRadius * 2, ringRadius * 2);

        //绘制外圈圆环
        QPen ringPen(QColor(61, 219, 255), 4);
        ringPen.setCapStyle(Qt::RoundCap);
        painter.setPen(ringPen);
        painter.setBrush(Qt::NoBrush);

        //绘制扇形进度条
        painter.drawArc(ringRect, 90 * 16, -m_loadingAngle * 16);

        //绘制中心圆点
        painter.setBrush(QColor(61, 219, 255));
        painter.drawEllipse(-4, -4, 8, 8);

        painter.restore();
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
 * 结束拖动状态(仅处理左键)，并实现屏幕边缘吸附
 */
void FloatWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton && m_dragging)
    {
        m_dragging = false;
        snapToScreenEdge();  //实现边缘吸附
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
    
    // 如果贴边隐藏了，就完全展开
    if (m_hiddenAtEdge)
    {
        m_hiddenAtEdge = false;
        move(m_fullGeometry.topLeft());
        setFixedSize(m_fullGeometry.size());
    }
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
    
    // 检查是否应该贴边隐藏
    checkAndHideAtEdge();
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

/**
 * @brief 屏幕边缘吸附
 *
 * 当窗口靠近屏幕边缘(20px内)时，自动吸附到最近的边缘
 */
void FloatWindow::snapToScreenEdge()
{
    const int snapMargin = 20; //吸附距离阈值
    QPoint pos = this->pos();
    bool snapped = false;

    //获取主屏幕可用区域
    QRect screenRect = QApplication::primaryScreen()->availableGeometry();
    int screenWidth = screenRect.width();
    int screenHeight = screenRect.height();
    int windowWidth = this->width();
    int windowHeight = this->height();

    //计算窗口中心到各边缘的距离
    int distToLeft = pos.x();
    int distToRight = screenWidth - (pos.x() + windowWidth);
    int distToTop = pos.y();
    int distToBottom = screenHeight - (pos.y() + windowHeight);

    //找到最近的边缘并吸附
    int minDist = qMin(qMin(distToLeft, distToRight), qMin(distToTop, distToBottom));

    if(minDist <= snapMargin)
    {
        if(distToLeft == minDist)
        {
            pos.setX(screenRect.left());
            snapped = true;
        }
        else if(distToRight == minDist)
        {
            pos.setX(screenRect.left() + screenWidth - windowWidth);
            snapped = true;
        }
        else if(distToTop == minDist)
        {
            pos.setY(screenRect.top());
            snapped = true;
        }
        else if(distToBottom == minDist)
        {
            pos.setY(screenRect.top() + screenHeight - windowHeight);
            snapped = true;
        }
    }

    if(snapped)
    {
        this->move(pos);
    }
}

/**
 * @brief 检查并贴边隐藏
 *
 * 当窗口贴边且鼠标离开时，只显示10px边缘
 */
void FloatWindow::checkAndHideAtEdge()
{
    // 如果正在拖动，不处理
    if (m_dragging)
        return;

    QRect screenRect = QApplication::primaryScreen()->availableGeometry();
    QPoint pos = this->pos();
    int windowWidth = this->width();
    int windowHeight = this->height();

    // 检查是否贴边
    bool atLeft = (pos.x() <= screenRect.left() + 5);
    bool atRight = (pos.x() + windowWidth >= screenRect.left() + screenRect.width() - 5);
    bool atTop = (pos.y() <= screenRect.top() + 5);
    bool atBottom = (pos.y() + windowHeight >= screenRect.top() + screenRect.height() - 5);

    if (atLeft || atRight || atTop || atBottom)
    {
        m_fullGeometry = geometry();
        m_hiddenAtEdge = true;

        if (atLeft)
        {
            m_hiddenEdge = Qt::LeftEdge;
            move(screenRect.left() - windowWidth + 10, pos.y());
        }
        else if (atRight)
        {
            m_hiddenEdge = Qt::RightEdge;
            move(screenRect.left() + screenRect.width() - 10, pos.y());
        }
        else if (atTop)
        {
            m_hiddenEdge = Qt::TopEdge;
            move(pos.x(), screenRect.top() - windowHeight + 10);
        }
        else if (atBottom)
        {
            m_hiddenEdge = Qt::BottomEdge;
            move(pos.x(), screenRect.top() + screenRect.height() - 10);
        }
    }
}

void FloatWindow::showProgress(int percentage, const QString &statusText)
{
    m_progressPercentage = percentage;
    m_progressText = statusText;
    m_showProgress = true;
    update();
}

void FloatWindow::hideProgress()
{
    m_showProgress = false;
    m_progressPercentage = 0;
    m_progressText.clear();
    update();
}

void FloatWindow::startLoading()
{
    m_isLoading = true;
    m_loadingAngle = 0;
    if (!m_loadingTimer.isNull()) {
        m_loadingTimer->start();
    }
    update();
}

void FloatWindow::stopLoading()
{
    m_isLoading = false;
    if (!m_loadingTimer.isNull()) {
        m_loadingTimer->stop();
    }
    update();
}
