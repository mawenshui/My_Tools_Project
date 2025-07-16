#include "keygendialog.h"
#include "ui_keygendialog.h"
#include <QMessageBox>

KeyGenDialog::KeyGenDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::KeyGenDialog)
{
    ui->setupUi(this);
    // 连接信号和槽
    connect(ui->typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &KeyGenDialog::onKeyTypeChanged);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &KeyGenDialog::validateAndAccept);
    // 初始化UI状态
    onKeyTypeChanged(ui->typeCombo->currentIndex());
}

KeyGenDialog::~KeyGenDialog()
{
    delete ui;
}

QString KeyGenDialog::getEmail() const
{
    return m_strEmail;
}

QString KeyGenDialog::getKeyType() const
{
    return m_strKeyType;
}

int KeyGenDialog::getKeyBits() const
{
    return m_iKeyBits;
}

QString KeyGenDialog::getPassword() const
{
    return m_strPassword;
}

void KeyGenDialog::onKeyTypeChanged(int index)
{
    Q_UNUSED(index);
    // 只有RSA类型才显示密钥长度选项
    bool isRSA = (ui->typeCombo->currentData().toString().toLower().contains("rsa"));
    ui->bitsLabel->setVisible(isRSA);
    ui->bitsCombo->setVisible(isRSA);
}

void KeyGenDialog::validateAndAccept()
{
    // 验证密码是否匹配
    if(ui->passEdit->text() != ui->confirmEdit->text())
    {
        QMessageBox::warning(this, "密码不匹配", "两次输入的密码不一致，请重新输入。");
        ui->passEdit->clear();
        ui->confirmEdit->clear();
        ui->passEdit->setFocus();
        return;
    }
    // 验证通过，接受对话框
    m_strEmail = ui->emailEdit->text();
    m_strKeyType = ui->typeCombo->currentText().remove(QRegularExpression(R"(\([^)]*\))")).trimmed(); //去除括号部分
    m_iKeyBits = QRegularExpression("\\d+").match(ui->bitsCombo->currentText()).captured(0).toInt(); //去除数字之外的内容
    m_strPassword = ui->passEdit->text();
    accept();
}
