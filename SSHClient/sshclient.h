#ifndef SSHCLIENT_H
#define SSHCLIENT_H

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTcpSocket>
#include <QRegExp>

class SSHClient : public QObject
{
    Q_OBJECT
public:
    // 认证方式枚举
    enum AuthMode {
        KeyFile,    // 密钥文件认证
        Password    // 密码认证
    };
    
    explicit SSHClient(QObject *parent = nullptr);
    ~SSHClient();
    
    // 使用密钥文件连接到远程主机
    void connectToHost(const QString &host, int port, const QString &username, const QString &keyFilePath, const QString &keyFilePassword = QString());
    
    // 使用密码连接到远程主机
    void connectToHostWithPassword(const QString &host, int port, const QString &username, const QString &password);
    
    // 断开连接
    void disconnect();
    
    // 执行命令
    void executeCommand(const QString &command);
    
    // 检查连接状态
    bool isConnected() const;
    
    // 检查网络连接状态
    void checkNetworkConnection(const QString &host, int port = 22);

signals:
    // 连接相关信号
    void connected();
    void disconnected();
    void connectionError(const QString &error);
    
    // 命令执行相关信号
    void commandOutput(const QString &output);
    void commandError(const QString &error);
    
    // 网络连接状态信号
    void networkCheckComplete(bool isConnected);
    void networkCheckError(const QString &error);

private slots:
    // 处理SSH进程的输出
    void onProcessStarted();
    void onProcessError(QProcess::ProcessError error);
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessReadyReadStandardOutput();
    void onProcessReadyReadStandardError();
    void onConnectionTimeout();
    
    // 处理网络连接检测
    void onNetworkReplyFinished(QNetworkReply *reply);
    void onNetworkReplyError(QNetworkReply::NetworkError error);
    void checkNetworkTimeout();

private:
    QProcess *m_process;
    QTimer *m_connectionTimer;
    bool m_connected;
    
    // 网络连接检测
    QNetworkAccessManager *m_networkManager;
    
    // 连接参数
    QString m_host;
    int m_port;
    QString m_username;
    QString m_keyFilePath;
    QString m_keyFilePassword;
    QString m_password;  // 密码认证时使用
    AuthMode m_authMode;
    int m_connectionTimeout = 5000; // 连接超时时间，默认5秒
    
    // 当前命令
    QString m_currentCommand;
    
    // 辅助函数
    void setupProcess();
    void cleanupProcess();
    QString getSSHCommand(const QString &command = QString()) const;
    bool isIPAddress(const QString &address);
    void pingHost(const QString &host);
};

#endif // SSHCLIENT_H