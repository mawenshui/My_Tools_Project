#include "ordersendwidget.h"
#include "ui_ordersendwidget.h"
#include "threadmanager.h"

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
    m_loopRunning(false),
    m_flowConfig(new ConfigManager(this)),
    m_mulFast(FLOW_FAST_MULTIPLIER),
    m_mulNormal(FLOW_NORMAL_MULTIPLIER),
    m_mulChecked(TIMEOUT_CHECKED_MULTIPLIER),
    m_isSingleLoopMode(false)
{
    qCInfo(OrderSendLog) << "OrderSendWidget 构造开始";
    ui->setupUi(this);
    
    // 连接"单个循环发送"按钮信号
    connect(ui->btnLoopSingle, &QPushButton::clicked, this, &OrderSendWidget::on_btnLoopSingle_clicked);

    qCInfo(OrderSendLog) << "OrderSendWidget UI setup 完成";
    //初始化配置和界面
    getconfig();        //从配置文件加载任务配置
    comboset();         //初始化下拉框选项
    // 读取倍率配置并应用到UI
    loadMultipliersFromConfig();
    applyMultiplierTextToUI();
    // 监听配置变化与重载，实时更新倍率与UI提示
    connect(m_flowConfig, &ConfigManager::configChanged, this, [this](const QString &key, const QVariant &){
        if(key=="flowFastMultiplier" || key=="flowNormalMultiplier" || key=="timeoutCheckedMultiplier")
        {
            loadMultipliersFromConfig();
            applyMultiplierTextToUI();
        }
    });
    connect(m_flowConfig, &ConfigManager::configReloaded, this, [this](){
        loadMultipliersFromConfig();
        applyMultiplierTextToUI();
    });

    // 设置按钮事件：打开倍率设置与帮助
    connect(ui->btnMultiplierSettings, &QPushButton::clicked, this, &OrderSendWidget::onOpenMultiplierSettings);
    connect(ui->btnMultiplierHelp, &QPushButton::clicked, this, &OrderSendWidget::onOpenMultiplierHelp);
    // 初始化超时编辑框显示为当前选择的默认值
    updateTimeoutUIForSelection();
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
    // 注入共享配置管理器，确保发送线程读取到最新倍率
    m_sendWorker->setConfigManager(m_flowConfig);
    m_sendWorker->moveToThread(m_sendThread);
    connect(m_sendWorker, &SendWorker::logMessage, this, &OrderSendWidget::handleLogMessage);
    connect(m_sendWorker, &SendWorker::finished, this, &OrderSendWidget::handleSendFinished);
    connect(this, &OrderSendWidget::startSendCommand, m_sendWorker, &SendWorker::sendCommand);
    ThreadManager::instance().registerThread(m_sendThread, "OrderSendThread", [this]()
    {
        if (m_sendWorker)
        {
            m_sendWorker->requestStop();
        }
        if (m_sendThread && m_sendThread->isRunning())
        {
            m_sendThread->quit();
        }
    });
    //连接搜索框的信号
    connect(ui->lineEdit_searchOrder, &QLineEdit::textChanged, this, &OrderSendWidget::filterComboBoxItems);
    // 指令选择变化时，更新 groupBox_5 的超时显示
    connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &OrderSendWidget::updateTimeoutUIForSelection);
    // 勾选框变化时，禁用/启用对应的编辑框
    connect(ui->isReplyTimeout, &QCheckBox::toggled, this, &OrderSendWidget::updateTimeoutEditEnableState);
    connect(ui->isReturnTimeout, &QCheckBox::toggled, this, &OrderSendWidget::updateTimeoutEditEnableState);
    connect(ui->isReReplyTimeout, &QCheckBox::toggled, this, &OrderSendWidget::updateTimeoutEditEnableState);
    // 流程模式变化时，同步编辑框可编辑状态
    connect(ui->radioFlowFast, &QRadioButton::toggled, this, &OrderSendWidget::updateTimeoutEditEnableState);
    connect(ui->radioFlowNormal, &QRadioButton::toggled, this, &OrderSendWidget::updateTimeoutEditEnableState);
    connect(ui->radioFlowManual, &QRadioButton::toggled, this, &OrderSendWidget::updateTimeoutEditEnableState);
    // 初始化编辑框禁用状态
    updateTimeoutEditEnableState();
    // 初始化界面锁定标志
    m_uiLocked = false;
    m_sendThread->start();
    qCInfo(OrderSendLog) << "OrderSendWidget 构造完成";
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
    // 刷新默认超时显示与禁用状态
    updateTimeoutUIForSelection();
    updateTimeoutEditEnableState();
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
    // 刷新三项超时显示为当前选择的默认值
    updateTimeoutUIForSelection();
}

void OrderSendWidget::handleSendFinished()
{
    ui->pushButtonstart->setEnabled(true);
    emit logMessage("DEBUG", "指令发送完成");
    emit logMessage("DEBUG", "**********************************************************");
    // 恢复 UI 超时编辑框为默认值，确保本次修改不影响后续发送
    restoreDefaultTimeoutsForSelection();
    if(m_loopRunning)
    {
        //使用QTimer延迟指定时间后发送下一个命令
        int interval = ui->spinLoopInterval ? ui->spinLoopInterval->value() : 2000;
        QTimer::singleShot(interval, this, &OrderSendWidget::sendNextLoopCommand);
    }
    else
    {
        // 单次发送结束，解锁界面
        setUILockedForSending(false);
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
    // 列表初始化完成后，刷新默认超时显示
    updateTimeoutUIForSelection();
    updateTimeoutEditEnableState();
}

/**
 * @brief 根据当前选择的指令更新 groupBox_5 三项超时显示
 */
void OrderSendWidget::updateTimeoutUIForSelection()
{
    if(ui->comboBox->count() <= 0)
    {
        return;
    }
    QString comname = ui->comboBox->currentText();
    // 默认：应答 200ms；反馈来自配置（秒->毫秒）；反馈应答 500ms
    int replyMs = 200;
    int returnMs = static_cast<int>(m_time.value(comname, 0.0) * 1000);
    int reReplyMs = 500;
    ui->lineEdit_yingda->setText(QString::number(replyMs));
    ui->lineEdit_fankui->setText(QString::number(returnMs));
    ui->lineEdit_fankuiyingda->setText(QString::number(reReplyMs));
}

void OrderSendWidget::updateTimeoutEditEnableState()
{
    if(!ui) return;
    // 若界面处于发送锁定，所有相关编辑控件禁用
    if(m_uiLocked)
    {
        ui->lineEdit_yingda->setEnabled(false);
        ui->lineEdit_fankui->setEnabled(false);
        ui->lineEdit_fankuiyingda->setEnabled(false);
        return;
    }
    bool manualMode = ui->radioFlowManual->isChecked();
    ui->lineEdit_yingda->setEnabled(manualMode && !ui->isReplyTimeout->isChecked());
    ui->lineEdit_fankui->setEnabled(manualMode && !ui->isReturnTimeout->isChecked());
    ui->lineEdit_fankuiyingda->setEnabled(manualMode && !ui->isReReplyTimeout->isChecked());
}

/**
 * @brief 发送完成后恢复当前指令的默认超时显示
 */
void OrderSendWidget::restoreDefaultTimeoutsForSelection()
{
//    updateTimeoutUIForSelection();
}

/**
 * @brief 主窗口析构函数
 */
OrderSendWidget::~OrderSendWidget()
{
    m_loopRunning = false; //确保循环停止
    if (m_sendWorker)
    {
        m_sendWorker->requestStop();
    }
    if (m_sendThread)
    {
        ThreadManager::instance().unregisterThread(m_sendThread);
        m_sendThread->quit();
        m_sendThread->wait();
    }
    delete m_sendWorker;
    delete ui;
}

/**
 * @brief 发送期间锁定/解锁界面可编辑控件
 *        锁定：禁用倍率设置入口、流程模式选择、超时勾选与编辑框
 *        解锁：恢复到逻辑计算的可编辑状态
 */
void OrderSendWidget::setUILockedForSending(bool locked)
{
    m_uiLocked = locked;
    // 禁用/启用：倍率设置入口（帮助可保留启用）
    ui->btnMultiplierSettings->setEnabled(!locked);
    // 禁用/启用：流程模式切换
    ui->radioFlowFast->setEnabled(!locked);
    ui->radioFlowNormal->setEnabled(!locked);
    ui->radioFlowManual->setEnabled(!locked);
    // 禁用/启用：超时相关勾选
    ui->isReplyTimeout->setEnabled(!locked);
    ui->isReturnTimeout->setEnabled(!locked);
    ui->isReReplyTimeout->setEnabled(!locked);
    // 禁用/启用：循环间隔设置
    if(ui->spinLoopInterval)
    {
        ui->spinLoopInterval->setEnabled(!locked);
    }
    // 编辑框根据当前逻辑或锁定状态刷新
    updateTimeoutEditEnableState();
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
    // 解析三项超时（毫秒），若为空或无效则使用默认值
    auto toValidMs = [](const QString &text, int fallbackMs) -> int {
        bool ok = false;
        int v = text.trimmed().toInt(&ok);
        return (ok && v > 0) ? v : fallbackMs;
    };
    // 默认：应答 200ms；反馈来自配置（秒->毫秒）；反馈应答 500ms
    int defaultReplyMs = 200;
    int defaultReturnMs = static_cast<int>(m_time.value(comname, 0.0) * 1000);
    int defaultReReplyMs = 500;
    int replyMs = toValidMs(ui->lineEdit_yingda->text(), defaultReplyMs);
    int returnMs = toValidMs(ui->lineEdit_fankui->text(), defaultReturnMs);
    int reReplyMs = toValidMs(ui->lineEdit_fankuiyingda->text(), defaultReReplyMs);
    // 根据勾选框确定本次实际延时（用于日志展示）：
    bool replyChecked = ui->isReplyTimeout->isChecked();
    bool returnChecked = ui->isReturnTimeout->isChecked();
    bool reReplyChecked = ui->isReReplyTimeout->isChecked();
    bool anyChecked = replyChecked || returnChecked || reReplyChecked;
    // 流程模式：快速（0.01×默认）、正常（0.8×默认）、手动（精确手动值）
    int flowMode = ui->radioFlowFast->isChecked() ? 1 : (ui->radioFlowNormal->isChecked() ? 2 : 0);
    int effectiveReplyMs = 0, effectiveReturnMs = 0, effectiveReReplyMs = 0;
    if(anyChecked)
    {
        effectiveReplyMs   = replyChecked   ? static_cast<int>(defaultReplyMs   * m_mulChecked) : 0;
        effectiveReturnMs  = returnChecked  ? static_cast<int>(defaultReturnMs  * m_mulChecked) : 0;
        effectiveReReplyMs = reReplyChecked ? static_cast<int>(defaultReReplyMs * m_mulChecked) : 0;
    }
    else
    {
        if(flowMode == 1)
        {
            effectiveReplyMs   = static_cast<int>(defaultReplyMs   * m_mulFast);
            effectiveReturnMs  = static_cast<int>(defaultReturnMs  * m_mulFast);
            effectiveReReplyMs = static_cast<int>(defaultReReplyMs * m_mulFast);
        }
        else if(flowMode == 2)
        {
            effectiveReplyMs   = static_cast<int>(defaultReplyMs   * m_mulNormal);
            effectiveReturnMs  = static_cast<int>(defaultReturnMs  * m_mulNormal);
            effectiveReReplyMs = static_cast<int>(defaultReReplyMs * m_mulNormal);
        }
        else
        {
            effectiveReplyMs   = replyMs;
            effectiveReturnMs  = returnMs;
            effectiveReReplyMs = reReplyMs;
        }
    }
    //禁用按钮防止重复点击
    ui->pushButtonstart->setEnabled(false);
    emit logMessage("DEBUG", "**********************************************************");
    emit logMessage("DEBUG", QString("执行单次发送指令：[%1]").arg(comname));
    emit logMessage("DEBUG", QString("本次使用超时(ms)：应答=%1，反馈=%2，反馈应答=%3")
                                 .arg(effectiveReplyMs).arg(effectiveReturnMs).arg(effectiveReReplyMs));
    // 模式标注：勾选→“超时×1.5/未勾选（不延时）”；未勾选→流程模式或手动
    if(anyChecked)
    {
        const QString mStr = QStringLiteral("超时×%1").arg(QString::number(m_mulChecked, 'g', 3));
        auto modeStrChecked = [&](bool checked)->QString{ return checked ? mStr : QStringLiteral("未勾选（不延时）"); };
        emit logMessage("DEBUG", QString("超时模式：应答=%1，反馈=%2，反馈应答=%3")
                                     .arg(modeStrChecked(replyChecked))
                                     .arg(modeStrChecked(returnChecked))
                                     .arg(modeStrChecked(reReplyChecked)));
    }
    else
    {
        QString flowModeStr = flowMode==1 ? QStringLiteral("快速流程（%1×)").arg(QString::number(m_mulFast, 'g', 3))
                              : (flowMode==2 ? QStringLiteral("正常流程（%1×)").arg(QString::number(m_mulNormal, 'g', 3)) : QStringLiteral("手动配置（精确）"));
        emit logMessage("DEBUG", QString("超时模式（统一）：%1").arg(flowModeStr));
    }
    //通过信号触发子线程发送
    emit startSendCommand(address, port, comname,
                          m_commandId, m_workId, m_time,
                          ui->noCRC->isChecked(), ui->reNOCRC->isChecked(),
                          ui->noreplay->isChecked(), ui->noreturn->isChecked(),
                          ui->noReturnReply->isChecked(), ui->replyNOCRC->isChecked(),
                          ui->ReERR->isChecked(), ui->noShake->isChecked(),
                          ui->isReplyTimeout->isChecked(), ui->isReturnTimeout->isChecked(),
                          ui->isReReplyTimeout->isChecked(),
                          replyMs, returnMs, reReplyMs,
                          flowMode);
    // 开始发送后立刻锁定相关界面控件
    setUILockedForSending(true);
}

void OrderSendWidget::loadMultipliersFromConfig()
{
    // 回退默认值：来自 alldefine.h 常量
    m_mulFast = m_flowConfig->get("flowFastMultiplier", FLOW_FAST_MULTIPLIER).toDouble();
    m_mulNormal = m_flowConfig->get("flowNormalMultiplier", FLOW_NORMAL_MULTIPLIER).toDouble();
    m_mulChecked = m_flowConfig->get("timeoutCheckedMultiplier", TIMEOUT_CHECKED_MULTIPLIER).toDouble();
    // 安全性：限制范围（>0），避免非法配置导致负/零倍率
    if(m_mulFast <= 0) m_mulFast = FLOW_FAST_MULTIPLIER;
    if(m_mulNormal <= 0) m_mulNormal = FLOW_NORMAL_MULTIPLIER;
    if(m_mulChecked <= 0) m_mulChecked = TIMEOUT_CHECKED_MULTIPLIER;
}

void OrderSendWidget::applyMultiplierTextToUI()
{
    auto fmtMul = [](double m){ return QString::number(m, 'g', 3); };
    ui->radioFlowFast->setToolTip(QString("使用默认超时时间的 FAST_MULTIPLIER 倍（当前：%1×）。禁用手动输入。").arg(fmtMul(m_mulFast)));
    ui->radioFlowNormal->setToolTip(QString("使用默认超时时间的 NORMAL_MULTIPLIER 倍（当前：%1×）。禁用手动输入。").arg(fmtMul(m_mulNormal)));
    ui->radioFlowManual->setToolTip(QString("启用手动输入，按输入的毫秒值精确控制超时。"));
    if(ui->labelFlowHelp)
    {
        ui->labelFlowHelp->setText(QString("提示：勾选“应答/反馈/反馈应答超时”时，按超时×%1；未勾选时按流程模式统一应用（快速：%2×，正常：%3×，手动：按输入毫秒）。")
                                   .arg(fmtMul(m_mulChecked))
                                   .arg(fmtMul(m_mulFast))
                                   .arg(fmtMul(m_mulNormal)));
        ui->labelFlowHelp->setWordWrap(true);
    }
}

/**
 * @brief 打开倍率设置对话框（带校验与保存）
 */
void OrderSendWidget::onOpenMultiplierSettings()
{
    // 发送中禁止打开设置，避免修改倍率
    if(m_uiLocked)
    {
        QMessageBox::information(this, "提示", "正在发送中，倍率设置暂不可修改。");
        return;
    }
    MultiplierSettingsDialog dlg(m_flowConfig, this);
    dlg.exec();
}

/**
 * @brief 打开倍率帮助对话框（集中说明与恢复默认）
 */
void OrderSendWidget::onOpenMultiplierHelp()
{
    MultiplierHelpDialog dlg(m_flowConfig, this);
    dlg.exec();
}

/**
 * @brief 循环发送按钮点击处理
 */
void OrderSendWidget::on_pushButton_clicked()
{
    emit logMessage("DEBUG", "循环发送开始，在点击停止按钮前会重复遍历指令列表并发送数据");
    ui->pushButtonStop->setEnabled(true);
    ui->pushButton->setEnabled(false);
    ui->btnLoopSingle->setEnabled(false);
    ui->pushButtonstart->setEnabled(false);
    m_loopRunning = true;
    m_isSingleLoopMode = false;
    m_currentLoopIndex = 0;
    // 循环发送期间，保持界面锁定
    setUILockedForSending(true);
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
        ui->btnLoopSingle->setEnabled(true);
        ui->pushButtonstart->setEnabled(true);
        // 解锁界面
        setUILockedForSending(false);
        m_currentLoopIndex = 0;
        emit logMessage("DEBUG", "********************************************************");
        emit logMessage("DEBUG", "******************循环发送完成，发送已停止******************");
        emit logMessage("DEBUG", "********************************************************");
        return;
    }

    // 单指令循环模式处理
    if(m_isSingleLoopMode)
    {
        // 直接复用单次发送逻辑，无需切换index
        on_pushButtonstart_clicked();
        return;
    }

    if(m_loopRunning && m_currentLoopIndex >= ui->comboBox->count())
    {
        m_currentLoopIndex = 0;
        emit logMessage("DEBUG", "***************************************************************************");
        emit logMessage("DEBUG", "******************列表循环发送完成，等待一分钟后自动进行下一次循环******************");
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
    ui->btnLoopSingle->setEnabled(true);
    ui->pushButtonstart->setEnabled(true);
    // 停止后解锁界面
    setUILockedForSending(false);
    emit logMessage("DEBUG", "点击停止按钮，循环发送已停止");
}

/**
 * @brief 单个循环发送按钮点击处理
 */
void OrderSendWidget::on_btnLoopSingle_clicked()
{
    emit logMessage("DEBUG", "单指令循环发送开始，在点击停止按钮前会重复发送当前指令");
    ui->pushButtonStop->setEnabled(true);
    ui->pushButton->setEnabled(false);
    ui->btnLoopSingle->setEnabled(false);
    ui->pushButtonstart->setEnabled(false);
    m_loopRunning = true;
    m_isSingleLoopMode = true;
    // 循环发送期间，保持界面锁定
    setUILockedForSending(true);
    on_pushButtonstart_clicked(); //开始第一次发送
}

