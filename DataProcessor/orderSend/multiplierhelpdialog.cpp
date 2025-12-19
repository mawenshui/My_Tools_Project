// multiplierhelpdialog.cpp
#include "multiplierhelpdialog.h"
#include "ui_multiplierhelpdialog.h"

/**
 * @brief 构造帮助对话框
 * 使用由 .ui 生成的界面，填充说明文本并连接按钮事件。
 */
MultiplierHelpDialog::MultiplierHelpDialog(ConfigManager* cfg, QWidget* parent)
    : QDialog(parent), m_cfg(cfg), ui(new Ui::MultiplierHelpDialog)
{
    ui->setupUi(this); // 使用 .ui 声明的界面
    setObjectName("multiplierHelpDialog");
    setAttribute(Qt::WA_StyledBackground, true);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    // 设置说明文本内容（调用现有标签，不创建控件）
    ui->dialogBodyText->setWordWrap(true);
    ui->dialogBodyText->setText(tr(
        "说明：\n"
        "- 快速流程倍率：对默认超时（应答/反馈/反馈应答）统一乘以该系数；\n"
        "- 正常流程倍率：同上，但系数不同；\n"
        "- 勾选超时倍率：当勾选对应超时复选框时，仅该项按默认×此系数，其它不延时；\n"
        "- 手动流程：不应用倍率，直接使用用户输入的毫秒值。\n\n"
        "范围：0.001–3.000。倍率为非法或缺失时回退到默认常量。\n"
        "默认常量：快速=%1，正常=%2，超时=%3。"
    ).arg(QString::number(FLOW_FAST_MULTIPLIER, 'g', 3))
     .arg(QString::number(FLOW_NORMAL_MULTIPLIER, 'g', 3))
     .arg(QString::number(TIMEOUT_CHECKED_MULTIPLIER, 'g', 3)));

    // 连接按钮（恢复默认与关闭）
    connect(ui->restoreButton, &QPushButton::clicked, this, &MultiplierHelpDialog::onRestoreDefaults);
    connect(ui->dialogButtons, &QDialogButtonBox::rejected, this, &MultiplierHelpDialog::accept);
}

/**
 * @brief 析构函数，释放 UI 资源
 */
MultiplierHelpDialog::~MultiplierHelpDialog()
{
    delete ui;
}

void MultiplierHelpDialog::onRestoreDefaults()
{
    QVariantMap changes;
    changes["flowFastMultiplier"] = FLOW_FAST_MULTIPLIER;
    changes["flowNormalMultiplier"] = FLOW_NORMAL_MULTIPLIER;
    changes["timeoutCheckedMultiplier"] = TIMEOUT_CHECKED_MULTIPLIER;
    m_cfg->updateConfig(changes); // 发出变更信号并写入
}
