/**
 * @brief 进度对话框类 - 显示操作进度和状态
 *
 * 实现思路：
 * 1. 模态对话框设计，阻止用户其他操作
 * 2. 实时显示操作名称和进度
 * 3. 支持详细状态信息显示
 * 4. 集成取消按钮功能
 */
#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QDialog>

namespace Ui
{
    class ProgressDialog;
}

class ProgressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProgressDialog(QWidget *parent = nullptr);
    ~ProgressDialog();

    // 设置操作名称
    void setOperationName(const QString &name);
    // 设置进度值(0-100)
    void setProgress(int value);
    // 设置状态文本
    void setStatus(const QString &status);

signals:
    void operationCancelled(); // 用户取消操作时发射

private slots:
    void on_btnCancel_clicked();

private:
    Ui::ProgressDialog* ui; // UI对象指针
    void closeEvent(QCloseEvent *event);
};

#endif // PROGRESSDIALOG_H
