#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QDateTime>
#include <QRegularExpression>
#include <QNetworkInterface>
#include <QLoggingCategory>
#include <QComboBox>
#include <QStatusBar>

//定义日志分类
Q_LOGGING_CATEGORY(mainWindowLog, "[app.MainWindow]")

/**
 * @brief MainWindow构造函数
 * @param parent 父窗口指针
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow()),
      m_currentFilterLevel("ALL")  //默认显示所有日志级别
{
    ui->setupUi(this);
    //设置任务栏图标
    setWindowIcon(QIcon(":/icons/icons/app.png"));
    //初始化界面和配置
    initTab_2();            //初始化第二个标签页
    initUI();               //初始化主界面
    loadConfig();           //加载配置文件
    setupConnections();     //建立信号槽连接
    setupLogFilter();       //设置日志过滤器
    updateButtonStates(false, false);  //更新按钮状态
    //加载并应用暗黑主题样式表
    loadAndApplyStyleSheet(":/styles/styles/drak_theme.qss");
    //配置保存防抖定时器（500毫秒单次触发）
    m_saveTimer.setInterval(500);
    m_saveTimer.setSingleShot(true);
}

/**
 * @brief MainWindow析构函数
 */
MainWindow::~MainWindow()
{
    //停止工作线程并释放资源
    if (m_worker)
    {
        m_worker->stopProcessing();
    }
    if (m_workerThread.isRunning())
    {
        m_workerThread.quit();
        m_workerThread.wait();
    }
}

/**
 * @brief 加载并应用样式表
 * @param styleSheetPath 样式表文件路径
 */
void MainWindow::loadAndApplyStyleSheet(const QString &styleSheetPath)
{
    QFile file(styleSheetPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QString styleSheet = file.readAll();
        this->setStyleSheet(styleSheet);
        file.close();
    }
    else
    {
        qCWarning(mainWindowLog) << "无法加载样式表文件：" << styleSheetPath;
    }
}

/**
 * @brief 应用程序退出前的清理工作
 */
void MainWindow::cleanupBeforeExit()
{
    qCInfo(mainWindowLog) << "正在执行退出前的清理...";
    //保存当前配置
    saveConfig();
    //释放资源（如果有）
    if (m_workerThread.isRunning())
    {
        m_workerThread.wait();
    }
    qCInfo(mainWindowLog) << "清理完成。";
}

/**
 * @brief 窗口关闭事件处理
 * @param event 关闭事件对象
 */
void MainWindow::closeEvent(QCloseEvent *event)
{
    //保存配置并接受关闭事件
    saveConfig();
    event->accept();
}

/**
 * @brief 初始化第二个标签页
 */
void MainWindow::initTab_2()
{
    //初始化第二个标签页的组件
    new CustomDataSender(ui->textEdit,
                         ui->addrEdit_2,
                         ui->sendIntervalEdit_2,
                         ui->uniqueCheck_2,
                         ui->startButton_2,
                         ui->pauseButton_2,
                         ui->stopButton_2,
                         ui->textEdit_2,
                         this);
}

/**
 * @brief 初始化主界面
 */
void MainWindow::initUI()
{
    //设置窗口标题和大小
    setWindowTitle("数据处理工具");
    resize(1000, 800);
    //设置状态栏初始文本
    ui->statusLabel->setText("就绪");
    //添加日志过滤下拉框
    m_pLogFilterCombo = new QComboBox(this);
    m_pLogFilterCombo->addItem("全部", "ALL");
    m_pLogFilterCombo->addItem("调试", "DEBUG");
    m_pLogFilterCombo->addItem("信息", "INFO");
    m_pLogFilterCombo->addItem("警告", "WARN");
    m_pLogFilterCombo->addItem("错误", "ERROR");
    m_pLogFilterCombo->setCurrentIndex(0);
    //将下拉框添加到日志视图上方的工具栏
    QToolBar *logToolBar = new QToolBar(this);
    logToolBar->addWidget(new QLabel("日志等级过滤:"));
    logToolBar->addWidget(m_pLogFilterCombo);
    addToolBar(Qt::TopToolBarArea, logToolBar);
}

/**
 * @brief 加载配置文件
 */
void MainWindow::loadConfig()
{
    //从配置管理器中加载配置
    QVariantMap config = m_configManager.getConfig();
    ui->dirEdit->setText(config["dataDir"].toString());
    ui->orderCombo->setCurrentText(config["order"].toString());
    ui->includeEdit->setText(config["includeTopics"].toStringList().join(","));
    ui->excludeEdit->setText(config["excludeTopics"].toStringList().join(","));
    ui->uniqueCheck->setChecked(config["uniqueMode"].toBool());
    ui->sendIntervalEdit->setText(config["sendInterval"].toString());
    //加载地址列表
    for (const auto &addr : config["addresses"].toStringList())
    {
        //过滤空行
        if(!addr.trimmed().isEmpty())
        {
            ui->addrList->addItem(addr.trimmed());
        }
    }
}

/**
 * @brief 保存配置文件
 */
void MainWindow::saveConfig()
{
    //保存配置到配置文件
    QVariantMap config;
    config["dataDir"] = ui->dirEdit->text();
    config["order"] = ui->orderCombo->currentText();
    config["includeTopics"] = ui->includeEdit->text().split(',', QString::SkipEmptyParts);
    config["excludeTopics"] = ui->excludeEdit->text().split(',', QString::SkipEmptyParts);
    config["uniqueMode"] = ui->uniqueCheck->isChecked();
    config["sendInterval"] = ui->sendIntervalEdit->text();
    //保存地址列表
    QStringList addrList = getAddressList();
    config["addresses"] = addrList;
    //更新配置
    m_configManager.updateConfig(config);
}

/**
 * @brief 获取地址列表所有行的文本内容
 * @return 地址列表字符串列表
 */
QStringList MainWindow::getAddressList()
{
    QStringList list;
    //遍历所有项目
    for(int i = 0; i < ui->addrList->count(); ++i)
    {
        //获取列表项指针
        QListWidgetItem* item = ui->addrList->item(i);
        if(item && !item->text().isEmpty())   //跳过空项
        {
            list.append(item->text());
        }
    }
    return list;
}

/**
 * @brief 获取地址列表选中行的文本内容
 * @return 选中地址的字符串列表
 */
QStringList MainWindow::getSelectedAddressList()
{
    QStringList list;
    //直接获取选中项的文本（不修改列表）
    foreach (QListWidgetItem* item, ui->addrList->selectedItems())
    {
        if(item && !item->text().isEmpty())
        {
            list.append(item->text());
        }
    }
    return list;
}

/**
 * @brief 建立信号槽连接
 */
void MainWindow::setupConnections()
{
    //连接日志过滤下拉框信号
    connect(m_pLogFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onLogFilterChanged);
    //连接按钮点击信号
    connect(ui->browseButton, &QPushButton::clicked, this, &MainWindow::onBrowseClicked);
    connect(ui->addAddrButton, &QPushButton::clicked, this, &MainWindow::onAddAddressClicked);
    connect(ui->removeAddrButton, &QPushButton::clicked, this, &MainWindow::onRemoveAddressClicked);
    connect(ui->startButton, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(ui->pauseButton, &QPushButton::clicked, this, &MainWindow::onPauseClicked);
    connect(ui->stopButton, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    //连接自动保存配置定时器
    connect(&m_saveTimer, &QTimer::timeout, this, &MainWindow::saveConfig);
}

/**
 * @brief 设置日志过滤器
 */
void MainWindow::setupLogFilter()
{
    //初始化日志格式
    m_logFormat.setFont(QFont("Consolas", 10));
}

/**
 * @brief 浏览按钮点击事件处理
 */
void MainWindow::onBrowseClicked()
{
    //打开目录选择对话框
    QString dir = QFileDialog::getExistingDirectory(this, "选择数据目录", QDir::homePath());
    if (!dir.isEmpty())
    {
        ui->dirEdit->setText(dir);
    }
}

/**
 * @brief 添加地址按钮点击事件处理
 */
void MainWindow::onAddAddressClicked()
{
    //添加地址到列表
    const QString addr = ui->addrEdit->text().trimmed();
    QHostAddress ip;
    quint16 port = 0;
    //验证地址格式
    if (!validateAddress(addr, ip, port))
    {
        QMessageBox::warning(this, "无效地址", "请输入有效的 IP:端口 格式");
        return;
    }
    //检查地址是否重复
    if (isDuplicateAddress(addr))
    {
        QMessageBox::warning(this, "重复地址", "该地址已存在于列表中");
        return;
    }
    //添加地址到 UI
    addAddressToUI(addr);
}

/**
 * @brief 验证地址格式
 * @param addr 待验证的地址字符串
 * @param ip 输出参数，解析出的IP地址
 * @param port 输出参数，解析出的端口号
 * @return 验证通过返回true，否则返回false
 */
bool MainWindow::validateAddress(const QString &addr, QHostAddress &ip, quint16 &port)
{
    //验证地址格式是否为 IP:端口
    QStringList parts = addr.split(':');
    if (parts.size() != 2)
    {
        return false;
    }
    if (!ip.setAddress(parts[0]))
    {
        return false;
    }
    bool ok;
    port = parts[1].toUShort(&ok);
    return ok;
}

/**
 * @brief 检查地址是否重复
 * @param addr 待检查的地址
 * @return 如果地址已存在返回true，否则返回false
 */
bool MainWindow::isDuplicateAddress(const QString &addr) const
{
    //检查地址是否已存在
    for (int i = 0; i < ui->addrList->count(); ++i)
    {
        if (ui->addrList->item(i)->text() == addr)
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief 添加地址到UI列表
 * @param addr 要添加的地址
 */
void MainWindow::addAddressToUI(const QString &addr)
{
    //添加地址到列表并清空输入框
    ui->addrList->addItem(addr.trimmed());
    ui->addrEdit->clear();
}

/**
 * @brief 删除地址按钮点击事件处理
 */
void MainWindow::onRemoveAddressClicked()
{
    //删除所有选中的项目
    QList<QListWidgetItem *> selectedItems = ui->addrList->selectedItems();
    foreach (QListWidgetItem* item, selectedItems)
    {
        delete ui->addrList->takeItem(ui->addrList->row(item));
    }
}

/**
 * @brief 开始按钮点击事件处理
 */
void MainWindow::onStartClicked()
{
    //获取选中的组播地址
    QStringList addrList = getSelectedAddressList();
    //启动工作线程
    if (!validateConfig(addrList))
    {
        return;
    }
    ui->logView->clear();
    saveConfig();
    //创建工作类对象并移动到工作线程
    m_worker.reset(new WorkerClass);
    m_worker->setAddrList(addrList);
    m_worker->configure(m_configManager.getConfig());
    m_worker->moveToThread(&m_workerThread);
    //连接信号槽并启动线程
    connectWorkerSlots();
    updateButtonStates(true, false);
    m_workerThread.start();
}

/**
 * @brief 暂停按钮点击事件处理
 */
void MainWindow::onPauseClicked()
{
    //暂停或恢复工作线程
    if (m_worker->isRunning())
    {
        m_worker->pauseProcessing(!m_worker->isPaused());
        ui->pauseButton->setText(m_worker->isPaused() ? "继续(&R)" : "暂停(&P)");
        updateButtonStates(m_worker->isRunning(), m_worker->isPaused());
    }
}

/**
 * @brief 停止按钮点击事件处理
 */
void MainWindow::onStopClicked()
{
    //停止工作线程
    if (m_worker)
    {
        qCDebug(mainWindowLog) << "正在停止工作线程并清理资源...";
        m_worker->stopProcessing();
        m_workerThread.quit();
        m_workerThread.wait();
        disConnectWorkerSlots();
        m_worker.reset();
        updateButtonStates(false, false);
        qCDebug(mainWindowLog) << "工作线程和资源已完全清理。";
    }
}

/**
 * @brief 连接工作线程信号槽
 */
void MainWindow::connectWorkerSlots()
{
    //连接工作线程信号槽
    connect(&m_workerThread, &QThread::started, m_worker.get(), &WorkerClass::startProcessing);
    connect(&m_workerThread, &QThread::quit, m_worker.get(), &WorkerClass::stopProcessing);
    connect(m_worker.get(), &WorkerClass::finished, &m_workerThread, &QThread::quit);
    connect(m_worker.get(), &WorkerClass::statsUpdated, this, &MainWindow::handleStats);
    connect(m_worker.get(), &WorkerClass::logMessage, this, &MainWindow::handleLog);
    connect(m_worker.get(), &WorkerClass::progressUpdated, this, &MainWindow::handleProgress);
}

/**
 * @brief 断开工作线程信号槽
 */
void MainWindow::disConnectWorkerSlots()
{
    //断开工作线程信号槽
    disconnect(&m_workerThread, &QThread::started, m_worker.get(), &WorkerClass::startProcessing);
    disconnect(&m_workerThread, &QThread::quit, m_worker.get(), nullptr);
    if (m_worker)
    {
        disconnect(m_worker.get(), &WorkerClass::finished, &m_workerThread, &QThread::quit);
        disconnect(m_worker.get(), &WorkerClass::statsUpdated, this, &MainWindow::handleStats);
        disconnect(m_worker.get(), &WorkerClass::logMessage, this, &MainWindow::handleLog);
        disconnect(m_worker.get(), &WorkerClass::progressUpdated, this, &MainWindow::handleProgress);
        disconnect(m_worker.get(), nullptr, this, nullptr);
    }
}

/**
 * @brief 处理日志消息
 * @param level 日志级别
 * @param msg 日志内容
 */
void MainWindow::handleLog(const QString &level, const QString &msg)
{
    LogEntry entry{level.toUpper(), msg, QDateTime::currentDateTime()};
    m_logEntriesByLevel[level].append(entry);
    //单类日志达到规定数量则清理旧日志
    if (m_logEntriesByLevel[level].size() > MAX_LOG_ENTRIES)
    {
        m_logEntriesByLevel[level].removeFirst();
    }
    //根据过滤级别显示日志
    if (m_currentFilterLevel == "ALL" || entry.level == m_currentFilterLevel)
    {
        QString html = QString("<div style='color:%1'>[%2] [%3] %4</div>")
                       .arg(colorForLevel(level).name(),
                            entry.time.toString(dt_format),
                            level,
                            msg.toHtmlEscaped());
        appendLogToView(html);
    }
}

/**
 * @brief 处理进度更新
 * @param value 进度值(0-100)
 */
void MainWindow::handleProgress(int value)
{
    //更新进度条
    ui->progressBar->setValue(value);
}

/**
 * @brief 处理统计信息更新
 * @param success 成功次数
 * @param failed 失败次数
 */
void MainWindow::handleStats(int success, int failed)
{
    //更新统计信息
    ui->statusLabel->setText(QString("成功: %1   失败: %2").arg(success).arg(failed));
}

/**
 * @brief 刷新日志视图
 */
void MainWindow::refreshLogView()
{
    //清空日志视图
    ui->logView->clear();
    //如果当前过滤级别是 "ALL"，则显示所有日志
    if (m_currentFilterLevel == "ALL")
    {
        for (const auto &level : m_logEntriesByLevel.keys())
        {
            const QList<LogEntry> &entries = m_logEntriesByLevel[level];
            for (const LogEntry &entry : entries)
            {
                QString html = QString("<div style='color:%1'>[%2] [%3] %4</div>")
                               .arg(colorForLevel(entry.level).name(),
                                    entry.time.toString(dt_format),
                                    entry.level,
                                    entry.message.toHtmlEscaped());
                appendLogToView(html);
            }
        }
    }
    else
    {
        //否则，只显示当前过滤级别的日志
        if (m_logEntriesByLevel.contains(m_currentFilterLevel))
        {
            const QList<LogEntry> &entries = m_logEntriesByLevel[m_currentFilterLevel];
            for (const LogEntry &entry : entries)
            {
                QString html = QString("<div style='color:%1'>[%2] [%3] %4</div>")
                               .arg(colorForLevel(entry.level).name(),
                                    entry.time.toString(dt_format),
                                    entry.level,
                                    entry.message.toHtmlEscaped());
                appendLogToView(html);
            }
        }
    }
}

/**
 * @brief 日志过滤级别改变事件处理
 * @param index 新的过滤级别索引
 */
void MainWindow::onLogFilterChanged(int index)
{
    //更新日志过滤级别
    m_currentFilterLevel = m_pLogFilterCombo->itemData(index).toString();
    refreshLogView();
}

/**
 * @brief 更新按钮状态
 * @param isRunning 是否正在运行
 * @param isPaused 是否已暂停
 */
void MainWindow::updateButtonStates(bool isRunning, bool isPaused)
{
    //更新按钮状态
    ui->startButton->setEnabled(!isRunning);
    ui->pauseButton->setEnabled(isRunning);
    ui->stopButton->setEnabled(isRunning && !isPaused);
    ui->browseButton->setEnabled(!isRunning);
    ui->addAddrButton->setEnabled(!isRunning);
    ui->removeAddrButton->setEnabled(!isRunning);
}

/**
 * @brief 验证配置是否有效
 * @param addrList 地址列表
 * @return 配置有效返回true，否则返回false
 */
bool MainWindow::validateConfig(const QStringList addrList)
{
    //验证配置是否有效
    if (ui->dirEdit->text().isEmpty())
    {
        QMessageBox::warning(this, "配置错误", "请选择数据目录");
        return false;
    }
    if (addrList.isEmpty())
    {
        QMessageBox::warning(this, "配置错误", "请添加选中一个目标地址");
        return false;
    }
    return true;
}

/**
 * @brief 根据日志级别获取颜色
 * @param level 日志级别
 * @return 对应的颜色
 */
QColor MainWindow::colorForLevel(const QString &level) const
{
    //根据日志级别返回颜色
    if (level == "INFO")
    {
        return m_logColors.info;
    }
    if (level == "WARN")
    {
        return m_logColors.warn;
    }
    if (level == "ERROR")
    {
        return m_logColors.error;
    }
    if (level == "DEBUG")
    {
        return m_logColors.debug;
    }
    return m_logColors.defualt;
}

/**
 * @brief 添加日志到视图
 * @param html 格式化的HTML日志内容
 */
void MainWindow::appendLogToView(const QString &html)
{
    //添加日志到视图
    QTextCursor cursor(ui->logView->document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertHtml(html);
    cursor.insertBlock();
    ui->logView->ensureCursorVisible();
}
