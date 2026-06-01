#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "datahandler.h"
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_bSend(false),
    m_bContinue(false),
    m_fileSeekPos(0),
    m_isRemberPos(Qt::Checked),
    m_handler(new DataHandler),
    m_handlerThread(new QThread)
{
    ui->setupUi(this);
    ui->pbt_ContinueSend->setVisible(false);

    m_handler->moveToThread(m_handlerThread);
    connect(m_handlerThread, &QThread::finished, m_handler, &DataHandler::deleteLater);
    connect(this, &MainWindow::StartWork, m_handler, &DataHandler::StartWork);
    connect(m_handler, &DataHandler::process, this, &MainWindow::process);
    m_handlerThread->start();
}

MainWindow::~MainWindow()
{
    m_handlerThread->quit();
    m_handlerThread->wait(2000);
    delete ui;
}

void MainWindow::process(int value)
{
    ui->pBar_file->setValue(value);
}

void MainWindow::on_pbt_Choose_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "选择数据文件");
    if(fileName.isEmpty())
        return;
    m_fileName = fileName;
    ui->lineEdit_FileName->setText(fileName);
}

void MainWindow::on_pbt_Send_clicked()
{
    m_fileSeekPos = 0;
    ui->pbt_ContinueSend->setText("暂停发送");

    if(!m_bSend) {
        ui->pbt_Send->setText("停止发送");
        ui->pbt_ContinueSend->setVisible(true);
        emit StartWork(m_fileName, m_fileSeekPos);
    }
    else {
        ui->pbt_Send->setText("开始发送");
        ui->pbt_ContinueSend->setVisible(false);
        m_handler->StopWork();
    }
    m_bSend = !m_bSend;
}

void MainWindow::on_pbt_ContinueSend_clicked()
{
    if(!m_bContinue) {
        m_handler->StopWork();
        ui->pbt_ContinueSend->setText("继续发送");
    }
    else {
        emit StartWork(m_fileName, m_fileSeekPos);
        ui->pbt_ContinueSend->setText("暂停发送");
    }
    m_bContinue = !m_bContinue;
}

void MainWindow::on_checkBox_stateChanged(int arg1)
{
    m_isRemberPos = Qt::CheckState(arg1);
}
