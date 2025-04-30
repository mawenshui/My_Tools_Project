#include "progressdialog.h"
#include "ui_progressdialog.h"

#include <QTime>
#include <QMessageBox>
#include <QCloseEvent>

ProgressDialog::ProgressDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProgressDialog)
{
    ui->setupUi(this);
    // 初始化对话框属性
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); // 隐藏帮助按钮
    setWindowTitle("操作进度");
    setFixedSize(400, 200); // 固定对话框大小
    // 初始化控件状态
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(0);
    ui->btnCancel->setText("取消");
}

ProgressDialog::~ProgressDialog()
{
    delete ui;
}

void ProgressDialog::setOperationName(const QString &name)
{
    // 设置操作名称（显示在标题栏）
    setWindowTitle(QString("%1 - 操作进度").arg(name));
    // 同时更新标签显示
    ui->lblOperation->setText(QString("当前操作: %1").arg(name));
}

void ProgressDialog::setProgress(int value)
{
    // 设置进度条值（0-100）
    ui->progressBar->setValue(value);
    // 自动关闭对话框（当进度完成时）
    if(value >= 100)
    {
        this->accept(); // 关闭对话框
    }
}

void ProgressDialog::setStatus(const QString &status)
{
    // 更新状态标签（支持富文本）
    ui->lblStatus->setText(QString("<b>状态:</b> %1").arg(status));
    // 自动添加到日志区域
    ui->textEdit->append(QString("[%1] %2")
                         .arg(QTime::currentTime().toString("hh:mm:ss"))
                         .arg(status));
}

void ProgressDialog::closeEvent(QCloseEvent *event)
{
    // 重写关闭事件，防止意外关闭
    if(ui->progressBar->value() < 100)
    {
        QMessageBox::warning(this, "警告", "操作正在进行中，请先取消操作！");
        event->ignore();
    }
    else
    {
        event->accept();
    }
}

void ProgressDialog::on_btnCancel_clicked()
{
    // 发送取消信号
    emit operationCancelled();
    // 更新界面状态
    ui->lblStatus->setText("<b>状态:</b> 正在取消操作...");
    ui->btnCancel->setEnabled(false);
    // 禁用关闭按钮
    setWindowFlag(Qt::WindowCloseButtonHint, false);
}
