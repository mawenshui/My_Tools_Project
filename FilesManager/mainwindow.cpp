// mainwindow.cpp
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QThread>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_fileOp(new FileOperation)
    , m_operationThread(new QThread(this))
{
    ui->setupUi(this);
    // 初始化UI
    initUI();
    // 设置线程
    m_fileOp->moveToThread(m_operationThread);
    m_operationThread->start();
    // 连接信号槽
    setupConnections();
}

MainWindow::~MainWindow()
{
    m_operationThread->quit();
    m_operationThread->wait();
    delete ui;
}

void MainWindow::initUI()
{
    // 设置窗口属性
    setWindowTitle("文件操作工具");
    resize(800, 600);
    // 初始化下拉框
    ui->cmbOperation->addItems({"复制", "移动"});
    ui->cmbConflict->addItems({"覆盖", "跳过", "重命名"});
    // 设置默认值
    ui->chkRecursive->setChecked(true);
    ui->leFilter->setPlaceholderText("例如: *.txt;*.docx (多个用分号分隔)");
    // 按钮图标
    ui->btnBrowseSource->setIcon(QIcon(":/icons/folder.png"));
    ui->btnBrowseDest->setIcon(QIcon(":/icons/folder.png"));
    ui->btnStart->setIcon(QIcon(":/icons/start.png"));
    ui->btnPause->setIcon(QIcon(":/icons/pause.png"));
    ui->btnCancel->setIcon(QIcon(":/icons/cancel.png"));
}

void MainWindow::setupConnections()
{
    // UI信号
    connect(ui->btnBrowseSource, &QPushButton::clicked, this, &MainWindow::onBrowseSource);
    connect(ui->btnBrowseDest, &QPushButton::clicked, this, &MainWindow::onBrowseDestination);
    connect(ui->btnStart, &QPushButton::clicked, this, &MainWindow::onStartOperation);
    connect(ui->btnPause, &QPushButton::toggled, this, &MainWindow::onPauseResume);
    connect(ui->btnCancel, &QPushButton::clicked, this, &MainWindow::onCancelOperation);
    // 文件操作信号
    connect(m_fileOp, &FileOperation::progressChanged, this, &MainWindow::updateProgress);
    connect(m_fileOp, &FileOperation::statusMessage, this, &MainWindow::showOperationStatus);
    connect(m_fileOp, &FileOperation::operationFinished, this, &MainWindow::operationCompleted);
    connect(m_fileOp, &FileOperation::errorOccurred, this, &MainWindow::showError);
}

void MainWindow::onBrowseSource()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择源目录", QDir::homePath());
    if(!dir.isEmpty())
    {
        ui->leSource->setText(QDir::toNativeSeparators(dir));
    }
}

void MainWindow::onBrowseDestination()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择目标目录", QDir::homePath());
    if(!dir.isEmpty())
    {
        ui->leDest->setText(QDir::toNativeSeparators(dir));
    }
}

void MainWindow::onStartOperation()
{
    // 获取输入参数
    QString srcDir = ui->leSource->text().trimmed();
    QString destDir = ui->leDest->text().trimmed();
    if(srcDir.isEmpty() || destDir.isEmpty())
    {
        QMessageBox::warning(this, "错误", "请先选择源目录和目标目录");
        return;
    }
    // 准备操作参数
    FileOperation::OperationType opType = ui->cmbOperation->currentIndex() == 0 ?
                                          FileOperation::Copy : FileOperation::Move;
    ConflictResolver::Resolution conflictRes = static_cast<ConflictResolver::Resolution>(
            ui->cmbConflict->currentIndex());
    QStringList filters = ui->leFilter->text().split(";", QString::SkipEmptyParts);
    // 禁用UI控件
    enableControls(false);
    // 清空日志
    ui->teLog->clear();
    // 添加操作日志
    ui->teLog->append(QString("[%1] 开始操作: %2")
                      .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                      .arg(opType == FileOperation::Copy ? "复制" : "移动"));
    // 开始操作
    QMetaObject::invokeMethod(m_fileOp, "startOperation",
                              Q_ARG(QString, srcDir),
                              Q_ARG(QString, destDir),
                              Q_ARG(FileOperation::OperationType, opType),
                              Q_ARG(bool, ui->chkRecursive->isChecked()),
                              Q_ARG(ConflictResolver::Resolution, conflictRes),
                              Q_ARG(QString, ui->leFilter->text()));
}

void MainWindow::onPauseResume(bool checked)
{
    if(checked)
    {
        QMetaObject::invokeMethod(m_fileOp, "pause");
        ui->teLog->append(QString("[%1] 操作已暂停")
                          .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
    }
    else
    {
        QMetaObject::invokeMethod(m_fileOp, "resume");
        ui->teLog->append(QString("[%1] 操作已恢复")
                          .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
    }
}

void MainWindow::onCancelOperation()
{
    QMetaObject::invokeMethod(m_fileOp, "cancel");
    ui->teLog->append(QString("[%1] 操作已取消")
                      .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
}

void MainWindow::updateProgress(int percent)
{
    ui->progressBar->setValue(percent);
}

void MainWindow::showOperationStatus(const QString &message)
{
    ui->teLog->append(QString("[%1] %2")
                      .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                      .arg(message));
}

void MainWindow::showError(const QString &error)
{
    QMessageBox::critical(this, "错误", error);
    enableControls(true);
}

void MainWindow::operationCompleted()
{
    ui->teLog->append(QString("[%1] 操作完成")
                      .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
    enableControls(true);
}

void MainWindow::enableControls(bool enable)
{
    ui->leSource->setEnabled(enable);
    ui->leDest->setEnabled(enable);
    ui->btnBrowseSource->setEnabled(enable);
    ui->btnBrowseDest->setEnabled(enable);
    ui->cmbOperation->setEnabled(enable);
    ui->cmbConflict->setEnabled(enable);
    ui->chkRecursive->setEnabled(enable);
    ui->leFilter->setEnabled(enable);
    ui->btnStart->setEnabled(enable);
    ui->btnPause->setEnabled(!enable);
    ui->btnCancel->setEnabled(!enable);
    if(enable)
    {
        ui->btnPause->setChecked(false);
    }
}
