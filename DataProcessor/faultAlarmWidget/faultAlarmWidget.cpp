#include "faultAlarmWidget.h"
#include <QApplication>
#include <QFile>
#include <QJsonArray>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QCryptographicHash>
#include <QRandomGenerator>

/**
 * @brief FaultAlarmWidget构造函数
 * @param parent 父窗口指针
 */
FaultAlarmWidget::FaultAlarmWidget(QWidget *parent)
    : QWidget(parent)
    , ui(nullptr)
    , m_udpSocket(new QUdpSocket(this))
    , m_sendTimer(new QTimer(this))
    , m_saveTimer(new QTimer(this))
    , m_isContinuousSending(false)
{
    // 配置保存定时器（防抖机制）
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(500); // 500ms延迟
    initUI();
    initConnections();
    loadStyleSheet();
    updateUI();
    updateSendButtonStates();
    loadConfiguration(); // 在UI初始化完成后加载配置
}

/**
 * @brief FaultAlarmWidget析构函数
 */
FaultAlarmWidget::~FaultAlarmWidget()
{
    saveConfiguration();
    if(m_isContinuousSending)
    {
        m_sendTimer->stop();
    }
}

/**
 * @brief 初始化用户界面
 */
void FaultAlarmWidget::initUI()
{
    setWindowTitle("故障告警数据帧处理工具");
    setMinimumSize(1000, 700);
    // 创建主布局
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    // 创建分割器
    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal, this);
    // 创建左侧配置区域
    QWidget *configWidget = new QWidget();
    QVBoxLayout *configLayout = new QVBoxLayout(configWidget);
    // 创建数据配置部分
    m_dataConfigWidget = createDataConfigWidget();
    configLayout->addWidget(m_dataConfigWidget);
    // 创建发送配置部分
    m_sendConfigWidget = createSendConfigWidget();
    configLayout->addWidget(m_sendConfigWidget);
    // 创建右侧日志显示区域
    m_logWidget = createLogWidget();
    // 添加到分割器
    mainSplitter->addWidget(configWidget);
    mainSplitter->addWidget(m_logWidget);
    mainSplitter->setStretchFactor(0, 2);
    mainSplitter->setStretchFactor(1, 1);
    mainLayout->addWidget(mainSplitter);
}

/**
 * @brief 创建数据配置部分界面
 * @return 数据配置部件指针
 */
QWidget* FaultAlarmWidget::createDataConfigWidget()
{
    QGroupBox *groupBox = new QGroupBox("数据配置");
    QGridLayout *layout = new QGridLayout(groupBox);
    int row = 0;
    // 控制位设置
    layout->addWidget(new QLabel("控制位:"), row, 0);
    QHBoxLayout *controlLayout = new QHBoxLayout();
    m_controlCombo = new QComboBox();
    m_controlCombo->addItem("0", 0);
    m_controlCombo->addItem("1", 1);
    m_controlCombo->setCurrentIndex(1); // 默认选择1
    controlLayout->addWidget(m_controlCombo);
    layout->addLayout(controlLayout, row++, 1);
    // 数值输入格式选择
    layout->addWidget(new QLabel("数值格式:"), row, 0);
    QHBoxLayout *formatLayout = new QHBoxLayout();
    m_formatCombo = new QComboBox();
    m_formatCombo->addItem("十进制", 0);
    m_formatCombo->addItem("十六进制", 1);
    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FaultAlarmWidget::onFormatChanged);
    formatLayout->addWidget(m_formatCombo);
    layout->addLayout(formatLayout, row++, 1);
    // 告警类型选择
    layout->addWidget(new QLabel("告警类型:"), row, 0);
    m_alarmTypeCombo = new QComboBox();
    m_alarmTypeCombo->addItem("故障码", 0x01);
    m_alarmTypeCombo->addItem("预警码", 0x02);
    layout->addWidget(m_alarmTypeCombo, row++, 1);
    // 故障码/预警码
    layout->addWidget(new QLabel("故障码/预警码:"), row, 0);
    m_faultCodeSpin = new QSpinBox();
    m_faultCodeSpin->setRange(0, 65535);
    m_faultCodeSpin->setValue(1001);
    m_faultCodeSpin->setDisplayIntegerBase(10); // 默认十进制显示
    layout->addWidget(m_faultCodeSpin, row++, 1);
    // 隔离标志
    layout->addWidget(new QLabel("隔离标志:"), row, 0);
    m_isolationCombo = new QComboBox();
    m_isolationCombo->addItem("预警码(填0)", 0x00);
    m_isolationCombo->addItem("可能与外系统有关", 0x01);
    m_isolationCombo->addItem("系统内部故障", 0x02);
    layout->addWidget(m_isolationCombo, row++, 1);
    // 故障等级
    layout->addWidget(new QLabel("故障等级:"), row, 0);
    m_faultLevelCombo = new QComboBox();
    m_faultLevelCombo->addItem("预警码(填0)", 0x00);
    m_faultLevelCombo->addItem("1类故障", 0x01);
    m_faultLevelCombo->addItem("2类故障", 0x02);
    m_faultLevelCombo->addItem("未知故障", 0x03);
    layout->addWidget(m_faultLevelCombo, row++, 1);
    // 预警数值
    layout->addWidget(new QLabel("预警数值:"), row, 0);
    m_warningValueSpin = new QDoubleSpinBox();
    m_warningValueSpin->setRange(0, 65535);
    m_warningValueSpin->setDecimals(2);
    m_warningValueSpin->setValue(100.0);
    layout->addWidget(m_warningValueSpin, row++, 1);
    // 预警阈值
    layout->addWidget(new QLabel("预警阈值:"), row, 0);
    m_warningThresholdSpin = new QDoubleSpinBox();
    m_warningThresholdSpin->setRange(0, 65535);
    m_warningThresholdSpin->setDecimals(2);
    m_warningThresholdSpin->setValue(150.0);
    layout->addWidget(m_warningThresholdSpin, row++, 1);
    // 主题号
    layout->addWidget(new QLabel("主题号:"), row, 0);
    m_topicNumberSpin = new QSpinBox();
    m_topicNumberSpin->setRange(0, 65535);
    m_topicNumberSpin->setValue(1000);
    m_topicNumberSpin->setDisplayIntegerBase(10); // 默认十进制显示
    layout->addWidget(m_topicNumberSpin, row++, 1);
    // 源设备号
    layout->addWidget(new QLabel("源设备号:"), row, 0);
    m_sourceDeviceSpin = new QSpinBox();
    m_sourceDeviceSpin->setRange(0, 255);
    m_sourceDeviceSpin->setValue(1);
    m_sourceDeviceSpin->setDisplayIntegerBase(10); // 默认十进制显示
    layout->addWidget(m_sourceDeviceSpin, row++, 1);
    // 目的设备号
    layout->addWidget(new QLabel("目的设备号:"), row, 0);
    m_destDeviceSpin = new QSpinBox();
    m_destDeviceSpin->setRange(0, 255);
    m_destDeviceSpin->setValue(2);
    m_destDeviceSpin->setDisplayIntegerBase(10); // 默认十进制显示
    layout->addWidget(m_destDeviceSpin, row++, 1);
    // 唯一标识码分段输入
    layout->addWidget(new QLabel("唯一标识码:"), row, 0);
    // 创建唯一标识码分段输入的垂直布局
    QVBoxLayout *uniqueIdMainLayout = new QVBoxLayout();
    // 第一行：组织机构代码
    QHBoxLayout *orgCodeLayout = new QHBoxLayout();
    orgCodeLayout->addWidget(new QLabel("组织机构代码(9位):"));
    m_orgCodeEdit = new QLineEdit();
    m_orgCodeEdit->setMaxLength(9);
    m_orgCodeEdit->setPlaceholderText("9位组织机构代码");
    m_orgCodeEdit->setText("633772342"); // 默认值
    orgCodeLayout->addWidget(m_orgCodeEdit);
    uniqueIdMainLayout->addLayout(orgCodeLayout);
    // 第二行：生产日期
    QHBoxLayout *dateLayout = new QHBoxLayout();
    dateLayout->addWidget(new QLabel("生产日期(8位):"));
    m_productDateEdit = new QLineEdit();
    m_productDateEdit->setMaxLength(8);
    m_productDateEdit->setPlaceholderText("YYYYMMDD格式");
    m_productDateEdit->setText(QDate::currentDate().toString("yyyyMMdd")); // 默认当前日期
    QPushButton *todayBtn = new QPushButton("今天");
    todayBtn->setMaximumWidth(50);
    connect(todayBtn, &QPushButton::clicked, [this]()
    {
        m_productDateEdit->setText(QDate::currentDate().toString("yyyyMMdd"));
        onUniqueIdSegmentChanged();
    });
    dateLayout->addWidget(m_productDateEdit);
    dateLayout->addWidget(todayBtn);
    uniqueIdMainLayout->addLayout(dateLayout);
    // 第三行：序列码
    QHBoxLayout *serialLayout = new QHBoxLayout();
    serialLayout->addWidget(new QLabel("序列码(6位):"));
    m_serialCodeEdit = new QLineEdit();
    m_serialCodeEdit->setMaxLength(6);
    m_serialCodeEdit->setPlaceholderText("6位序列码");
    m_serialCodeEdit->setText("SG1234"); // 默认值
    QPushButton *randomBtn = new QPushButton("随机");
    randomBtn->setMaximumWidth(50);
    connect(randomBtn, &QPushButton::clicked, [this]()
    {
        QString serialCode = QString("SG%1").arg(QRandomGenerator::global()->bounded(1000, 9999));
        m_serialCodeEdit->setText(serialCode);
        onUniqueIdSegmentChanged();
    });
    serialLayout->addWidget(m_serialCodeEdit);
    serialLayout->addWidget(randomBtn);
    uniqueIdMainLayout->addLayout(serialLayout);
    // 第四行：校验位
    QHBoxLayout *checkLayout = new QHBoxLayout();
    checkLayout->addWidget(new QLabel("校验位(1位):"));
    m_checkCodeEdit = new QLineEdit();
    m_checkCodeEdit->setMaxLength(1);
    m_checkCodeEdit->setPlaceholderText("1位校验码");
    m_checkCodeEdit->setReadOnly(true); // 校验位自动计算，只读
    QPushButton *calcBtn = new QPushButton("计算");
    calcBtn->setMaximumWidth(50);
    connect(calcBtn, &QPushButton::clicked, this, &FaultAlarmWidget::onCalculateCheckCode);
    checkLayout->addWidget(m_checkCodeEdit);
    checkLayout->addWidget(calcBtn);
    uniqueIdMainLayout->addLayout(checkLayout);
    // 第五行：最终结果
    QHBoxLayout *resultLayout = new QHBoxLayout();
    resultLayout->addWidget(new QLabel("最终结果(24位):"));
    m_uniqueIdResultEdit = new QLineEdit();
    m_uniqueIdResultEdit->setMaxLength(24);
    m_uniqueIdResultEdit->setPlaceholderText("24位完整唯一标识码");
    QPushButton *generateBtn = new QPushButton("生成");
    generateBtn->setMaximumWidth(60);
    connect(generateBtn, &QPushButton::clicked, this, &FaultAlarmWidget::onGenerateUniqueId);
    resultLayout->addWidget(m_uniqueIdResultEdit);
    resultLayout->addWidget(generateBtn);
    uniqueIdMainLayout->addLayout(resultLayout);
    layout->addLayout(uniqueIdMainLayout, row++, 1);
    // 操作按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *resetBtn = new QPushButton("重置数据");
    QPushButton *loadBtn = new QPushButton("加载预设");
    QPushButton *saveBtn = new QPushButton("保存预设");
    connect(resetBtn, &QPushButton::clicked, this, &FaultAlarmWidget::onResetData);
    connect(loadBtn, &QPushButton::clicked, this, &FaultAlarmWidget::onLoadPreset);
    connect(saveBtn, &QPushButton::clicked, this, &FaultAlarmWidget::onSavePreset);
    buttonLayout->addWidget(resetBtn);
    buttonLayout->addWidget(loadBtn);
    buttonLayout->addWidget(saveBtn);
    layout->addLayout(buttonLayout, row++, 0, 1, 2);
    return groupBox;
}

/**
 * @brief 创建发送配置部分界面
 * @return 发送配置部件指针
 */
QWidget* FaultAlarmWidget::createSendConfigWidget()
{
    QGroupBox *groupBox = new QGroupBox("发送配置");
    QGridLayout *layout = new QGridLayout(groupBox);
    int row = 0;
    // 目标IP地址
    layout->addWidget(new QLabel("目标IP:"), row, 0);
    m_targetIpEdit = new QLineEdit();
    m_targetIpEdit->setText("127.0.0.1");
    m_targetIpEdit->setPlaceholderText("目标IP地址");
    layout->addWidget(m_targetIpEdit, row++, 1);
    // 目标端口
    layout->addWidget(new QLabel("目标端口:"), row, 0);
    m_targetPortSpin = new QSpinBox();
    m_targetPortSpin->setRange(1, 65535);
    m_targetPortSpin->setValue(8080);
    layout->addWidget(m_targetPortSpin, row++, 1);
    // 发送间隔
    layout->addWidget(new QLabel("发送间隔(ms):"), row, 0);
    m_intervalSpin = new QSpinBox();
    m_intervalSpin->setRange(100, 60000);
    m_intervalSpin->setValue(1000);
    layout->addWidget(m_intervalSpin, row++, 1);
    // 发送按钮
    QVBoxLayout *buttonLayout = new QVBoxLayout();
    m_sendSingleBtn = new QPushButton("发送单帧");
    m_startContinuousBtn = new QPushButton("开始连续发送");
    m_stopContinuousBtn = new QPushButton("停止连续发送");
    buttonLayout->addWidget(m_sendSingleBtn);
    buttonLayout->addWidget(m_startContinuousBtn);
    buttonLayout->addWidget(m_stopContinuousBtn);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout, row++, 0, 1, 2);
    return groupBox;
}

/**
 * @brief 创建日志显示部分界面
 * @return 日志显示部件指针
 */
QWidget* FaultAlarmWidget::createLogWidget()
{
    QGroupBox *groupBox = new QGroupBox("实时日志");
    QVBoxLayout *layout = new QVBoxLayout(groupBox);
    // 日志显示区域
    m_logTextEdit = new QTextEdit();
    m_logTextEdit->setReadOnly(true);
//    m_logTextEdit->setMaximumBlockCount(1000); // 限制最大行数
    layout->addWidget(m_logTextEdit);
    // 清空日志按钮
    m_clearLogBtn = new QPushButton("清空日志");
    layout->addWidget(m_clearLogBtn);
    return groupBox;
}

/**
 * @brief 初始化信号槽连接
 */
void FaultAlarmWidget::initConnections()
{
    // 发送相关连接
    connect(m_sendSingleBtn, &QPushButton::clicked, this, &FaultAlarmWidget::onSendSingleFrame);
    connect(m_startContinuousBtn, &QPushButton::clicked, this, &FaultAlarmWidget::onStartContinuousSend);
    connect(m_stopContinuousBtn, &QPushButton::clicked, this, &FaultAlarmWidget::onStopContinuousSend);
    connect(m_clearLogBtn, &QPushButton::clicked, this, &FaultAlarmWidget::onClearLog);
    // 定时器连接
    connect(m_sendTimer, &QTimer::timeout, this, &FaultAlarmWidget::onSendTimer);
    connect(m_saveTimer, &QTimer::timeout, this, &FaultAlarmWidget::onSaveConfigurationDelayed);
    // 数据变化连接
    connect(m_alarmTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FaultAlarmWidget::onAlarmTypeChanged);
    // 唯一标识码分段输入连接
    connect(m_orgCodeEdit, &QLineEdit::textChanged, this, &FaultAlarmWidget::onUniqueIdSegmentChanged);
    connect(m_productDateEdit, &QLineEdit::textChanged, this, &FaultAlarmWidget::onUniqueIdSegmentChanged);
    connect(m_serialCodeEdit, &QLineEdit::textChanged, this, &FaultAlarmWidget::onUniqueIdSegmentChanged);
    connect(m_checkCodeEdit, &QLineEdit::textChanged, this, &FaultAlarmWidget::onUniqueIdSegmentChanged);
    connect(m_uniqueIdResultEdit, &QLineEdit::textChanged, this, &FaultAlarmWidget::onUniqueIdResultChanged);
    // 实时保存配置连接 - 数据配置控件（使用防抖机制）
    connect(m_controlCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]()
    {
        m_saveTimer->start();
    });
    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]()
    {
        m_saveTimer->start();
    });
    connect(m_alarmTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]()
    {
        m_saveTimer->start();
    });
    connect(m_faultCodeSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this]()
    {
        m_saveTimer->start();
    });
    connect(m_isolationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]()
    {
        m_saveTimer->start();
    });
    connect(m_faultLevelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]()
    {
        m_saveTimer->start();
    });
    connect(m_warningValueSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this]()
    {
        m_saveTimer->start();
    });
    connect(m_warningThresholdSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this]()
    {
        m_saveTimer->start();
    });
    connect(m_topicNumberSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this]()
    {
        m_saveTimer->start();
    });
    connect(m_sourceDeviceSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this]()
    {
        m_saveTimer->start();
    });
    connect(m_destDeviceSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this]()
    {
        m_saveTimer->start();
    });
    // 实时保存配置连接 - 唯一标识码控件（使用防抖机制）
    connect(m_orgCodeEdit, &QLineEdit::textChanged,
            this, [this]()
    {
        m_saveTimer->start();
    });
    connect(m_productDateEdit, &QLineEdit::textChanged,
            this, [this]()
    {
        m_saveTimer->start();
    });
    connect(m_serialCodeEdit, &QLineEdit::textChanged,
            this, [this]()
    {
        m_saveTimer->start();
    });
    connect(m_uniqueIdResultEdit, &QLineEdit::textChanged,
            this, [this]()
    {
        m_saveTimer->start();
    });
    // 实时保存配置连接 - 发送配置控件（使用防抖机制）
    connect(m_targetIpEdit, &QLineEdit::textChanged,
            this, [this]()
    {
        m_saveTimer->start();
    });
    connect(m_targetPortSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this]()
    {
        m_saveTimer->start();
    });
    connect(m_intervalSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this]()
    {
        m_saveTimer->start();
    });
}

/**
 * @brief 加载样式表
 */
void FaultAlarmWidget::loadStyleSheet()
{
    QFile file(":/styles/styles/drak_theme.qss");
    if(file.open(QFile::ReadOnly))
    {
        QString styleSheet = QLatin1String(file.readAll());
        setStyleSheet(styleSheet);
    }
}

/**
 * @brief 发送单帧数据
 */
void FaultAlarmWidget::onSendSingleFrame()
{
    updateTimestamp();
    QByteArray frame = buildDataFrame();
    QHostAddress targetAddr(m_targetIpEdit->text());
    quint16 targetPort = m_targetPortSpin->value();
    qint64 bytesSent = m_udpSocket->writeDatagram(frame, targetAddr, targetPort);
    if(bytesSent == frame.size())
    {
        appendLog("INFO", QString("成功发送数据帧到 %1:%2, 大小: %3 字节")
                  .arg(m_targetIpEdit->text())
                  .arg(targetPort)
                  .arg(frame.size()));
        appendLog("DEBUG", QString("数据帧内容: %1").arg(QString(frame.toHex(' ').toUpper())));
    }
    else
    {
        appendLog("ERROR", QString("发送数据帧失败: %1").arg(m_udpSocket->errorString()));
    }
}

/**
 * @brief 开始连续发送
 */
void FaultAlarmWidget::onStartContinuousSend()
{
    if(!m_isContinuousSending)
    {
        m_isContinuousSending = true;
        m_sendTimer->start(m_intervalSpin->value());
        updateSendButtonStates();
        appendLog("INFO", QString("开始连续发送，间隔: %1ms").arg(m_intervalSpin->value()));
    }
}

/**
 * @brief 停止连续发送
 */
void FaultAlarmWidget::onStopContinuousSend()
{
    if(m_isContinuousSending)
    {
        m_isContinuousSending = false;
        m_sendTimer->stop();
        updateSendButtonStates();
        appendLog("INFO", "停止连续发送");
    }
}

/**
 * @brief 清空日志
 */
void FaultAlarmWidget::onClearLog()
{
    m_logTextEdit->clear();
    appendLog("INFO", "日志已清空");
}

/**
 * @brief 告警类型改变处理
 */
void FaultAlarmWidget::onAlarmTypeChanged()
{
    int alarmType = m_alarmTypeCombo->currentData().toInt();
    // 根据告警类型启用/禁用相关控件
    if(alarmType == 0x01)    // 故障码
    {
        m_warningValueSpin->setEnabled(false);
        m_warningThresholdSpin->setEnabled(false);
        m_isolationCombo->setEnabled(true);
        m_faultLevelCombo->setEnabled(true);
        // 设置默认值
        m_warningValueSpin->setValue(0.0);
        m_warningThresholdSpin->setValue(0.0);
    }
    else     // 预警码
    {
        m_warningValueSpin->setEnabled(true);
        m_warningThresholdSpin->setEnabled(true);
        m_isolationCombo->setEnabled(false);
        m_faultLevelCombo->setEnabled(false);
        // 设置默认值
        m_isolationCombo->setCurrentIndex(0); // 填0
        m_faultLevelCombo->setCurrentIndex(0); // 填0
    }
}

/**
 * @brief 生成唯一标识码
 */
void FaultAlarmWidget::onGenerateUniqueId()
{
    // 生成组织机构代码 (9位) - 示例代码
    QString orgCode = "633772342";
    m_orgCodeEdit->setText(orgCode);
    // 生成生产日期 (8位) - 当前日期
    QString productDate = QDate::currentDate().toString("yyyyMMdd");
    m_productDateEdit->setText(productDate);
    // 生成序列码 (6位) - 包含固定码S和随机序号
    QString serialCode = QString("SG%1").arg(QRandomGenerator::global()->bounded(1000, 9999));
    m_serialCodeEdit->setText(serialCode);
    // 计算校验码
    onCalculateCheckCode();
    // 更新最终结果
    onUniqueIdSegmentChanged();
    QString uniqueId = m_uniqueIdResultEdit->text();
    appendLog("INFO", QString("生成唯一标识码: %1").arg(uniqueId));
}

/**
 * @brief 唯一标识码分段输入变化处理
 */
void FaultAlarmWidget::onUniqueIdSegmentChanged()
{
    // 组合各个分段生成完整的唯一标识码
    QString orgCode = m_orgCodeEdit->text().leftJustified(9, '0', true).left(9);
    QString productDate = m_productDateEdit->text().leftJustified(8, '0', true).left(8);
    QString serialCode = m_serialCodeEdit->text().leftJustified(6, '0', true).left(6);
    QString checkCode = m_checkCodeEdit->text().leftJustified(1, '0', true).left(1);
    QString fullUniqueId = orgCode + productDate + serialCode + checkCode;
    // 阻止递归调用
    m_uniqueIdResultEdit->blockSignals(true);
    m_uniqueIdResultEdit->setText(fullUniqueId);
    m_uniqueIdResultEdit->blockSignals(false);
}

/**
 * @brief 计算校验码
 */
void FaultAlarmWidget::onCalculateCheckCode()
{
    // 获取前23位
    QString orgCode = m_orgCodeEdit->text().leftJustified(9, '0', true).left(9);
    QString productDate = m_productDateEdit->text().leftJustified(8, '0', true).left(8);
    QString serialCode = m_serialCodeEdit->text().leftJustified(6, '0', true).left(6);
    QString first23 = orgCode + productDate + serialCode;
    // 计算校验码
    int checkSum = 0;
    for(const QChar &ch : first23)
    {
        if(ch.isDigit())
        {
            checkSum += ch.digitValue();
        }
        else
        {
            checkSum += ch.toLatin1(); // ASCII值
        }
    }
    QString checkCode = QString::number(checkSum % 10);
    // 更新校验位
    m_checkCodeEdit->blockSignals(true);
    m_checkCodeEdit->setText(checkCode);
    m_checkCodeEdit->blockSignals(false);
    // 更新完整结果
    onUniqueIdSegmentChanged();
}

/**
 * @brief 唯一标识码结果编辑变化处理
 */
void FaultAlarmWidget::onUniqueIdResultChanged()
{
    // 当用户直接编辑结果时，尝试解析并更新各个分段
    QString fullId = m_uniqueIdResultEdit->text();
    if(fullId.length() >= 24)
    {
        // 阻止递归调用
        m_orgCodeEdit->blockSignals(true);
        m_productDateEdit->blockSignals(true);
        m_serialCodeEdit->blockSignals(true);
        m_checkCodeEdit->blockSignals(true);
        // 解析各个分段
        m_orgCodeEdit->setText(fullId.mid(0, 9));
        m_productDateEdit->setText(fullId.mid(9, 8));
        m_serialCodeEdit->setText(fullId.mid(17, 6));
        m_checkCodeEdit->setText(fullId.mid(23, 1));
        // 恢复信号
        m_orgCodeEdit->blockSignals(false);
        m_productDateEdit->blockSignals(false);
        m_serialCodeEdit->blockSignals(false);
        m_checkCodeEdit->blockSignals(false);
    }
}

/**
 * @brief 加载预设配置
 */
void FaultAlarmWidget::onLoadPreset()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                       "加载预设配置",
                       QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                       "JSON文件 (*.json)");
    if(!fileName.isEmpty())
    {
        QFile file(fileName);
        if(file.open(QIODevice::ReadOnly))
        {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            jsonToData(doc.object());
            updateUI();
            appendLog("INFO", QString("加载预设配置: %1").arg(fileName));
        }
        else
        {
            appendLog("ERROR", QString("无法打开文件: %1").arg(fileName));
        }
    }
}

/**
 * @brief 保存预设配置
 */
void FaultAlarmWidget::onSavePreset()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                       "保存预设配置",
                       QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                       "JSON文件 (*.json)");
    if(!fileName.isEmpty())
    {
        QFile file(fileName);
        if(file.open(QIODevice::WriteOnly))
        {
            QJsonDocument doc(dataToJson());
            file.write(doc.toJson());
            appendLog("INFO", QString("保存预设配置: %1").arg(fileName));
        }
        else
        {
            appendLog("ERROR", QString("无法保存文件: %1").arg(fileName));
        }
    }
}

/**
 * @brief 重置数据
 */
void FaultAlarmWidget::onResetData()
{
    m_controlCombo->setCurrentIndex(1); // 默认选择1
    m_formatCombo->setCurrentIndex(0);  // 默认选择十进制
    m_alarmTypeCombo->setCurrentIndex(0);
    m_faultCodeSpin->setValue(1001);
    m_isolationCombo->setCurrentIndex(0);
    m_faultLevelCombo->setCurrentIndex(0);
    m_warningValueSpin->setValue(100.0);
    m_warningThresholdSpin->setValue(150.0);
    m_topicNumberSpin->setValue(1000);
    m_sourceDeviceSpin->setValue(1);
    m_destDeviceSpin->setValue(2);
    // 重置唯一标识码分段输入
    m_orgCodeEdit->setText("633772342");
    m_productDateEdit->setText(QDate::currentDate().toString("yyyyMMdd"));
    m_serialCodeEdit->setText("SG1234");
    m_checkCodeEdit->clear();
    m_uniqueIdResultEdit->clear();
    // 计算校验码并更新结果
    onCalculateCheckCode();
    // 重置数值格式为十进制
    onFormatChanged();
    appendLog("INFO", "数据已重置为默认值");
}

/**
 * @brief 数值格式改变处理
 */
void FaultAlarmWidget::onFormatChanged()
{
    int format = m_formatCombo->currentData().toInt();
    int base = (format == 0) ? 10 : 16; // 0=十进制, 1=十六进制
    // 更新所有数值输入控件的显示格式
    m_faultCodeSpin->setDisplayIntegerBase(base);
    m_topicNumberSpin->setDisplayIntegerBase(base);
    m_sourceDeviceSpin->setDisplayIntegerBase(base);
    m_destDeviceSpin->setDisplayIntegerBase(base);
    // 更新前缀显示
    QString prefix = (format == 0) ? "" : "0x";
    m_faultCodeSpin->setPrefix(prefix);
    m_topicNumberSpin->setPrefix(prefix);
    m_sourceDeviceSpin->setPrefix(prefix);
    m_destDeviceSpin->setPrefix(prefix);
    QString formatName = (format == 0) ? "十进制" : "十六进制";
    appendLog("INFO", QString("数值显示格式已切换为: %1").arg(formatName));
}

/**
 * @brief 定时发送处理
 */
void FaultAlarmWidget::onSendTimer()
{
    onSendSingleFrame();
}

/**
 * @brief 构建数据帧
 * @return 完整的数据帧字节数组
 */
QByteArray FaultAlarmWidget::buildDataFrame()
{
    QByteArray frame;
    // 更新当前数据
    m_currentData.control = m_controlCombo->currentData().toInt();
    m_currentData.topicNumber = m_topicNumberSpin->value();
    m_currentData.sourceDevice = m_sourceDeviceSpin->value();
    m_currentData.destDevice = m_destDeviceSpin->value();
    m_currentData.alarmIdentifier = m_alarmTypeCombo->currentData().toInt();
    m_currentData.faultCode = m_faultCodeSpin->value();
    m_currentData.isolationFlag = m_isolationCombo->currentData().toInt();
    m_currentData.faultLevel = m_faultLevelCombo->currentData().toInt();
    m_currentData.warningValue = m_warningValueSpin->value();
    m_currentData.warningThreshold = m_warningThresholdSpin->value();
    m_currentData.uniqueId = m_uniqueIdResultEdit->text().leftJustified(24, '\0', true);
    // 数据域长度固定为37字节 (1+2+1+1+4+4+24)
    m_currentData.dataLength = 37;
    // 构建帧头 (11字节)
    frame.append(m_currentData.control);
    // 数据域长度 (先低后高)
    frame.append(static_cast<char>(m_currentData.dataLength & 0xFF));
    frame.append(static_cast<char>((m_currentData.dataLength >> 8) & 0x07)); // 高3位
    // 主题号 (先低后高)
    frame.append(static_cast<char>(m_currentData.topicNumber & 0xFF));
    frame.append(static_cast<char>((m_currentData.topicNumber >> 8) & 0xFF));
    // 源设备号和目的设备号
    frame.append(m_currentData.sourceDevice);
    frame.append(m_currentData.destDevice);
    // 时间戳 (4字节，先低后高)
    frame.append(static_cast<char>(m_currentData.timestamp & 0xFF));
    frame.append(static_cast<char>((m_currentData.timestamp >> 8) & 0xFF));
    frame.append(static_cast<char>((m_currentData.timestamp >> 16) & 0xFF));
    frame.append(static_cast<char>((m_currentData.timestamp >> 24) & 0xFF));
    // 构建数据域 (37字节)
    frame.append(m_currentData.alarmIdentifier);
    // 故障码/预警码 (先低后高)
    frame.append(static_cast<char>(m_currentData.faultCode & 0xFF));
    frame.append(static_cast<char>((m_currentData.faultCode >> 8) & 0xFF));
    frame.append(m_currentData.isolationFlag);
    frame.append(m_currentData.faultLevel);
    // 预警数值 (4字节浮点数)
    union
    {
        float f;
        quint32 i;
    } warningValueUnion;
    warningValueUnion.f = m_currentData.warningValue;
    frame.append(static_cast<char>(warningValueUnion.i & 0xFF));
    frame.append(static_cast<char>((warningValueUnion.i >> 8) & 0xFF));
    frame.append(static_cast<char>((warningValueUnion.i >> 16) & 0xFF));
    frame.append(static_cast<char>((warningValueUnion.i >> 24) & 0xFF));
    // 预警阈值 (4字节浮点数)
    union
    {
        float f;
        quint32 i;
    } warningThresholdUnion;
    warningThresholdUnion.f = m_currentData.warningThreshold;
    frame.append(static_cast<char>(warningThresholdUnion.i & 0xFF));
    frame.append(static_cast<char>((warningThresholdUnion.i >> 8) & 0xFF));
    frame.append(static_cast<char>((warningThresholdUnion.i >> 16) & 0xFF));
    frame.append(static_cast<char>((warningThresholdUnion.i >> 24) & 0xFF));
    // 唯一标识码 (24字节)
    QByteArray uniqueIdBytes = m_currentData.uniqueId.toUtf8().leftJustified(24, '\0', true);
    frame.append(uniqueIdBytes);
    // 计算并添加校验和 (2字节)
    quint16 checksum = calculateChecksum(frame);
    frame.append(static_cast<char>(checksum & 0xFF));
    frame.append(static_cast<char>((checksum >> 8) & 0xFF));
    return frame;
}

/**
 * @brief 计算校验和
 * @param data 数据字节数组
 * @return 16位校验和
 */
quint16 FaultAlarmWidget::calculateChecksum(const QByteArray &data)
{
    quint32 sum = 0;
    for(int i = 0; i < data.size(); ++i)
    {
        sum += static_cast<quint8>(data[i]);
    }
    // 高16位与低16位异或
    quint16 checksum = (sum >> 16) ^ (sum & 0xFFFF);
    return checksum;
}

/**
 * @brief 更新时间戳
 */
void FaultAlarmWidget::updateTimestamp()
{
    QDateTime now = QDateTime::currentDateTime();
    QTime time = now.time();
    // 计算从00:00:00开始的毫秒数
    m_currentData.timestamp = time.msecsSinceStartOfDay();
}

/**
 * @brief 生成唯一标识码
 * @return 24位唯一标识码字符串
 */
QString FaultAlarmWidget::generateUniqueId()
{
    // 组织机构代码 (9位) - 示例代码
    QString orgCode = "633772342";
    // 生产日期 (8位) - 当前日期
    QString productDate = QDate::currentDate().toString("yyyyMMdd");
    // 序列码 (6位) - 包含固定码S和随机序号
    QString serialCode = QString("SG%1").arg(QRandomGenerator::global()->bounded(1000, 9999));
    // 前23位
    QString first23 = orgCode + productDate + serialCode;
    // 计算校验码
    int checkSum = 0;
    for(const QChar &ch : first23)
    {
        if(ch.isDigit())
        {
            checkSum += ch.digitValue();
        }
        else
        {
            checkSum += ch.toLatin1(); // ASCII值
        }
    }
    QString checkCode = QString::number(checkSum % 10);
    return first23 + checkCode;
}

/**
 * @brief 更新界面显示
 */
void FaultAlarmWidget::updateUI()
{
    // 根据当前数据更新界面控件
    onAlarmTypeChanged();
}

/**
 * @brief 更新发送按钮状态
 */
void FaultAlarmWidget::updateSendButtonStates()
{
    m_sendSingleBtn->setEnabled(!m_isContinuousSending);
    m_startContinuousBtn->setEnabled(!m_isContinuousSending);
    m_stopContinuousBtn->setEnabled(m_isContinuousSending);
}

/**
 * @brief 添加日志
 * @param level 日志级别
 * @param message 日志消息
 */
void FaultAlarmWidget::appendLog(const QString &level, const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString logEntry = QString("[%1] [%2] %3").arg(timestamp, level, message);
    // 设置颜色
    QTextCharFormat format;
    if(level == "ERROR")
    {
        format.setForeground(QColor(255, 100, 100));
    }
    else if(level == "WARN")
    {
        format.setForeground(QColor(255, 200, 100));
    }
    else if(level == "INFO")
    {
        format.setForeground(QColor(100, 255, 100));
    }
    else
    {
        format.setForeground(QColor(100, 200, 255));
    }
    QTextCursor cursor = m_logTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(logEntry + "\n", format);
    // 自动滚动到底部
    m_logTextEdit->ensureCursorVisible();
}

QString FaultAlarmWidget::getConfigPath()
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
        appendLog("WARN", QString("无法访问应用程序目录的上一级目录: %1").arg(appDir));
        return QString();
    }
    //构建路径（跨平台兼容）
    QString configPath = QDir::cleanPath(parentDir.absolutePath() + QDir::separator() + "config" + QDir::separator() + "FaultAlarmWidget_config.json");
    //确保目录存在
    QFileInfo configInfo(configPath);
    QDir configDir = configInfo.absoluteDir();
    if(!configDir.exists())
    {
        if(!configDir.mkpath("."))
        {
            appendLog("WARN", QString("无法创建配置目录: %1").arg(configDir.absolutePath()));
            return QString();
        }
        appendLog("DEBUG", QString("已创建配置目录: %1").arg(configDir.absolutePath()));
    }
    return configPath;
}

/**
 * @brief 保存配置（静默保存，不输出日志）
 */
void FaultAlarmWidget::saveConfiguration()
{
    QString configFilePath = getConfigPath();
    // 将当前配置转换为JSON
    QJsonObject configJson = dataToJson();
    QJsonDocument doc(configJson);
    // 保存到文件（静默保存）
    QFile file(configFilePath);
    if(file.open(QIODevice::WriteOnly))
    {
        file.write(doc.toJson());
        file.close();
    }
}

/**
 * @brief 延迟保存配置（带日志输出）
 */
void FaultAlarmWidget::onSaveConfigurationDelayed()
{
    QString configFilePath = getConfigPath();
    // 将当前配置转换为JSON
    QJsonObject configJson = dataToJson();
    QJsonDocument doc(configJson);
    // 保存到文件
    QFile file(configFilePath);
    if(file.open(QIODevice::WriteOnly))
    {
        file.write(doc.toJson());
        file.close();
        appendLog("INFO", QString("配置已自动保存到: %1").arg(configFilePath));
    }
    else
    {
        appendLog("ERROR", QString("无法保存配置文件: %1").arg(configFilePath));
    }
}

/**
 * @brief 加载配置
 */
void FaultAlarmWidget::loadConfiguration()
{
    QString configFilePath = getConfigPath();
    QFile file(configFilePath);
    if(!file.exists())
    {
        appendLog("INFO", "配置文件不存在，使用默认配置");
        return;
    }
    if(file.open(QIODevice::ReadOnly))
    {
        QByteArray data = file.readAll();
        file.close();
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        if(error.error != QJsonParseError::NoError)
        {
            appendLog("ERROR", QString("配置文件解析错误: %1").arg(error.errorString()));
            return;
        }
        if(doc.isObject())
        {
            jsonToData(doc.object());
            appendLog("INFO", QString("配置已从文件加载: %1").arg(configFilePath));
        }
    }
    else
    {
        appendLog("ERROR", QString("无法读取配置文件: %1").arg(configFilePath));
    }
}

/**
 * @brief 数据转JSON
 * @return JSON对象
 */
QJsonObject FaultAlarmWidget::dataToJson() const
{
    QJsonObject json;
    json["alarmType"] = m_alarmTypeCombo->currentIndex();
    json["faultCode"] = m_faultCodeSpin->value();
    json["isolationFlag"] = m_isolationCombo->currentIndex();
    json["faultLevel"] = m_faultLevelCombo->currentIndex();
    json["warningValue"] = m_warningValueSpin->value();
    json["warningThreshold"] = m_warningThresholdSpin->value();
    json["topicNumber"] = m_topicNumberSpin->value();
    json["sourceDevice"] = m_sourceDeviceSpin->value();
    json["destDevice"] = m_destDeviceSpin->value();
    // 保存唯一标识码分段信息
    json["orgCode"] = m_orgCodeEdit->text();
    json["productDate"] = m_productDateEdit->text();
    json["serialCode"] = m_serialCodeEdit->text();
    json["checkCode"] = m_checkCodeEdit->text();
    json["uniqueIdResult"] = m_uniqueIdResultEdit->text();
    json["targetIp"] = m_targetIpEdit->text();
    json["targetPort"] = m_targetPortSpin->value();
    json["interval"] = m_intervalSpin->value();
    return json;
}

/**
 * @brief JSON转数据
 * @param json JSON对象
 */
void FaultAlarmWidget::jsonToData(const QJsonObject &json)
{
    m_alarmTypeCombo->setCurrentIndex(json["alarmType"].toInt());
    m_faultCodeSpin->setValue(json["faultCode"].toInt());
    m_isolationCombo->setCurrentIndex(json["isolationFlag"].toInt());
    m_faultLevelCombo->setCurrentIndex(json["faultLevel"].toInt());
    m_warningValueSpin->setValue(json["warningValue"].toDouble());
    m_warningThresholdSpin->setValue(json["warningThreshold"].toDouble());
    m_topicNumberSpin->setValue(json["topicNumber"].toInt());
    m_sourceDeviceSpin->setValue(json["sourceDevice"].toInt());
    m_destDeviceSpin->setValue(json["destDevice"].toInt());
    // 加载唯一标识码分段信息
    if(json.contains("orgCode"))
    {
        m_orgCodeEdit->setText(json["orgCode"].toString());
        m_productDateEdit->setText(json["productDate"].toString());
        m_serialCodeEdit->setText(json["serialCode"].toString());
        m_checkCodeEdit->setText(json["checkCode"].toString());
        m_uniqueIdResultEdit->setText(json["uniqueIdResult"].toString());
    }
    else
    {
        // 兼容旧版本配置
        QString oldUniqueId = json["uniqueId"].toString();
        if(!oldUniqueId.isEmpty() && oldUniqueId.length() >= 24)
        {
            m_orgCodeEdit->setText(oldUniqueId.mid(0, 9));
            m_productDateEdit->setText(oldUniqueId.mid(9, 8));
            m_serialCodeEdit->setText(oldUniqueId.mid(17, 6));
            m_checkCodeEdit->setText(oldUniqueId.mid(23, 1));
            m_uniqueIdResultEdit->setText(oldUniqueId);
        }
    }
    m_targetIpEdit->setText(json["targetIp"].toString());
    m_targetPortSpin->setValue(json["targetPort"].toInt());
    m_intervalSpin->setValue(json["interval"].toInt());
}
