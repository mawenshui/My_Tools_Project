#include "ordersendwidget.h"
#include "ui_ordersendwidget.h"

//定义日志分类
Q_LOGGING_CATEGORY(OrderSendLog, "[app.OrderSend]")

/**
 * @brief 主窗口构造函数
 * @param parent 父窗口指针
 */
OrderSendWidget::OrderSendWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OrderSendWidget),
    m_currentLoopIndex(0),
    m_loopRunning(false)
{
    ui->setupUi(this);
    //初始化配置和界面
    getconfig();        //从配置文件加载任务配置
    comboset();         //初始化下拉框选项
    //设置默认网络参数
    ui->lineEditIP->setText("239.255.1.21");
    ui->lineEditPort->setText("9221");
    //初始化成员变量
    runed = false;      //循环发送标志初始化为false
    stopRequested = false; //停止请求标志初始化为false
    //连接信号槽：日志添加信号与槽函数连接
    connect(this, &OrderSendWidget::logMessage, this, &OrderSendWidget::handleLogMessage);
    //初始化发送线程
    m_sendThread = new QThread(this);
    m_sendWorker = new SendWorker();
    m_sendWorker->moveToThread(m_sendThread);
    connect(m_sendWorker, &SendWorker::logMessage, this, &OrderSendWidget::handleLogMessage);
    connect(m_sendWorker, &SendWorker::finished, this, &OrderSendWidget::handleSendFinished);
    connect(this, &OrderSendWidget::startSendCommand, m_sendWorker, &SendWorker::sendCommand);
    //连接搜索框的信号
    connect(ui->lineEdit_searchOrder, &QLineEdit::textChanged, this, &OrderSendWidget::filterComboBoxItems);
    m_sendThread->start();
}

/**
 * @brief 根据搜索框内容过滤comboBox中的项
 * @param filterText 过滤文本
 */
void OrderSendWidget::filterComboBoxItems(const QString &filterText)
{
    //保存当前选中的项
    QString currentText = ui->comboBox->currentText();
    //清空comboBox但不触发信号
    ui->comboBox->blockSignals(true);
    ui->comboBox->clear();
    //如果搜索框为空，显示所有项
    if(filterText.isEmpty())
    {
        QMap<QString, int>::iterator itr = m_commandId.begin();
        while(itr != m_commandId.end())
        {
            ui->comboBox->addItem(QString("【0x%1】%2").arg(QString::number(itr.value(), 16).toUpper()).arg(itr.key()));
            itr++;
        }
    }
    else
    {
        //否则只显示匹配的项
        QMap<QString, int>::iterator itr = m_commandId.begin();
        while(itr != m_commandId.end())
        {
            QString itemText = QString("【0x%1】%2").arg(QString::number(itr.value(), 16).toUpper()).arg(itr.key());
            if(itemText.contains(filterText, Qt::CaseInsensitive))
            {
                ui->comboBox->addItem(itemText);
            }
            itr++;
        }
    }
    ui->comboBox->blockSignals(false);
    //尝试恢复之前选中的项
    int index = ui->comboBox->findText(currentText);
    if(index >= 0)
    {
        ui->comboBox->setCurrentIndex(index);
    }
    else if(ui->comboBox->count() > 0)
    {
        ui->comboBox->setCurrentIndex(0);
    }
}

void OrderSendWidget::handleSendFinished()
{
    ui->pushButtonstart->setEnabled(true);
    emit logMessage("DEBUG", "指令发送完成");
    emit logMessage("DEBUG", "**********************************************************");
    if(m_loopRunning)
    {
        //使用QTimer延迟2秒后发送下一个命令
        QTimer::singleShot(2000, this, &OrderSendWidget::sendNextLoopCommand);
    }
}

void OrderSendWidget::handleLogMessage(QString level, QString message)
{
    LogEntry entry{level.toUpper(), message, QDateTime::currentDateTime()};
    QString html = QString("<div style='color:%1'>[%2] [%3] %4</div>")
                   .arg(colorForLevel(level).name(),
                        entry.time.toString(dt_format),
                        level,
                        message.toHtmlEscaped());
    appendLogToView(html);
}

/**
 * @brief 根据日志级别获取颜色
 * @param level 日志级别
 * @return 对应的颜色
 */
QColor OrderSendWidget::colorForLevel(const QString &level) const
{
    //根据日志级别返回颜色
    if(level == "INFO")
    {
        return m_logColors.info;
    }
    if(level == "WARN")
    {
        return m_logColors.warn;
    }
    if(level == "ERROR")
    {
        return m_logColors.error;
    }
    if(level == "DEBUG")
    {
        return m_logColors.debug;
    }
    return m_logColors.defualt;
}

void OrderSendWidget::appendLogToView(const QString &html)
{
    //添加日志到视图
    QTextCursor cursor(ui->textEdit->document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertHtml(html);
    cursor.insertBlock();
    ui->textEdit->ensureCursorVisible();
}

/**
 * @brief 从XML配置文件读取任务配置
 */
void OrderSendWidget::getconfig()
{
    //获取应用程序所在目录
    QString appDir = QCoreApplication::applicationDirPath();
    //处理可能的符号链接（如果是macOS的.app包）
    QFileInfo appInfo(appDir);
    if(appInfo.isSymLink())
    {
        appDir = appInfo.symLinkTarget();
    }
    //获取上一级目录
    QDir parentDir(appDir);
    if(!parentDir.cdUp())
    {
        qCWarning(OrderSendLog) << "无法访问应用程序目录的上一级目录:" << appDir;
        return;
    }
    //构建配置文件路径（跨平台兼容）
    QString configPath = QDir::cleanPath(parentDir.absolutePath() + QDir::separator() + "config" + QDir::separator() + "sibugz_config.xml");
    TiXmlDocument* treeDoc = new TiXmlDocument();
    //加载XML文件
    if(!treeDoc->LoadFile(configPath.toStdString().c_str()))
    {
        qCritical() << "MainWindow::getconfig()::配置文件加载失败：" << configPath;
        return;
    }
    //获取根节点
    TiXmlElement* root = treeDoc->RootElement();
    //遍历一级子节点（task节点）
    for(TiXmlElement *elem = root->FirstChildElement(); elem != NULL; elem = elem->NextSiblingElement())
    {
        const char* elemValue = elem->Value();
        //处理task节点
        if(strcmp(elemValue, "task") == 0)
        {
            QString name = elem->Attribute("name"); //获取任务名称
            //初始化任务参数
            double time = -1;
            int id = -1;
            int comid = -1;
            int index = -1;
            //遍历task的子节点
            for(TiXmlElement *elem1 = elem->FirstChildElement(); elem1 != NULL; elem1 = elem1->NextSiblingElement())
            {
                elemValue = elem1->Value();
                const char* attr_value = elem1->GetText();
                //处理不同配置项
                if(strcmp(elemValue, "outTime") == 0)
                {
                    //获取超时时间
                    time = QString(attr_value).toDouble();
                }
                else if(strcmp(elemValue, "id") == 0)
                {
                    //处理ID（支持十六进制和十进制格式）
                    QString temps;
                    if(attr_value[0] == 'x')
                    {
                        temps = attr_value + 1;
                        id = temps.toLongLong(Q_NULLPTR, 16);
                    }
                    else if(attr_value[1] == 'x')
                    {
                        temps = attr_value + 2;
                        id = temps.toLongLong(Q_NULLPTR, 16);
                    }
                    else
                    {
                        temps = attr_value;
                        id = temps.toLongLong();
                    }
                }
                else if(strcmp(elemValue, "comid") == 0)
                {
                    //处理命令ID（支持十六进制和十进制格式）
                    QString temps;
                    if(attr_value[0] == 'x')
                    {
                        temps = attr_value + 1;
                        comid = temps.toLongLong(Q_NULLPTR, 16);
                    }
                    else if(attr_value[1] == 'x')
                    {
                        temps = attr_value + 2;
                        comid = temps.toLongLong(Q_NULLPTR, 16);
                    }
                    else
                    {
                        temps = attr_value;
                        comid = temps.toLongLong();
                    }
                }
                else if(strcmp(elemValue, "startIndex") == 0)
                {
                    //获取起始索引
                    QString temps = attr_value;
                    index = temps.toLongLong();
                }
            }
            //将配置存入Map
            m_time.insert(QString("【0x%1】%2").arg(QString::number(id, 16).toUpper()).arg(name), time);
            m_commandId.insert(QString("【0x%1】%2").arg(QString::number(id, 16).toUpper()).arg(name), id);
            m_workId.insert(QString("【0x%1】%2").arg(QString::number(id, 16).toUpper()).arg(name), comid);
        }
    }
    delete treeDoc; //释放XML文档内存
}

/**
 * @brief 初始化下拉框选项
 */
void OrderSendWidget::comboset()
{
    //清空comboBox但不触发信号
    ui->comboBox->blockSignals(true);
    ui->comboBox->clear();
    //遍历命令ID映射表，将任务名称添加到下拉框
    QMap<QString, int>::iterator itr = m_commandId.begin();
    while(itr != m_commandId.end())
    {
        ui->comboBox->addItem(itr.key());
        itr++;
    }
    ui->comboBox->blockSignals(false);
}

/**
 * @brief 主窗口析构函数
 */
OrderSendWidget::~OrderSendWidget()
{
    m_loopRunning = false; //确保循环停止
    m_sendThread->quit();
    m_sendThread->wait();
    delete m_sendWorker;
    delete ui;
}

/**
 * @brief 开始发送按钮点击处理函数
 */
void OrderSendWidget::on_pushButtonstart_clicked()
{
    QString address = ui->lineEditIP->text();
    quint16 port = static_cast<quint16>(ui->lineEditPort->text().toUInt());
    //验证IP地址格式
    QHostAddress addressCheck;
    if(!addressCheck.setAddress(address))
    {
        QMessageBox::warning(this, "错误", "无效的IP地址格式");
        return;
    }
    //验证端口范围
    if(port < 1 || port > 65535)
    {
        QMessageBox::warning(this, "错误", "端口号必须为1-65535");
        return;
    }
    if(ui->comboBox->count() == 0)
    {
        QMessageBox::warning(this, "错误", "未加载到指令信息");
        return;
    }
    //获取当前选择的命令名称
    QString comname = ui->comboBox->currentText();
    if(!m_commandId.contains(comname))
    {
        return;
    }
    //禁用按钮防止重复点击
    ui->pushButtonstart->setEnabled(false);
    emit logMessage("DEBUG", "**********************************************************");
    emit logMessage("DEBUG", QString("执行单次发送指令：[%1]").arg(comname));
    //通过信号触发子线程发送
    emit startSendCommand(address, port, comname,
                          m_commandId, m_workId, m_time,
                          ui->noCRC->isChecked(), ui->reNOCRC->isChecked(),
                          ui->noreplay->isChecked(), ui->noreturn->isChecked(),
                          ui->noReturnReply->isChecked(), ui->replyNOCRC->isChecked(),
                          ui->ReERR->isChecked(), ui->noShake->isChecked(),
                          ui->isReplyTimeout->isChecked(), ui->isReturnTimeout->isChecked(),
                          ui->isReReplyTimeout->isChecked());
}

/**
 * @brief 循环发送按钮点击处理
 */
void OrderSendWidget::on_pushButton_clicked()
{
    emit logMessage("DEBUG", "循环发送开始，在点击停止按钮前会重复遍历指令列表并发送数据");
    ui->pushButtonStop->setEnabled(true);
    ui->pushButton->setEnabled(false);
    ui->pushButtonstart->setEnabled(false);
    m_loopRunning = true;
    m_currentLoopIndex = 0;
    sendNextLoopCommand(); //开始发送第一个命令
}

void OrderSendWidget::sendNextLoopCommand()
{
    if(!m_loopRunning)
    {
        //循环结束
        m_loopRunning = false;
        ui->pushButtonStop->setEnabled(false);
        ui->pushButton->setEnabled(true);
        ui->pushButtonstart->setEnabled(true);
        m_currentLoopIndex = 0;
        emit logMessage("DEBUG", "********************************************************");
        emit logMessage("DEBUG", "******************循环发送完成，发送已停止******************");
        emit logMessage("DEBUG", "********************************************************");
        return;
    }
    if(m_loopRunning && m_currentLoopIndex >= ui->comboBox->count())
    {
        m_currentLoopIndex = 0;
        emit logMessage("DEBUG", "***************************************************************************");
        emit logMessage("DEBUG", "******************发送一个循环完成，等待一分钟后自动进行下一个循环******************");
        emit logMessage("DEBUG", "***************************************************************************");
        QThread::msleep(60000);
    }
    //设置当前命令并触发发送
    ui->comboBox->setCurrentIndex(m_currentLoopIndex);
    on_pushButtonstart_clicked();
    m_currentLoopIndex++;
}

/**
 * @brief 清空日志按钮点击处理
 */
void OrderSendWidget::on_pushButtonClearLog_clicked()
{
    ui->textEdit->clear(); //清空文本编辑框
}

/**
 * @brief 停止循环发送按钮点击处理
 */
void OrderSendWidget::on_pushButtonStop_clicked()
{
    m_loopRunning = false; //停止循环
    ui->pushButtonStop->setEnabled(false);
    ui->pushButton->setEnabled(true);
    ui->pushButtonstart->setEnabled(true);
    emit logMessage("DEBUG", "点击停止按钮，循环发送已停止");
}

