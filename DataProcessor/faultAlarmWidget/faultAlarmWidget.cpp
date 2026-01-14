#include "faultAlarmWidget.h"
#include <QApplication>
#include <QFile>
#include <QJsonArray>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(FaultAlarmLog, "[app.FaultAlarm]")

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
    qCInfo(FaultAlarmLog) << "FaultAlarmWidget 构造开始";
    //配置保存定时器（防抖机制）
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(500);     //500ms延迟
    initUI();
    initConnections();
    loadStyleSheet();
    updateUI();
    updateSendButtonStates();
    loadConfiguration();     //在UI初始化完成后加载配置
    qCInfo(FaultAlarmLog) << "FaultAlarmWidget 构造完成";
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
    setMinimumSize(800, 600);     //减小最小尺寸，适应小屏设备
    //创建主布局 - 使用QGridLayout替代QHBoxLayout，更好地支持响应式布局
    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->setSpacing(5);     //减小间距，节省空间
    mainLayout->setContentsMargins(5, 5, 5, 5);     //减小边距，节省空间
    //创建左侧配置区域
    QScrollArea *configScrollArea = new QScrollArea();
    configScrollArea->setWidgetResizable(true);
    configScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    configScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    configScrollArea->setStyleSheet(
        "QScrollArea { background-color: #2E3440; border: none; }"
        "QScrollArea > QWidget > QWidget { background-color: #2E3440; }"
    );
    QWidget *configWidget = new QWidget();
    configWidget->setStyleSheet("background-color: #2E3440;");
    QVBoxLayout *configLayout = new QVBoxLayout(configWidget);
    configLayout->setSpacing(5);
    configLayout->setContentsMargins(5, 5, 5, 5);
    //创建数据配置部分
    m_dataConfigWidget = createDataConfigWidget();
    configLayout->addWidget(m_dataConfigWidget);
    //创建发送配置部分
    m_sendConfigWidget = createSendConfigWidget();
    configLayout->addWidget(m_sendConfigWidget);
    configScrollArea->setWidget(configWidget);
    //创建右侧日志显示区域
    m_logWidget = createLogWidget();
    //使用QSplitter允许用户调整左右区域的大小
    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal);
    mainSplitter->setStyleSheet(
        "QSplitter::handle { background-color: #4C566A; }"
        "QSplitter::handle:horizontal { width: 4px; }"
        "QSplitter::handle:vertical { height: 4px; }"
    );
    mainSplitter->addWidget(configScrollArea);
    mainSplitter->addWidget(m_logWidget);
    mainSplitter->setStretchFactor(0, 2);
    mainSplitter->setStretchFactor(1, 1);
    //将分割器添加到主布局
    mainLayout->addWidget(mainSplitter, 0, 0);
    mainLayout->setRowStretch(0, 1);
    mainLayout->setColumnStretch(0, 1);
}

/**
 * @brief 创建数据配置部分界面
 * @return 数据配置部件指针
 */
QWidget* FaultAlarmWidget::createDataConfigWidget()
{
    QGroupBox *groupBox = new QGroupBox("数据配置");
    groupBox->setStyleSheet(
        "QGroupBox {background - color: #2E3440;color: #ECEFF4; border: 1px solid #4C566A;border - radius: 4px;margin - top: 8px;font - weight: bold;}"
        "QGroupBox::title{subcontrol - origin:margin;subcontrol - position:top center;padding: 0 5px;background - color:#3B4252;}"
        "QLabel{color:#E5E9F0;}"
    );
    QGridLayout* layout = new QGridLayout(groupBox);
    layout->setSpacing(5);     //减小间距，节省空间
    layout->setContentsMargins(5, 10, 5, 5);     //减小边距，节省空间
    //使用两列布局，更好地利用水平空间
    int leftCol = 0;
    int rightCol = 2;
    int leftRow = 0;
    int rightRow = 0;
    //左侧列 - 基本参数
    //控制位设置
    layout->addWidget(new QLabel("控制位:"), leftRow, leftCol);
    m_controlCombo = new QComboBox();
    m_controlCombo->addItem("0", 0);
    m_controlCombo->addItem("1", 1);
    m_controlCombo->setCurrentIndex(1);     //默认选择1
    m_controlCombo->setStyleSheet(
        "QComboBox { background - color: #3B4252;color: #ECEFF4;border: 1px solid #4C566A;border - radius: 3px;padding: 3px;min - width: 6em;}"
        "QComboBox::drop - down{subcontrol - origin: padding;subcontrol - position: top right;width: 20px;border - left: 1px solid #4C566A;border - top - right - radius: 3px;border - bottom - right - radius: 3px;}"
        "QComboBox::down - arrow{image: url(: / icons / icons / arrow - down.png);}"
        "QComboBox QAbstractItemView{background - color: #3B4252;color: #ECEFF4;border: 1px solid #4C566A;selection - background - color: #5E81AC;selection - color: #ECEFF4;}"
    );
    layout->addWidget(m_controlCombo, leftRow++, leftCol + 1);
    //数值输入格式选择
    layout->addWidget(new QLabel("数值格式:"), leftRow, leftCol);
    m_formatCombo = new QComboBox();
    m_formatCombo->addItem("十进制", 0);
    m_formatCombo->addItem("十六进制", 1);
    m_formatCombo->setStyleSheet(
        "QComboBox {background - color: #3B4252;color: #ECEFF4;border: 1px solid #4C566A;border - radius: 3px;padding: 3px;min - width: 6em;}"
        "QComboBox::drop - down{subcontrol - origin: padding;subcontrol - position: top right; width: 20px;border - left: 1px solid #4C566A;border - top - right - radius: 3px;border - bottom - right - radius: 3px;}"
        "QComboBox::down - arrow{image: url(: / icons / icons / arrow - down.png);}"
        "QComboBox QAbstractItemView{background - color: #3B4252;color: #ECEFF4; border: 1px solid #4C566A;selection - background - color: #5E81AC;selection - color: #ECEFF4;}"
    );
    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FaultAlarmWidget::onFormatChanged);
    layout->addWidget(m_formatCombo, leftRow++, leftCol + 1);
    //告警类型选择
    layout->addWidget(new QLabel("告警类型:"), leftRow, leftCol);
    m_alarmTypeCombo = new QComboBox();
    m_alarmTypeCombo->addItem("故障码", 0x01);
    m_alarmTypeCombo->addItem("预警码", 0x02);
    m_alarmTypeCombo->setStyleSheet(
        "QComboBox { background - color: #3B4252;color: #ECEFF4;border: 1px solid #4C566A;border - radius: 3px;padding: 3px;min - width: 6em;}"
        "QComboBox::drop - down{subcontrol - origin: padding;subcontrol - position: top right;width: 20px;border - left: 1px solid #4C566A;border - top - right - radius: 3px;border - bottom - right - radius: 3px;}"
        "QComboBox::down - arrow{image: url(: / icons / icons / arrow - down.png);}"
        "QComboBox QAbstractItemView{background - color: #3B4252;color: #ECEFF4;border: 1px solid #4C566A;selection - background - color: #5E81AC;selection - color: #ECEFF4;}"
    );
    layout->addWidget(m_alarmTypeCombo, leftRow++, leftCol + 1);
    //故障码/预警码
    layout->addWidget(new QLabel("故障码/预警码:"), leftRow, leftCol);
    m_faultCodeSpin = new QSpinBox();
    m_faultCodeSpin->setRange(0, 65535);
    m_faultCodeSpin->setValue(1001);
    m_faultCodeSpin->setDisplayIntegerBase(10);     //默认十进制显示
    m_faultCodeSpin->setStyleSheet(
        "QSpinBox {background - color: #3B4252;color: #ECEFF4;border: 1px solid #4C566A;border - radius: 3px;padding: 3px;selection - background - color: #5E81AC;}"
        "QSpinBox::up - button, QSpinBox::down - button{background - color: #4C566A;width: 16px;border: 1px solid #5E81AC;}"
        "QSpinBox::up - button: hover, QSpinBox::down - button: hover{background - color: #5E81AC;}"
        "QSpinBox::up - button: pressed, QSpinBox::down - button: pressed{background - color: #81A1C1;}"
    );
    layout->addWidget(m_faultCodeSpin, leftRow++, leftCol + 1);
    //隔离标志
    layout->addWidget(new QLabel("隔离标志:"), leftRow, leftCol);
    m_isolationCombo = new QComboBox();
    m_isolationCombo->addItem("预警码(填0)", 0x00);
    m_isolationCombo->addItem("可能与外系统有关", 0x01);
    m_isolationCombo->addItem("系统内部故障", 0x02);
    m_isolationCombo->setStyleSheet(
        "QComboBox {background - color: #3B4252;color: #ECEFF4;border: 1px solid #4C566A;border - radius: 3px;padding: 3px;min - width: 6em;}"
        "QComboBox::drop - down{subcontrol - origin: padding;subcontrol - position: top right;width: 20px;border - left: 1px solid #4C566A;border - top - right - radius: 3px; border - bottom - right - radius: 3px;}"
        "QComboBox::down - arrow{image: url(: / icons / icons / arrow - down.png);}"
        "QComboBox QAbstractItemView{background - color: #3B4252;color: #ECEFF4;border: 1px solid #4C566A;selection - background - color: #5E81AC;selection - color: #ECEFF4;}"
    );
    layout->addWidget(m_isolationCombo, leftRow++, leftCol + 1);
    //故障等级
    layout->addWidget(new QLabel("故障等级:"), leftRow, leftCol);
    m_faultLevelCombo = new QComboBox();
    m_faultLevelCombo->addItem("预警码(填0)", 0x00);
    m_faultLevelCombo->addItem("1类故障", 0x01);
    m_faultLevelCombo->addItem("2类故障", 0x02);
    m_faultLevelCombo->addItem("未知故障", 0x03);
    m_faultLevelCombo->setStyleSheet("QComboBox { background-color: #3B4252; color: #ECEFF4; border: 1px solid #4C566A; border-radius: 3px; padding: 3px; min-width: 6em; }QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 20px; border-left: 1px solid #4C566A; border-top-right-radius: 3px; border-bottom-right-radius: 3px; }QComboBox::down-arrow { image: url(:/icons/icons/arrow-down.png); }QComboBox QAbstractItemView { background-color: #3B4252; color: #ECEFF4; border: 1px solid #4C566A; selection-background-color: #5E81AC; selection-color: #ECEFF4; }"
                                    );
    layout->addWidget(m_faultLevelCombo, leftRow++, leftCol + 1);
    //右侧列 - 数值和设备参数
    //预警数值
    layout->addWidget(new QLabel("预警数值:"), rightRow, rightCol);
    m_warningValueSpin = new QDoubleSpinBox();
    m_warningValueSpin->setRange(0, 65535);
    m_warningValueSpin->setDecimals(2);
    m_warningValueSpin->setValue(100.0);
    m_warningValueSpin->setStyleSheet("QDoubleSpinBox { background-color: #3B4252; color: #ECEFF4; border: 1px solid #4C566A; border-radius: 3px; padding: 3px; selection-background-color: #5E81AC; }QDoubleSpinBox::up-button, QDoubleSpinBox::down-button { background-color: #4C566A; width: 16px; border: 1px solid #5E81AC; }QDoubleSpinBox::up-button:hover, QDoubleSpinBox::down-button:hover { background-color: #5E81AC; }QDoubleSpinBox::up-button:pressed, QDoubleSpinBox::down-button:pressed { background-color: #81A1C1; }"
                                     );
    layout->addWidget(m_warningValueSpin, rightRow++, rightCol + 1);
    //预警阈值
    layout->addWidget(new QLabel("预警阈值:"), rightRow, rightCol);
    m_warningThresholdSpin = new QDoubleSpinBox();
    m_warningThresholdSpin->setRange(0, 65535);
    m_warningThresholdSpin->setDecimals(2);
    m_warningThresholdSpin->setValue(150.0);
    m_warningThresholdSpin->setStyleSheet("QDoubleSpinBox { background-color: #3B4252; color: #ECEFF4; border: 1px solid #4C566A; border-radius: 3px; padding: 3px; selection-background-color: #5E81AC; }QDoubleSpinBox::up-button, QDoubleSpinBox::down-button { background-color: #4C566A; width: 16px; border: 1px solid #5E81AC; }QDoubleSpinBox::up-button:hover, QDoubleSpinBox::down-button:hover { background-color: #5E81AC; }QDoubleSpinBox::up-button:pressed, QDoubleSpinBox::down-button:pressed { background-color: #81A1C1; }"
                                         );
    layout->addWidget(m_warningThresholdSpin, rightRow++, rightCol + 1);
    //主题号
    layout->addWidget(new QLabel("主题号:"), rightRow, rightCol);
    m_topicNumberSpin = new QSpinBox();
    m_topicNumberSpin->setRange(0, 65535);
    m_topicNumberSpin->setValue(1000);
    m_topicNumberSpin->setDisplayIntegerBase(10);     //默认十进制显示
    m_topicNumberSpin->setStyleSheet("QSpinBox { background-color: #3B4252; color: #ECEFF4; border: 1px solid #4C566A; border-radius: 3px; padding: 3px; selection-background-color: #5E81AC; }QSpinBox::up-button, QSpinBox::down-button { background-color: #4C566A; width: 16px; border: 1px solid #5E81AC; }QSpinBox::up-button:hover, QSpinBox::down-button:hover { background-color: #5E81AC; }QSpinBox::up-button:pressed, QSpinBox::down-button:pressed { background-color: #81A1C1; }"
                                    );
    layout->addWidget(m_topicNumberSpin, rightRow++, rightCol + 1);
    //源设备号
    layout->addWidget(new QLabel("源设备号:"), rightRow, rightCol);
    m_sourceDeviceSpin = new QSpinBox();
    m_sourceDeviceSpin->setRange(0, 255);
    m_sourceDeviceSpin->setValue(1);
    m_sourceDeviceSpin->setDisplayIntegerBase(10);     //默认十进制显示
    m_sourceDeviceSpin->setStyleSheet("QSpinBox { background-color: #3B4252; color: #ECEFF4; border: 1px solid #4C566A; border-radius: 3px; padding: 3px; selection-background-color: #5E81AC; }QSpinBox::up-button, QSpinBox::down-button { background-color: #4C566A; width: 16px; border: 1px solid #5E81AC; }QSpinBox::up-button:hover, QSpinBox::down-button:hover { background-color: #5E81AC; }QSpinBox::up-button:pressed, QSpinBox::down-button:pressed { background-color: #81A1C1; }"
                                     );
    layout->addWidget(m_sourceDeviceSpin, rightRow++, rightCol + 1);
    //目的设备号
    layout->addWidget(new QLabel("目的设备号:"), rightRow, rightCol);
    m_destDeviceSpin = new QSpinBox();
    m_destDeviceSpin->setRange(0, 255);
    m_destDeviceSpin->setValue(2);
    m_destDeviceSpin->setDisplayIntegerBase(10);     //默认十进制显示
    m_destDeviceSpin->setStyleSheet("QSpinBox { background-color: #3B4252; color: #ECEFF4; border: 1px solid #4C566A; border-radius: 3px; padding: 3px; selection-background-color: #5E81AC; }QSpinBox::up-button, QSpinBox::down-button { background-color: #4C566A; width: 16px; border: 1px solid #5E81AC; }QSpinBox::up-button:hover, QSpinBox::down-button:hover { background-color: #5E81AC; }QSpinBox::up-button:pressed, QSpinBox::down-button:pressed { background-color: #81A1C1; }"
                                   );
    layout->addWidget(m_destDeviceSpin, rightRow++, rightCol + 1);
    //唯一标识码部分 - 使用单独的GroupBox和栅格布局
    QGroupBox *uniqueIdGroupBox = new QGroupBox("唯一标识码");
    uniqueIdGroupBox->setStyleSheet("QGroupBox { background-color: #3B4252; color: #ECEFF4; border: 1px solid #4C566A; border-radius: 4px; margin-top: 8px; font-weight: bold; }QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top center; padding: 0 5px; background-color: #434C5E; }QLabel { color: #E5E9F0; }"
                                   );
    QGridLayout *uniqueIdLayout = new QGridLayout(uniqueIdGroupBox);
    uniqueIdLayout->setSpacing(5);
    uniqueIdLayout->setContentsMargins(5, 10, 5, 5);
    int uidRow = 0;
    //军地组织机构标识符
    uniqueIdLayout->addWidget(new QLabel("军地组织机构标识符(1位):"), uidRow, 0);
    m_orgIdentifierEdit = new QLineEdit();
    m_orgIdentifierEdit->setMaxLength(1);
    m_orgIdentifierEdit->setPlaceholderText("1位标识符");
    m_orgIdentifierEdit->setText("1");     //默认值
    m_orgIdentifierEdit->setStyleSheet("QLineEdit { background-color: #3B4252; color: #ECEFF4; border: 1px solid #4C566A; border-radius: 3px; padding: 3px; selection-background-color: #5E81AC; }");
    uniqueIdLayout->addWidget(m_orgIdentifierEdit, uidRow++, 1);
    //组织机构代码
    uniqueIdLayout->addWidget(new QLabel("组织机构代码(9位):"), uidRow, 0);
    m_orgCodeEdit = new QLineEdit();
    m_orgCodeEdit->setMaxLength(9);
    m_orgCodeEdit->setPlaceholderText("9位组织机构代码");
    m_orgCodeEdit->setText("633772342");     //默认值
    m_orgCodeEdit->setStyleSheet("QLineEdit { background-color: #3B4252; color: #ECEFF4; border: 1px solid #4C566A; border-radius: 3px; padding: 3px; selection-background-color: #5E81AC; }");
    uniqueIdLayout->addWidget(m_orgCodeEdit, uidRow++, 1);
    //生产日期
    uniqueIdLayout->addWidget(new QLabel("生产日期(8位):"), uidRow, 0);
    QHBoxLayout *dateLayout = new QHBoxLayout();
    m_productDateEdit = new QLineEdit();
    m_productDateEdit->setMaxLength(8);
    m_productDateEdit->setPlaceholderText("YYYYMMDD格式");
    m_productDateEdit->setText(QDate::currentDate().toString("yyyyMMdd"));     //默认当前日期
    m_productDateEdit->setStyleSheet("QLineEdit { background-color: #3B4252; color: #ECEFF4; border: 1px solid #4C566A; border-radius: 3px; padding: 3px; selection-background-color: #5E81AC; }");
    QPushButton *todayBtn = new QPushButton("今天");
    todayBtn->setMaximumWidth(50);
    todayBtn->setStyleSheet("QPushButton { background-color: #4C566A; color: #ECEFF4; border: 1px solid #5E81AC; border-radius: 3px; padding: 3px; }QPushButton:hover { background-color: #5E81AC; }QPushButton:pressed { background-color: #81A1C1; }");
    connect(todayBtn, &QPushButton::clicked, [this]()
    {
        m_productDateEdit->setText(QDate::currentDate().toString("yyyyMMdd"));
        onUniqueIdSegmentChanged();
    });
    dateLayout->addWidget(m_productDateEdit);
    dateLayout->addWidget(todayBtn);
    dateLayout->setStretch(0, 1);     //让输入框占据更多空间
    uniqueIdLayout->addLayout(dateLayout, uidRow++, 1);
    
    //序列码分段
    QHBoxLayout *serialPartsLayout = new QHBoxLayout();
    
    //产品大类代码
    m_productCategoryEdit = new QLineEdit();
    m_productCategoryEdit->setMaxLength(2);
    m_productCategoryEdit->setPlaceholderText("产品大类(2位)");
    m_productCategoryEdit->setText("");     //默认值
    m_productCategoryEdit->setToolTip("产品大类代码(2位)");
    m_productCategoryEdit->setStyleSheet("QLineEdit { background-color: #3B4252; color: #ECEFF4; border: 1px solid #4C566A; border-radius: 3px; padding: 3px; selection-background-color: #5E81AC; }");
    serialPartsLayout->addWidget(m_productCategoryEdit);

    //层级码
    m_levelCodeEdit = new QLineEdit();
    m_levelCodeEdit->setMaxLength(2);
    m_levelCodeEdit->setPlaceholderText("层级(2位)");
    m_levelCodeEdit->setText("");     //默认值
    m_levelCodeEdit->setToolTip("层级码(2位)");
    m_levelCodeEdit->setStyleSheet("QLineEdit { background-color: #3B4252; color: #ECEFF4; border: 1px solid #4C566A; border-radius: 3px; padding: 3px; selection-background-color: #5E81AC; }");
    serialPartsLayout->addWidget(m_levelCodeEdit);

    //固定码S
    m_fixedCodeSEdit = new QLineEdit();
    m_fixedCodeSEdit->setMaxLength(1);
    m_fixedCodeSEdit->setPlaceholderText("固定码(1位)");
    m_fixedCodeSEdit->setText("S");     //默认值
    m_fixedCodeSEdit->setToolTip("固定码S(1位)");
    m_fixedCodeSEdit->setStyleSheet("QLineEdit { background-color: #3B4252; color: #ECEFF4; border: 1px solid #4C566A; border-radius: 3px; padding: 3px; selection-background-color: #5E81AC; }");
    serialPartsLayout->addWidget(m_fixedCodeSEdit);

    //分系统码
    m_subsystemCodeEdit = new QLineEdit();
    m_subsystemCodeEdit->setMaxLength(1);
    m_subsystemCodeEdit->setPlaceholderText("分系统(1位)");
    m_subsystemCodeEdit->setText("G");     //默认值
    m_subsystemCodeEdit->setToolTip("分系统码(1位)");
    m_subsystemCodeEdit->setStyleSheet("QLineEdit { background-color: #3B4252; color: #ECEFF4; border: 1px solid #4C566A; border-radius: 3px; padding: 3px; selection-background-color: #5E81AC; }");
    serialPartsLayout->addWidget(m_subsystemCodeEdit);

    //设备码
    m_deviceCodeEdit = new QLineEdit();
    m_deviceCodeEdit->setMaxLength(2);
    m_deviceCodeEdit->setPlaceholderText("设备(2位)");
    m_deviceCodeEdit->setText("");     //默认值
    m_deviceCodeEdit->setToolTip("设备码(2位)");
    m_deviceCodeEdit->setStyleSheet("QLineEdit { background-color: #3B4252; color: #ECEFF4; border: 1px solid #4C566A; border-radius: 3px; padding: 3px; selection-background-color: #5E81AC; }");
    serialPartsLayout->addWidget(m_deviceCodeEdit);

    //序号码
    m_sequenceNumberEdit = new QLineEdit();
    m_sequenceNumberEdit->setMaxLength(2);
    m_sequenceNumberEdit->setPlaceholderText("序号(2位)");
    m_sequenceNumberEdit->setText("");     //默认值
    m_sequenceNumberEdit->setToolTip("序号码(2位)");
    m_sequenceNumberEdit->setStyleSheet("QLineEdit { background-color: #3B4252; color: #ECEFF4; border: 1px solid #4C566A; border-radius: 3px; padding: 3px; selection-background-color: #5E81AC; }");
    serialPartsLayout->addWidget(m_sequenceNumberEdit);

    uniqueIdLayout->addWidget(new QLabel("序列码(10位，分段配置):"), uidRow, 0);
    uniqueIdLayout->addLayout(serialPartsLayout, uidRow++, 1);

    //校验位
    uniqueIdLayout->addWidget(new QLabel("校验位(1位):"), uidRow, 0);
    QHBoxLayout *checkLayout = new QHBoxLayout();
    m_checkCodeEdit = new QLineEdit();
    m_checkCodeEdit->setMaxLength(1);
    m_checkCodeEdit->setPlaceholderText("1位校验码");
    m_checkCodeEdit->setReadOnly(true);     //校验位自动计算，只读
    m_checkCodeEdit->setStyleSheet("QLineEdit { background-color: #3B4252; color: #ECEFF4; border: 1px solid #4C566A; border-radius: 3px; padding: 3px; selection-background-color: #5E81AC; }QLineEdit:read-only { background-color: #2E3440; color: #D8DEE9; }");
    QPushButton *calcBtn = new QPushButton("计算");
    calcBtn->setMaximumWidth(50);
    calcBtn->setStyleSheet("QPushButton { background-color: #4C566A; color: #ECEFF4; border: 1px solid #5E81AC; border-radius: 3px; padding: 3px; }QPushButton:hover { background-color: #5E81AC; }QPushButton:pressed { background-color: #81A1C1; }");
    connect(calcBtn, &QPushButton::clicked, this, &FaultAlarmWidget::onCalculateCheckCode);
    checkLayout->addWidget(m_checkCodeEdit);
    checkLayout->addWidget(calcBtn);
    checkLayout->setStretch(0, 1);     //让输入框占据更多空间
    uniqueIdLayout->addLayout(checkLayout, uidRow++, 1);
    //最终结果
    uniqueIdLayout->addWidget(new QLabel("最终结果(25位):"), uidRow, 0);
    QHBoxLayout *resultLayout = new QHBoxLayout();
    m_uniqueIdResultEdit = new QLineEdit();
    m_uniqueIdResultEdit->setMaxLength(25);
    m_uniqueIdResultEdit->setPlaceholderText("25位完整唯一标识码");
    m_uniqueIdResultEdit->setStyleSheet("QLineEdit { background-color: #3B4252; color: #ECEFF4; border: 1px solid #4C566A; border-radius: 3px; padding: 3px; selection-background-color: #5E81AC; }");
    QPushButton *generateBtn = new QPushButton("生成");
    generateBtn->setMaximumWidth(60);
    generateBtn->setStyleSheet("QPushButton { background-color: #4C566A; color: #ECEFF4; border: 1px solid #5E81AC; border-radius: 3px; padding: 3px; }QPushButton:hover { background-color: #5E81AC; }QPushButton:pressed { background-color: #81A1C1; }");
    connect(generateBtn, &QPushButton::clicked, this, &FaultAlarmWidget::onGenerateUniqueId);
    resultLayout->addWidget(m_uniqueIdResultEdit);
    resultLayout->addWidget(generateBtn);
    resultLayout->setStretch(0, 1);     //让输入框占据更多空间
    uniqueIdLayout->addLayout(resultLayout, uidRow++, 1);
    //将唯一标识码组添加到主布局
    layout->addWidget(uniqueIdGroupBox, leftRow, 0, 1, 4);
    leftRow++;
    //操作按钮 - 放在底部，跨越所有列
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *resetBtn = new QPushButton("重置数据");
    resetBtn->setStyleSheet("QPushButton { background-color: #4C566A; color: #ECEFF4; border: 1px solid #5E81AC; border-radius: 3px; padding: 5px; }QPushButton:hover { background-color: #5E81AC; }QPushButton:pressed { background-color: #81A1C1; }");
    QPushButton *loadBtn = new QPushButton("加载预设");
    loadBtn->setStyleSheet("QPushButton { background-color: #4C566A; color: #ECEFF4; border: 1px solid #5E81AC; border-radius: 3px; padding: 5px; }QPushButton:hover { background-color: #5E81AC; }QPushButton:pressed { background-color: #81A1C1; }");
    QPushButton *saveBtn = new QPushButton("保存预设");
    saveBtn->setStyleSheet("QPushButton { background-color: #4C566A; color: #ECEFF4; border: 1px solid #5E81AC; border-radius: 3px; padding: 5px; }QPushButton:hover { background-color: #5E81AC; }QPushButton:pressed { background-color: #81A1C1; }");
    connect(resetBtn, &QPushButton::clicked, this, &FaultAlarmWidget::onResetData);
    connect(loadBtn, &QPushButton::clicked, this, &FaultAlarmWidget::onLoadPreset);
    connect(saveBtn, &QPushButton::clicked, this, &FaultAlarmWidget::onSavePreset);
    buttonLayout->addWidget(resetBtn);
    buttonLayout->addWidget(loadBtn);
    buttonLayout->addWidget(saveBtn);
    layout->addLayout(buttonLayout, leftRow++, 0, 1, 4);
    return groupBox;
}

/**
 * @brief 创建发送配置部分界面
 * @return 发送配置部件指针
 */
QWidget* FaultAlarmWidget::createSendConfigWidget()
{
    QGroupBox *groupBox = new QGroupBox("发送配置");
    groupBox->setStyleSheet(
        "QGroupBox { background - color: #2E3440;color: #ECEFF4;border: 1px solid #4C566A;border - radius: 4px;margin - top: 8px;font - weight: bold;}"
        "QGroupBox::title{subcontrol - origin: margin;subcontrol - position: top center;padding: 0 5px;background - color: #3B4252;}"
        "QLabel{color: #E5E9F0;}"
    );
    QGridLayout *layout = new QGridLayout(groupBox);
    layout->setSpacing(5);     //减小间距，节省空间
    layout->setContentsMargins(5, 10, 5, 5);     //减小边距，节省空间
    //使用两列布局，更好地利用水平空间
    int leftCol = 0;
    int rightCol = 2;
    //左侧列 - 网络配置
    //目标IP地址
    layout->addWidget(new QLabel("目标IP:"), 0, leftCol);
    m_targetIpEdit = new QLineEdit();
    m_targetIpEdit->setText("127.0.0.1");
    m_targetIpEdit->setPlaceholderText("目标IP地址");
    m_targetIpEdit->setStyleSheet(
        "QLineEdit {background - color: #3B4252;color: #ECEFF4;border: 1px solid #4C566A;border - radius: 3px;padding: 3px;selection - background - color: #5E81AC;}"
    );
    layout->addWidget(m_targetIpEdit, 0, leftCol + 1);
    //目标端口
    layout->addWidget(new QLabel("目标端口:"), 1, leftCol);
    m_targetPortSpin = new QSpinBox();
    m_targetPortSpin->setRange(1, 65535);
    m_targetPortSpin->setValue(8080);
    m_targetPortSpin->setStyleSheet(
        "QSpinBox {background - color: #3B4252;color: #ECEFF4;border: 1px solid #4C566A;border - radius: 3px;padding: 3px;selection - background - color: #5E81AC;}"
        "QSpinBox::up - button, QSpinBox::down - button{background - color: #4C566A;width: 16px;border: 1px solid #5E81AC;}"
        "QSpinBox::up - button: hover, QSpinBox::down - button: hover{background - color: #5E81AC;}"
        "QSpinBox::up - button: pressed, QSpinBox::down - button: pressed{background - color: #81A1C1;}"
    );
    layout->addWidget(m_targetPortSpin, 1, leftCol + 1);
    //右侧列 - 发送配置
    //发送间隔
    layout->addWidget(new QLabel("发送间隔(ms):"), 0, rightCol);
    m_intervalSpin = new QSpinBox();
    m_intervalSpin->setRange(100, 60000);
    m_intervalSpin->setValue(1000);
    m_intervalSpin->setStyleSheet(
        "QSpinBox {background - color: #3B4252;color: #ECEFF4;border: 1px solid #4C566A;border - radius: 3px;padding: 3px;selection - background - color: #5E81AC;}"
        "QSpinBox::up - button, QSpinBox::down - button{background - color: #4C566A;width: 16px;border: 1px solid #5E81AC;}"
        "QSpinBox::up - button: hover, QSpinBox::down - button: hover{background - color: #5E81AC;}"
        "QSpinBox::up - button: pressed, QSpinBox::down - button: pressed{background - color: #81A1C1;}"
    );
    layout->addWidget(m_intervalSpin, 0, rightCol + 1);
    //发送按钮 - 使用水平布局，更好地利用空间
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_sendSingleBtn = new QPushButton("发送单帧");
    m_sendSingleBtn->setStyleSheet(
        "QPushButton {background - color: #4C566A;color: #ECEFF4;border: 1px solid #5E81AC;border - radius: 3px;padding: 5px;}"
        "QPushButton: hover { background - color: #5E81AC; }"
        "QPushButton: pressed { background - color: #81A1C1; }"
    );
    m_startContinuousBtn = new QPushButton("开始连续发送");
    m_startContinuousBtn->setStyleSheet(
        "QPushButton {background - color: #4C566A;color: #ECEFF4;border: 1px solid #5E81AC;border - radius: 3px;padding: 5px;}"
        "QPushButton: hover { background - color: #5E81AC; }"
        "QPushButton: pressed { background - color: #81A1C1; }"
    );
    m_stopContinuousBtn = new QPushButton("停止连续发送");
    m_stopContinuousBtn->setStyleSheet(
        "QPushButton {background - color: #4C566A;color: #ECEFF4;border: 1px solid #5E81AC;border - radius: 3px;padding: 5px;}"
        "QPushButton: hover { background - color: #BF616A; }"
        "QPushButton: pressed { background - color: #D08770; }"
        "QPushButton: disabled { background - color: #3B4252; color: #4C566A; }"
    );
    //设置按钮策略，使其在小屏幕上能够适当缩小
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_sendSingleBtn->setSizePolicy(sizePolicy);
    m_startContinuousBtn->setSizePolicy(sizePolicy);
    m_stopContinuousBtn->setSizePolicy(sizePolicy);
    buttonLayout->addWidget(m_sendSingleBtn);
    buttonLayout->addWidget(m_startContinuousBtn);
    buttonLayout->addWidget(m_stopContinuousBtn);
    //将按钮布局添加到主布局，跨越所有列
    layout->addLayout(buttonLayout, 2, 0, 1, 4);
    return groupBox;
}

/**
 * @brief 创建日志显示部分界面
 * @return 日志显示部件指针
 */
QWidget* FaultAlarmWidget::createLogWidget()
{
    QGroupBox *groupBox = new QGroupBox("实时日志");
    groupBox->setStyleSheet(
        "QGroupBox { background-color: #2E3440; color: #ECEFF4;  border: 1px solid #4C566A; border-radius: 4px;  margin-top: 8px; font-weight: bold; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top center; padding: 0 5px; background-color: #3B4252; }"
        "QLabel {  color: #E5E9F0; }"
    );
    QVBoxLayout *layout = new QVBoxLayout(groupBox);
    layout->setSpacing(5);     //减小间距，节省空间
    layout->setContentsMargins(5, 10, 5, 5);     //减小边距，节省空间
    //日志显示区域
    m_logTextEdit = new QTextEdit();
    m_logTextEdit->setReadOnly(true);
    //设置最小高度，确保在小屏幕上也有足够的显示空间
    m_logTextEdit->setMinimumHeight(150);
    //设置字体大小，确保在小屏幕上也能清晰显示
    QFont font = m_logTextEdit->font();
    font.setPointSize(9);     //稍微小一点的字体
    m_logTextEdit->setFont(font);
    //设置垂直滚动条策略，确保内容过多时可以滚动查看
    m_logTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    //设置水平滚动条策略，确保内容过宽时可以滚动查看
    m_logTextEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_logTextEdit->setStyleSheet(
        "QTextEdit { background-color: #3B4252; color: #ECEFF4; border: 1px solid #4C566A; border-radius: 3px;  selection-background-color: #5E81AC; selection-color: #ECEFF4; }"
    );
    layout->addWidget(m_logTextEdit);
    //清空日志按钮 - 放在右下角
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();     //添加弹性空间，使按钮靠右对齐
    m_clearLogBtn = new QPushButton("清空日志");
    //设置按钮大小策略，使其在小屏幕上能够适当缩小
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_clearLogBtn->setSizePolicy(sizePolicy);
    m_clearLogBtn->setStyleSheet(
        "QPushButton { background-color: #4C566A;  color: #ECEFF4; border: 1px solid #5E81AC; border-radius: 3px;  padding: 5px; }"
        "QPushButton:hover { background-color: #5E81AC; }"
        "QPushButton:pressed { background-color: #81A1C1; }"
    );
    buttonLayout->addWidget(m_clearLogBtn);
    layout->addLayout(buttonLayout);
    return groupBox;
}

/**
 * @brief 初始化信号槽连接
 */
void FaultAlarmWidget::initConnections()
{
    //发送相关连接
    connect(m_sendSingleBtn, &QPushButton::clicked, this, &FaultAlarmWidget::onSendSingleFrame);
    connect(m_startContinuousBtn, &QPushButton::clicked, this, &FaultAlarmWidget::onStartContinuousSend);
    connect(m_stopContinuousBtn, &QPushButton::clicked, this, &FaultAlarmWidget::onStopContinuousSend);
    connect(m_clearLogBtn, &QPushButton::clicked, this, &FaultAlarmWidget::onClearLog);
    //定时器连接
    connect(m_sendTimer, &QTimer::timeout, this, &FaultAlarmWidget::onSendTimer);
    connect(m_saveTimer, &QTimer::timeout, this, &FaultAlarmWidget::onSaveConfigurationDelayed);
    //数据变化连接
    connect(m_alarmTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FaultAlarmWidget::onAlarmTypeChanged);
    //唯一标识码分段输入连接
    connect(m_orgIdentifierEdit, &QLineEdit::textChanged, this, &FaultAlarmWidget::onUniqueIdSegmentChanged);
    connect(m_orgCodeEdit, &QLineEdit::textChanged, this, &FaultAlarmWidget::onUniqueIdSegmentChanged);
    connect(m_productDateEdit, &QLineEdit::textChanged, this, &FaultAlarmWidget::onUniqueIdSegmentChanged);
    connect(m_productCategoryEdit, &QLineEdit::textChanged, this, &FaultAlarmWidget::onUniqueIdSegmentChanged);
    connect(m_levelCodeEdit, &QLineEdit::textChanged, this, &FaultAlarmWidget::onUniqueIdSegmentChanged);
    connect(m_fixedCodeSEdit, &QLineEdit::textChanged, this, &FaultAlarmWidget::onUniqueIdSegmentChanged);
    connect(m_subsystemCodeEdit, &QLineEdit::textChanged, this, &FaultAlarmWidget::onUniqueIdSegmentChanged);
    connect(m_deviceCodeEdit, &QLineEdit::textChanged, this, &FaultAlarmWidget::onUniqueIdSegmentChanged);
    connect(m_sequenceNumberEdit, &QLineEdit::textChanged, this, &FaultAlarmWidget::onUniqueIdSegmentChanged);
    connect(m_checkCodeEdit, &QLineEdit::textChanged, this, &FaultAlarmWidget::onUniqueIdSegmentChanged);
    connect(m_uniqueIdResultEdit, &QLineEdit::textChanged, this, &FaultAlarmWidget::onUniqueIdResultChanged);
    //自动计算校验码连接（当前24位参数变化时自动计算）
    connect(m_orgIdentifierEdit, &QLineEdit::textChanged, this, &FaultAlarmWidget::onAutoCalculateCheckCode);
    connect(m_orgCodeEdit, &QLineEdit::textChanged, this, &FaultAlarmWidget::onAutoCalculateCheckCode);
    connect(m_productDateEdit, &QLineEdit::textChanged, this, &FaultAlarmWidget::onAutoCalculateCheckCode);
    connect(m_productCategoryEdit, &QLineEdit::textChanged, this, &FaultAlarmWidget::onAutoCalculateCheckCode);
    connect(m_levelCodeEdit, &QLineEdit::textChanged, this, &FaultAlarmWidget::onAutoCalculateCheckCode);
    connect(m_fixedCodeSEdit, &QLineEdit::textChanged, this, &FaultAlarmWidget::onAutoCalculateCheckCode);
    connect(m_subsystemCodeEdit, &QLineEdit::textChanged, this, &FaultAlarmWidget::onAutoCalculateCheckCode);
    connect(m_deviceCodeEdit, &QLineEdit::textChanged, this, &FaultAlarmWidget::onAutoCalculateCheckCode);
    connect(m_sequenceNumberEdit, &QLineEdit::textChanged, this, &FaultAlarmWidget::onAutoCalculateCheckCode);
    //实时保存配置连接 - 数据配置控件（使用防抖机制）
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
    //实时保存配置连接 - 唯一标识码控件（使用防抖机制）
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
    connect(m_productCategoryEdit, &QLineEdit::textChanged,
            this, [this]()
    {
        m_saveTimer->start();
    });
    connect(m_levelCodeEdit, &QLineEdit::textChanged,
            this, [this]()
    {
        m_saveTimer->start();
    });
    connect(m_fixedCodeSEdit, &QLineEdit::textChanged,
            this, [this]()
    {
        m_saveTimer->start();
    });
    connect(m_subsystemCodeEdit, &QLineEdit::textChanged,
            this, [this]()
    {
        m_saveTimer->start();
    });
    connect(m_deviceCodeEdit, &QLineEdit::textChanged,
            this, [this]()
    {
        m_saveTimer->start();
    });
    connect(m_sequenceNumberEdit, &QLineEdit::textChanged,
            this, [this]()
    {
        m_saveTimer->start();
    });
    connect(m_uniqueIdResultEdit, &QLineEdit::textChanged,
            this, [this]()
    {
        m_saveTimer->start();
    });
    //实时保存配置连接 - 发送配置控件（使用防抖机制）
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
    //根据告警类型启用/禁用相关控件
    if(alarmType == 0x01)        //故障码
    {
        m_warningValueSpin->setEnabled(false);
        m_warningThresholdSpin->setEnabled(false);
        m_isolationCombo->setEnabled(true);
        m_faultLevelCombo->setEnabled(true);
        //设置默认值
        m_warningValueSpin->setValue(0.0);
        m_warningThresholdSpin->setValue(0.0);
    }
    else         //预警码
    {
        m_warningValueSpin->setEnabled(true);
        m_warningThresholdSpin->setEnabled(true);
        m_isolationCombo->setEnabled(false);
        m_faultLevelCombo->setEnabled(false);
        //设置默认值
        m_isolationCombo->setCurrentIndex(0);     //填0
        m_faultLevelCombo->setCurrentIndex(0);     //填0
    }
}

/**
 * @brief 生成唯一标识码
 */
void FaultAlarmWidget::onGenerateUniqueId()
{
    //生成组织机构标识符 (1位)
    m_orgIdentifierEdit->setText("1");
    //生成组织机构代码 (9位) - 示例代码
    QString orgCode = "633772342";
    m_orgCodeEdit->setText(orgCode);
    //生成生产日期 (8位) - 当前日期
    QString productDate = QDate::currentDate().toString("yyyyMMdd");
    m_productDateEdit->setText(productDate);
    //生成序列码 (10位) - 分段配置
    //产品大类(2位) - 默认01
    m_productCategoryEdit->setText("01");
    //层级(2位) - 默认00
    m_levelCodeEdit->setText("00");
    //固定码(1位) - 默认S
    m_fixedCodeSEdit->setText("S");
    //分系统(1位) - 默认G
    m_subsystemCodeEdit->setText("G");
    //设备(2位) - 默认01
    m_deviceCodeEdit->setText("01");
    //序号(2位) - 随机
    QString randomSeq = QString("%1").arg(QRandomGenerator::global()->bounded(0, 99), 2, 10, QChar('0'));
    m_sequenceNumberEdit->setText(randomSeq);
    
    //计算校验码
    onCalculateCheckCode();
    //更新最终结果
    onUniqueIdSegmentChanged();
    QString uniqueId = m_uniqueIdResultEdit->text();
    appendLog("INFO", QString("生成唯一标识码: %1").arg(uniqueId));
}

/**
 * @brief 唯一标识码分段输入变化处理
 */
void FaultAlarmWidget::onUniqueIdSegmentChanged()
{
    //组合各个分段生成完整的唯一标识码
    QString orgIdentifier = m_orgIdentifierEdit->text().leftJustified(1, 'E', true).left(1);
    QString orgCode = m_orgCodeEdit->text().leftJustified(9, '0', true).left(9);
    
    // 处理日期转换 (8位yyyyMMdd -> 4位编码)
    QString dateStr = m_productDateEdit->text();
    QString dateCode;
    if (dateStr.length() == 8) {
         QDate date = QDate::fromString(dateStr, "yyyyMMdd");
         if(date.isValid())
             dateCode = dateToCode(date);
         else
             dateCode = "0000";
    } else {
         dateCode = dateStr.leftJustified(4, '0', true).left(4); 
    }
    
    // 组合序列码 (10位)
    QString serialCode = m_productCategoryEdit->text().leftJustified(2, '0', true).left(2) +
                         m_levelCodeEdit->text().leftJustified(2, '0', true).left(2) +
                         m_fixedCodeSEdit->text().leftJustified(1, 'S', true).left(1) +
                         m_subsystemCodeEdit->text().leftJustified(1, 'G', true).left(1) +
                         m_deviceCodeEdit->text().leftJustified(2, '0', true).left(2) +
                         m_sequenceNumberEdit->text().leftJustified(2, '0', true).left(2);

    QString checkCode = m_checkCodeEdit->text().leftJustified(1, '0', true).left(1);
    
    QString fullUniqueId = orgIdentifier + orgCode + dateCode + serialCode + checkCode;
    //阻止递归调用
    m_uniqueIdResultEdit->blockSignals(true);
    m_uniqueIdResultEdit->setText(fullUniqueId);
    m_uniqueIdResultEdit->blockSignals(false);
}

/**
 * @brief 计算校验码
 */
void FaultAlarmWidget::onCalculateCheckCode()
{
    //获取前24位
    QString orgIdentifier = m_orgIdentifierEdit->text().leftJustified(1, 'E', true).left(1);
    QString orgCode = m_orgCodeEdit->text().leftJustified(9, '0', true).left(9);
    
    // 处理日期转换
    QString dateStr = m_productDateEdit->text();
    QString dateCode;
    if (dateStr.length() == 8) {
         QDate date = QDate::fromString(dateStr, "yyyyMMdd");
         if(date.isValid())
             dateCode = dateToCode(date);
         else
             dateCode = "0000";
    } else {
         dateCode = dateStr.leftJustified(4, '0', true).left(4); 
    }
    
    // 组合序列码 (10位)
    QString serialCode = m_productCategoryEdit->text().leftJustified(2, '0', true).left(2) +
                         m_levelCodeEdit->text().leftJustified(2, '0', true).left(2) +
                         m_fixedCodeSEdit->text().leftJustified(1, 'S', true).left(1) +
                         m_subsystemCodeEdit->text().leftJustified(1, 'G', true).left(1) +
                         m_deviceCodeEdit->text().leftJustified(2, '0', true).left(2) +
                         m_sequenceNumberEdit->text().leftJustified(2, '0', true).left(2);

    QString first24 = orgIdentifier + orgCode + dateCode + serialCode;
    
    //计算校验码
    int checkSum = 0;
    for(const QChar &ch : first24)
    {
        if(ch.isDigit())
        {
            checkSum += ch.digitValue();
        }
        else
        {
            checkSum += getCharValue(ch.toLatin1()); // Use getCharValue helper
        }
    }
    QString checkCode = QString::number(checkSum % 10);
    //更新校验位
    m_checkCodeEdit->blockSignals(true);
    m_checkCodeEdit->setText(checkCode);
    m_checkCodeEdit->blockSignals(false);
    //更新完整结果
    onUniqueIdSegmentChanged();
}

/**
 * @brief 自动计算校验码（参数变化时触发）
 */
void FaultAlarmWidget::onAutoCalculateCheckCode()
{
    //获取前24位
    QString orgIdentifier = m_orgIdentifierEdit->text().leftJustified(1, 'E', true).left(1);
    QString orgCode = m_orgCodeEdit->text().leftJustified(9, '0', true).left(9);
    QString dateStr = m_productDateEdit->text();
    
    // 组合序列码 (10位)
    QString serialCode = m_productCategoryEdit->text().leftJustified(2, '0', true).left(2) +
                         m_levelCodeEdit->text().leftJustified(2, '0', true).left(2) +
                         m_fixedCodeSEdit->text().leftJustified(1, 'S', true).left(1) +
                         m_subsystemCodeEdit->text().leftJustified(1, 'G', true).left(1) +
                         m_deviceCodeEdit->text().leftJustified(2, '0', true).left(2) +
                         m_sequenceNumberEdit->text().leftJustified(2, '0', true).left(2);
    
    //只有当前24位都有内容时才自动计算校验码
    if(!orgIdentifier.isEmpty() && !orgCode.isEmpty() && !dateStr.isEmpty() && !serialCode.isEmpty())
    {
        // 处理日期转换
        QString dateCode;
        if (dateStr.length() == 8) {
             QDate date = QDate::fromString(dateStr, "yyyyMMdd");
             if(date.isValid())
                 dateCode = dateToCode(date);
             else
                 dateCode = "0000";
        } else {
             dateCode = dateStr.leftJustified(4, '0', true).left(4); 
        }
        
        QString first24 = orgIdentifier + orgCode + dateCode + serialCode;
        //计算校验码
        int checkSum = 0;
        for(const QChar &ch : first24)
        {
            if(ch.isDigit())
            {
                checkSum += ch.digitValue();
            }
            else
            {
                checkSum += getCharValue(ch.toLatin1());     //Use getCharValue
            }
        }
        QString checkCode = QString::number(checkSum % 10);
        //更新校验位（阻止信号避免递归）
        m_checkCodeEdit->blockSignals(true);
        m_checkCodeEdit->setText(checkCode);
        m_checkCodeEdit->blockSignals(false);
        //更新完整结果
        onUniqueIdSegmentChanged();
    }
}

/**
 * @brief 唯一标识码结果编辑变化处理
 */
void FaultAlarmWidget::onUniqueIdResultChanged()
{
    //当用户直接编辑结果时，尝试解析并更新各个分段
    QString fullId = m_uniqueIdResultEdit->text();
    if(fullId.length() >= 25)
    {
        //阻止递归调用
        m_orgIdentifierEdit->blockSignals(true);
        m_orgCodeEdit->blockSignals(true);
        m_productDateEdit->blockSignals(true);
        m_productCategoryEdit->blockSignals(true);
        m_levelCodeEdit->blockSignals(true);
        m_fixedCodeSEdit->blockSignals(true);
        m_subsystemCodeEdit->blockSignals(true);
        m_deviceCodeEdit->blockSignals(true);
        m_sequenceNumberEdit->blockSignals(true);
        m_checkCodeEdit->blockSignals(true);
        //解析各个分段
        m_orgIdentifierEdit->setText(fullId.mid(0, 1));
        m_orgCodeEdit->setText(fullId.mid(1, 9));
        
        // Date Code (4 bytes) -> Date String (8 bytes)
        QString dateCode = fullId.mid(10, 4);
        QDate date = codeToDate(dateCode);
        m_productDateEdit->setText(date.toString("yyyyMMdd"));
        
        // Serial Code (10 bytes) -> 6 parts
        m_productCategoryEdit->setText(fullId.mid(14, 2));
        m_levelCodeEdit->setText(fullId.mid(16, 2));
        m_fixedCodeSEdit->setText(fullId.mid(18, 1));
        m_subsystemCodeEdit->setText(fullId.mid(19, 1));
        m_deviceCodeEdit->setText(fullId.mid(20, 2));
        m_sequenceNumberEdit->setText(fullId.mid(22, 2));

        m_checkCodeEdit->setText(fullId.mid(24, 1));
        //恢复信号
        m_orgIdentifierEdit->blockSignals(false);
        m_orgCodeEdit->blockSignals(false);
        m_productDateEdit->blockSignals(false);
        m_productCategoryEdit->blockSignals(false);
        m_levelCodeEdit->blockSignals(false);
        m_fixedCodeSEdit->blockSignals(false);
        m_subsystemCodeEdit->blockSignals(false);
        m_deviceCodeEdit->blockSignals(false);
        m_sequenceNumberEdit->blockSignals(false);
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
    m_controlCombo->setCurrentIndex(1);     //默认选择1
    m_formatCombo->setCurrentIndex(0);      //默认选择十进制
    m_alarmTypeCombo->setCurrentIndex(0);
    m_faultCodeSpin->setValue(1001);
    m_isolationCombo->setCurrentIndex(0);
    m_faultLevelCombo->setCurrentIndex(0);
    m_warningValueSpin->setValue(100.0);
    m_warningThresholdSpin->setValue(150.0);
    m_topicNumberSpin->setValue(1000);
    m_sourceDeviceSpin->setValue(1);
    m_destDeviceSpin->setValue(2);
    //重置唯一标识码分段输入
    m_orgIdentifierEdit->setText("1");
    m_orgCodeEdit->setText("633772342");
    m_productDateEdit->setText(QDate::currentDate().toString("yyyyMMdd"));
    
    //重置序列码分段
    m_productCategoryEdit->setText("");
    m_levelCodeEdit->setText("");
    m_fixedCodeSEdit->setText("S");
    m_subsystemCodeEdit->setText("G");
    m_deviceCodeEdit->setText("");
    m_sequenceNumberEdit->setText("");

    m_checkCodeEdit->clear();
    m_uniqueIdResultEdit->clear();
    //计算校验码并更新结果
    onCalculateCheckCode();
    //重置数值格式为十进制
    onFormatChanged();
    appendLog("INFO", "数据已重置为默认值");
}

/**
 * @brief 数值格式改变处理
 */
void FaultAlarmWidget::onFormatChanged()
{
    int format = m_formatCombo->currentData().toInt();
    int base = (format == 0) ? 10 : 16;     //0=十进制, 1=十六进制
    //更新所有数值输入控件的显示格式
    m_faultCodeSpin->setDisplayIntegerBase(base);
    m_topicNumberSpin->setDisplayIntegerBase(base);
    m_sourceDeviceSpin->setDisplayIntegerBase(base);
    m_destDeviceSpin->setDisplayIntegerBase(base);
    //更新前缀显示
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
    //更新当前数据
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
    m_currentData.uniqueId = m_uniqueIdResultEdit->text().leftJustified(25, '\0', true);
    //数据域长度固定为38字节 (1+2+1+1+4+4+25)
    m_currentData.dataLength = 38;
    //构建帧头 (11字节)
    frame.append(m_currentData.control);
    //数据域长度 (先低后高)
    frame.append(static_cast<char>(m_currentData.dataLength & 0xFF));
    frame.append(static_cast<char>((m_currentData.dataLength >> 8) & 0x07));     //高3位
    //主题号 (先低后高)
    frame.append(static_cast<char>(m_currentData.topicNumber & 0xFF));
    frame.append(static_cast<char>((m_currentData.topicNumber >> 8) & 0xFF));
    //源设备号和目的设备号
    frame.append(m_currentData.sourceDevice);
    frame.append(m_currentData.destDevice);
    //时间戳 (4字节，先低后高)
    frame.append(static_cast<char>(m_currentData.timestamp & 0xFF));
    frame.append(static_cast<char>((m_currentData.timestamp >> 8) & 0xFF));
    frame.append(static_cast<char>((m_currentData.timestamp >> 16) & 0xFF));
    frame.append(static_cast<char>((m_currentData.timestamp >> 24) & 0xFF));
    //构建数据域 (37字节)
    frame.append(m_currentData.alarmIdentifier);
    //故障码/预警码 (先低后高)
    frame.append(static_cast<char>(m_currentData.faultCode & 0xFF));
    frame.append(static_cast<char>((m_currentData.faultCode >> 8) & 0xFF));
    frame.append(m_currentData.isolationFlag);
    frame.append(m_currentData.faultLevel);
    //预警数值 (float 4字节)
    //注意：这里假设float为IEEE 754标准，且字节序匹配
    const char *pWarningValue = reinterpret_cast<const char *>(&m_currentData.warningValue);
    frame.append(pWarningValue, 4);
    //预警阈值 (float 4字节)
    const char *pWarningThreshold = reinterpret_cast<const char *>(&m_currentData.warningThreshold);
    frame.append(pWarningThreshold, 4);
    //唯一标识码 (25字节)
    QByteArray uniqueIdBytes = m_currentData.uniqueId.toLatin1();
    //确保长度为25，不足补0
    if(uniqueIdBytes.length() < 25)
    {
        uniqueIdBytes = uniqueIdBytes.leftJustified(25, '\0');
    }
    else if(uniqueIdBytes.length() > 25)
    {
        uniqueIdBytes = uniqueIdBytes.left(25);
    }
    frame.append(uniqueIdBytes);
    //计算校验和 (从帧头到数据域结束的所有字节累加和)
    uint8_t checksum = 0;
    for(char byte : frame)
    {
        checksum += static_cast<uint8_t>(byte);
    }
    m_currentData.checksum = checksum;
    frame.append(checksum);
    //帧尾 (1字节)
    frame.append(static_cast<char>(m_currentData.frameTail));
    return frame;
}

// 辅助函数实现

char FaultAlarmWidget::toBase33(int val)
{
    static const QString base33Digits = "0123456789ABCDEFGHJKLMNPQRSTUVWXYZ";
    if (val >= 0 && val < base33Digits.length())
    {
        return base33Digits[val].toLatin1();
    }
    return '0'; // Fallback
}

int FaultAlarmWidget::fromBase33(char c)
{
    static const QString base33Digits = "0123456789ABCDEFGHJKLMNPQRSTUVWXYZ";
    int idx = base33Digits.indexOf(c);
    if (idx != -1) return idx;
    
    return 0; // Fallback
}

QString FaultAlarmWidget::dateToCode(const QDate& date)
{
    if (!date.isValid()) return "0000";
    
    // Year: Remove first 2 digits, then Hex. e.g. 2025 -> 25 -> 19
    int year = date.year() % 100;
    QString yearHex = QString("%1").arg(year, 2, 16, QChar('0')).toUpper();
    
    // Month: Base33
    char monthChar = toBase33(date.month());
    
    // Day: Base33
    char dayChar = toBase33(date.day());
    
    return yearHex + QString(monthChar) + QString(dayChar);
}

QDate FaultAlarmWidget::codeToDate(const QString& code)
{
    if (code.length() != 4) return QDate::currentDate();
    
    // Year
    bool ok;
    int yearVal = code.mid(0, 2).toInt(&ok, 16);
    int year = 2000 + yearVal; // Assume 20xx
    
    // Month
    int month = fromBase33(code[2].toLatin1());
    
    // Day
    int day = fromBase33(code[3].toLatin1());
    
    return QDate(year, month, day);
}

int FaultAlarmWidget::getCharValue(char c)
{
    return fromBase33(c);
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
    //高16位与低16位异或
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
    //计算从00:00:00开始的毫秒数
    m_currentData.timestamp = time.msecsSinceStartOfDay();
}

/**
 * @brief 生成唯一标识码
 * @return 24位唯一标识码字符串
 */
QString FaultAlarmWidget::generateUniqueId()
{
    //军地组织机构标识符 (1位)
    QString orgIdentifier = "E";
    //组织机构代码 (9位) - 示例代码
    QString orgCode = "633772342";
    //生产日期 (8位) -> 4位编码
    QDate date = QDate::currentDate();
    QString dateCode = dateToCode(date);
    //序列码 (10位)
    QString serialCode = QString("%1").arg(QRandomGenerator::global()->bounded(0, 999999999), 10, 10, QChar('0'));
    //前24位
    QString first24 = orgIdentifier + orgCode + dateCode + serialCode;
    //计算校验码
    int checkSum = 0;
    for(const QChar &ch : first24)
    {
        if(ch.isDigit())
        {
            checkSum += ch.digitValue();
        }
        else
        {
            checkSum += getCharValue(ch.toLatin1());
        }
    }
    QString checkCode = QString::number(checkSum % 10);
    return first24 + checkCode;
}

/**
 * @brief 更新界面显示
 */
void FaultAlarmWidget::updateUI()
{
    //根据当前数据更新界面控件
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
    //设置颜色
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
    //自动滚动到底部
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
    //将当前配置转换为JSON
    QJsonObject configJson = dataToJson();
    QJsonDocument doc(configJson);
    //保存到文件（静默保存）
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
    //将当前配置转换为JSON
    QJsonObject configJson = dataToJson();
    QJsonDocument doc(configJson);
    //保存到文件
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
    //保存唯一标识码分段信息
    json["orgIdentifier"] = m_orgIdentifierEdit->text();
    json["orgCode"] = m_orgCodeEdit->text();
    json["productDate"] = m_productDateEdit->text();
    
    //保存序列码分段
    json["productCategory"] = m_productCategoryEdit->text();
    json["levelCode"] = m_levelCodeEdit->text();
    json["fixedCodeS"] = m_fixedCodeSEdit->text();
    json["subsystemCode"] = m_subsystemCodeEdit->text();
    json["deviceCode"] = m_deviceCodeEdit->text();
    json["sequenceNumber"] = m_sequenceNumberEdit->text();

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
    //加载唯一标识码分段信息
    if(json.contains("orgCode"))
    {
        m_orgCodeEdit->setText(json["orgCode"].toString());
        if(json.contains("orgIdentifier"))
            m_orgIdentifierEdit->setText(json["orgIdentifier"].toString());
        m_productDateEdit->setText(json["productDate"].toString());
        
        //加载序列码分段
        if(json.contains("productCategory")) {
            m_productCategoryEdit->setText(json["productCategory"].toString());
            m_levelCodeEdit->setText(json["levelCode"].toString());
            m_fixedCodeSEdit->setText(json["fixedCodeS"].toString());
            m_subsystemCodeEdit->setText(json["subsystemCode"].toString());
            m_deviceCodeEdit->setText(json["deviceCode"].toString());
            m_sequenceNumberEdit->setText(json["sequenceNumber"].toString());
        } else if(json.contains("serialCode")) {
             // 兼容旧的完整序列码字段 (如果有)
             QString serialCode = json["serialCode"].toString();
             if(serialCode.length() >= 10) {
                 m_productCategoryEdit->setText(serialCode.mid(0, 2));
                 m_levelCodeEdit->setText(serialCode.mid(2, 2));
                 m_fixedCodeSEdit->setText(serialCode.mid(4, 1));
                 m_subsystemCodeEdit->setText(serialCode.mid(5, 1));
                 m_deviceCodeEdit->setText(serialCode.mid(6, 2));
                 m_sequenceNumberEdit->setText(serialCode.mid(8, 2));
             }
        }

        m_checkCodeEdit->setText(json["checkCode"].toString());
        m_uniqueIdResultEdit->setText(json["uniqueIdResult"].toString());
    }
    else
    {
        //兼容旧版本配置
        QString oldUniqueId = json["uniqueId"].toString();
        if(!oldUniqueId.isEmpty() && oldUniqueId.length() >= 24)
        {
            m_orgCodeEdit->setText(oldUniqueId.mid(0, 9));
            m_productDateEdit->setText(oldUniqueId.mid(9, 8));
            // 旧版序列码只有6位，无法直接映射到新的10位分段
            // 这里我们只设置结果显示，分段留空或设默认
            m_checkCodeEdit->setText(oldUniqueId.mid(23, 1));
            m_uniqueIdResultEdit->setText(oldUniqueId);
        }
    }
    m_targetIpEdit->setText(json["targetIp"].toString());
    m_targetPortSpin->setValue(json["targetPort"].toInt());
    m_intervalSpin->setValue(json["interval"].toInt());
}
