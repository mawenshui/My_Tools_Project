#ifndef FLOATWINDOW_H
#define FLOATWINDOW_H

#include <QWidget>
#include <QPoint>
#include <QColor>
#include <QScopedPointer>

class QTimer;

/**
 * @brief 浮动窗口类，实现可拖动、可自定义的悬浮窗口
 *
 * 该类继承自QWidget，提供可拖动、可自定义外观的悬浮窗口功能，
 * 支持鼠标交互和上下文菜单，常用于显示实时信息或快捷操作入口。
 */
class FloatWindow : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     */
    explicit FloatWindow(QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~FloatWindow();

    //外观设置方法组 -----------------------------------------------

    /**
     * @brief 设置背景图标
     * @param pixmap 背景图标
     *
     * 设置后背景图标将覆盖自绘制的背景
     */
    void setBackgroundPixmap(const QPixmap &pixmap);

    /**
     * @brief 获取背景图标
     * @return pixmap 背景图标
     */
    QPixmap getBackgroundPixmap();
public:
    /**
     * @brief 清除背景图标
     *
     * 清除后恢复自绘制的背景
     */
    void clearBackgroundPixmap();

    void showProgress(int percentage, const QString &statusText);
    void hideProgress();
    void startLoading();
    void stopLoading();

    /**
     * @brief 检查是否有背景图标
     * @return 是否有背景图标
     */
    bool hasBackgroundPixmap() const;

    /**
     * @brief 设置窗口背景颜色
     * @param color 背景颜色
     */
    void setBackgroundColor(const QColor &color);

    /**
     * @brief 设置显示文本
     * @param text 要显示的文本内容
     */
    void setText(const QString &text);

    /**
     * @brief 设置文本颜色
     * @param color 文本颜色
     */
    void setTextColor(const QColor &color);

    /**
     * @brief 设置窗口是否可拖动
     * @param enabled true-可拖动, false-不可拖动
     */
    void setDraggable(bool enabled);

    //状态获取方法组 -----------------------------------------------

    /**
     * @brief 获取窗口是否置顶
     * @return 是否置顶
     */
    bool isOnTop() const;

    /**
     * @brief 获取窗口是否可拖动
     * @return 是否可拖动
     */
    bool isDraggable() const;

    /**
     * @brief 获取当前背景颜色
     * @return 背景颜色
     */
    QColor backgroundColor() const;

    /**
     * @brief 获取当前显示文本
     * @return 显示文本内容
     */
    QString text() const;

    /**
     * @brief 获取当前文本颜色
     * @return 文本颜色
     */
    QColor textColor() const;

signals:
    /**
     * @brief 双击窗口时发出的信号
     */
    void doubleClicked();

    /**
     * @brief 请求显示上下文菜单的信号
     * @param pos 菜单显示位置(屏幕坐标)
     */
    void showContextMenu(const QPoint &pos);

protected:
    //重写的Qt事件处理函数 -----------------------------------------

    /**
     * @brief 绘制事件处理
     * @param event 绘制事件
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief 鼠标按下事件处理
     * @param event 鼠标事件
     */
    void mousePressEvent(QMouseEvent *event) override;

    /**
     * @brief 鼠标移动事件处理
     * @param event 鼠标事件
     */
    void mouseMoveEvent(QMouseEvent *event) override;

    /**
     * @brief 鼠标释放事件处理
     * @param event 鼠标事件
     */
    void mouseReleaseEvent(QMouseEvent *event) override;

    /**
     * @brief 鼠标双击事件处理
     * @param event 鼠标事件
     */
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    /**
     * @brief 上下文菜单事件处理
     * @param event 菜单事件
     */
    void contextMenuEvent(QContextMenuEvent *event) override;

    /**
     * @brief 鼠标进入窗口事件处理
     * @param event 进入事件
     */
    void enterEvent(QEvent *event) override;

    /**
     * @brief 鼠标离开窗口事件处理
     * @param event 离开事件
     */
    void leaveEvent(QEvent *event) override;

private:
    //成员变量 -----------------------------------------------------
    QPoint m_dragPosition;      //拖动起始位置
    bool m_dragging;            //是否正在拖动
    bool m_draggable;           //窗口是否可拖动
    QColor m_backgroundColor;   //背景颜色
    QColor m_textColor;         //文本颜色
    QString m_displayText;      //显示文本内容
    bool m_hovered;             //鼠标是否悬停在窗口上
    QPixmap m_backgroundPixmap;  //背景图标
    bool m_hasBackgroundPixmap;   //是否有背景图标
    bool m_hiddenAtEdge;          //是否贴边隐藏
    QRect m_fullGeometry;         //隐藏前的完整位置
    Qt::Edge m_hiddenEdge;         //隐藏的边缘方向
    bool m_showProgress;                    //是否显示进度条
    int m_progressPercentage;                //进度百分比
    QString m_progressText;                  //进度状态文本
    bool m_isLoading;                       //是否正在加载
    int m_loadingAngle;                     //加载动画旋转角度
    QScopedPointer<QTimer> m_loadingTimer;  //加载动画定时器（智能指针管理）

    /**
     * @brief 更新悬停状态
     * @param hovered 是否悬停
     *
     * 用于处理鼠标进入/离开时的视觉效果变化
     */
    void updateHoverState(bool hovered);

    /**
     * @brief 屏幕边缘吸附
     *
     * 当窗口靠近屏幕边缘(20px内)时，自动吸附到最近的边缘
     */
    void snapToScreenEdge();
    
    /**
     * @brief 检查并贴边隐藏
     *
     * 当窗口贴边且鼠标离开时，只显示10px边缘
     */
    void checkAndHideAtEdge();
};

#endif //FLOATWINDOW_H
