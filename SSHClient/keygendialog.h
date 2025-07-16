#ifndef KEYGENDIALOG_H
#define KEYGENDIALOG_H

#include <QDialog>

namespace Ui
{
    class KeyGenDialog;
}

class KeyGenDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KeyGenDialog(QWidget *parent = nullptr);
    ~KeyGenDialog();

    // 获取用户输入的值
    QString getEmail() const;
    QString getKeyType() const;
    int getKeyBits() const;
    QString getPassword() const;

private slots:
    // 处理密钥类型变化，控制密钥长度选项的可见性
    void onKeyTypeChanged(int index);

    // 验证输入并接受对话框
    void validateAndAccept();

private:
    Ui::KeyGenDialog* ui;
    QString m_strEmail;
    QString m_strKeyType;
    int m_iKeyBits;
    QString m_strPassword;
};

#endif // KEYGENDIALOG_H
