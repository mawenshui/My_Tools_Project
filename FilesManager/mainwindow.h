/**
 * @brief 主窗口类 - 负责用户界面交互和整体流程控制
 *
 * 实现思路：
 * 1. 作为程序主界面，整合所有功能模块
 * 2. 通过信号槽连接UI与后台操作
 * 3. 管理文件操作线程的生命周期
 * 4. 提供操作状态反馈和错误处理
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "fileoperation.h"
#include "conflictresolver.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 源目录选择按钮点击槽
    void onBrowseSource();
    // 目标目录选择按钮点击槽
    void onBrowseDestination();
    // 开始操作按钮点击槽
    void onStartOperation();
    // 暂停/恢复按钮切换槽
    void onPauseResume(bool checked);
    // 取消操作按钮点击槽
    void onCancelOperation();

    // 更新进度条槽
    void updateProgress(int percent);
    // 显示操作状态消息槽
    void showOperationStatus(const QString &message);
    // 显示错误消息槽
    void showError(const QString &error);
    // 操作完成处理槽
    void operationCompleted();

private:
    Ui::MainWindow* ui;                  // Qt Designer生成的UI对象
    FileOperation* m_fileOp;              // 文件操作核心对象
    QThread* m_operationThread;           // 文件操作线程
    ConflictResolver m_conflictResolver;  // 冲突解决器

    // 初始化信号槽连接
    void setupConnections();
    // 初始化UI状态
    void initUI();
    // 设置控件可用状态
    void enableControls(bool enable);
};

#endif // MAINWINDOW_H
