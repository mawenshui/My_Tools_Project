#ifndef SSHCLIENTWIDGET_H
#define SSHCLIENTWIDGET_H

#include <QWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QTemporaryDir>
#include "sshclient.h"
#include "keygendialog.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class SSHClientWidget;
}
QT_END_NAMESPACE

class SSHClientWidget : public QWidget
{
    Q_OBJECT

public:
    SSHClientWidget(QWidget *parent = nullptr);
    ~SSHClientWidget();

private slots:
    // 连接相关槽函数
    void on_connectButton_clicked();
    void on_disconnectButton_clicked();
    void on_executeButton_clicked();
    void on_checkNetworkButton_clicked();
    // 移除了密码认证相关的槽函数声明
    void on_browseKeyFileButton_clicked();
    void on_generateKeyButton_clicked();
    void onAuthModeChanged();
    void onKeyGenFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onKeyGenError(QProcess::ProcessError error);

    // 处理命令输入
    void onCommandEntered();

    // 处理SSH客户端的信号
    void onConnected();
    void onDisconnected();
    void onConnectionError(const QString &error);
    void onCommandOutput(const QString &output);
    void onCommandError(const QString &error);
    void onNetworkCheckComplete(bool isConnected);
    void onNetworkCheckError(const QString &error);

private:
    // 辅助函数
    void findAndSetSshKeyPath();
    void updateUIState(bool connected);
    void appendOutput(const QString &text, bool isError = false);
    void appendOutput(const QString &text, bool isError, bool isCommand, bool isSystem);
    void updateAuthUIState();
    bool installSshpassFromLocal();
    bool isToolAvailable(const QString &tool);
    bool deployKey();
    bool deployKeyWithSshpass();
    bool deployKeyWithoutPassword();

private:
    Ui::SSHClientWidget* ui;
    SSHClient* m_sshClient;
    QProcess* m_keyGenProcess;
};
#endif // SSHCLIENTWIDGET_H
