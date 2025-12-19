// multipliersettingsdialog.cpp
#include "multipliersettingsdialog.h"
#include "ui_multipliersettingsdialog.h"

/**
 * @brief 构造倍率设置对话框
 * 使用由 .ui 生成的界面，初始化控件参数并从配置载入当前值。
 */
MultiplierSettingsDialog::MultiplierSettingsDialog(ConfigManager* cfg, QWidget* parent)
    : QDialog(parent), m_cfg(cfg), ui(new Ui::MultiplierSettingsDialog)
{
    ui->setupUi(this); // 使用 .ui 声明的界面
    setObjectName("multiplierSettingsDialog");
    setAttribute(Qt::WA_StyledBackground, true);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    // 配置三个倍率输入控件（范围、步进与小数位），仅调用已存在控件
    auto setupSpin = [](QDoubleSpinBox* s){
        s->setRange(0.001, 3.0);
        s->setDecimals(3);
        s->setSingleStep(0.001);
        s->setAlignment(Qt::AlignRight);
    };
    setupSpin(ui->fastSpinBox);
    setupSpin(ui->normalSpinBox);
    setupSpin(ui->checkedSpinBox);

    // 载入当前配置（回退默认常量）
    ui->fastSpinBox->setValue(m_cfg->get("flowFastMultiplier", FLOW_FAST_MULTIPLIER).toDouble());
    ui->normalSpinBox->setValue(m_cfg->get("flowNormalMultiplier", FLOW_NORMAL_MULTIPLIER).toDouble());
    ui->checkedSpinBox->setValue(m_cfg->get("timeoutCheckedMultiplier", TIMEOUT_CHECKED_MULTIPLIER).toDouble());

    // 连接按钮
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &MultiplierSettingsDialog::onAccept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &MultiplierSettingsDialog::reject);
}

/**
 * @brief 析构函数，释放 UI 资源
 */
MultiplierSettingsDialog::~MultiplierSettingsDialog()
{
    delete ui;
}

void MultiplierSettingsDialog::onAccept()
{
    // 写入配置：控件保证范围合法，无需额外校验；仍可做保护以防未来变更
    const double fast = ui->fastSpinBox->value();
    const double normal = ui->normalSpinBox->value();
    const double checked = ui->checkedSpinBox->value();

    m_cfg->set("flowFastMultiplier", fast);
    m_cfg->set("flowNormalMultiplier", normal);
    m_cfg->set("timeoutCheckedMultiplier", checked);
    // 立即持久化，确保发送线程及后续会话可见
    m_cfg->saveConfig();

    accept();
}
