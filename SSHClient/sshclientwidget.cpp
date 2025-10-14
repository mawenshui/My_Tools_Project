#include "sshclientwidget.h"
#include "ui_sshclientwidget.h"
#include "sshclient.h"
#include "keygendialog.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDebug>
#include <QTextStream>
#include <QClipboard>
#include <QApplication>
#include <QTimer>

SSHClientWidget::SSHClientWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SSHClientWidget)
    , m_sshClient(nullptr)
    , m_keyGenProcess(nullptr)
{
    ui->setupUi(this);
    // 创建SSH客户端
    m_sshClient = new SSHClient(this);
    // 初始化密钥生成进程
    m_keyGenProcess = new QProcess(this);
    // 设置窗口标题和图标
    setWindowTitle("SSH客户端 - 安全连接工具");
    // 设置输出文本框的字体和样式
    QFont consoleFont("Consolas", 9);
    ui->outputTextEdit->setFont(consoleFont);
    ui->outputTextEdit->setLineWrapMode(QPlainTextEdit::NoWrap); // 禁用自动换行
    // 设置命令输入框的字体
    ui->commandLineEdit->setFont(consoleFont);
    // 设置整体样式
    QString styleSheet = "QGroupBox { font-weight: bold; }";
    styleSheet += "QLineEdit, QPlainTextEdit { border: 1px solid #C0C0C0; border-radius: 3px; padding: 2px; }";
    styleSheet += "QPushButton { padding: 4px 8px; }";
    setStyleSheet(styleSheet);
    // 连接信号和槽
    connect(ui->commandLineEdit, &QLineEdit::returnPressed, this, &SSHClientWidget::onCommandEntered);
    connect(m_sshClient, &SSHClient::connected, this, &SSHClientWidget::onConnected);
    connect(m_sshClient, &SSHClient::disconnected, this, &SSHClientWidget::onDisconnected);
    connect(m_sshClient, &SSHClient::connectionError, this, &SSHClientWidget::onConnectionError);
    connect(m_sshClient, &SSHClient::commandOutput, this, &SSHClientWidget::onCommandOutput);
    connect(m_sshClient, &SSHClient::commandError, this, &SSHClientWidget::onCommandError);
    connect(m_sshClient, &SSHClient::networkCheckComplete, this, &SSHClientWidget::onNetworkCheckComplete);
    connect(m_sshClient, &SSHClient::networkCheckError, this, &SSHClientWidget::onNetworkCheckError);
    connect(m_keyGenProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &SSHClientWidget::onKeyGenFinished);
    connect(m_keyGenProcess, &QProcess::errorOccurred, this, &SSHClientWidget::onKeyGenError);
    // 连接认证方式切换信号
    connect(ui->radioButton_keyAuth, &QRadioButton::toggled, this, &SSHClientWidget::onAuthModeChanged);
    connect(ui->radioButton_passwordAuth, &QRadioButton::toggled, this, &SSHClientWidget::onAuthModeChanged);
    // 初始化UI状态
    updateUIState(false);
    updateAuthUIState();
    findAndSetSshKeyPath();
}

SSHClientWidget::~SSHClientWidget()
{
    // 断开连接
    if(m_sshClient && m_sshClient->isConnected())
    {
        m_sshClient->disconnect();
    }
    delete ui;
}

void SSHClientWidget::findAndSetSshKeyPath()
{
    QString sshKeyPath;
    // 根据操作系统选择不同的路径
#ifdef Q_OS_LINUX
    sshKeyPath = "/root/.ssh/id_rsa";
#elif defined(Q_OS_WIN)
    sshKeyPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.ssh/id_rsa";
#else
    // 其他操作系统处理
    return;
#endif
    // 检查文件是否存在
    if(QFile::exists(sshKeyPath))
    {
        ui->keyFileLineEdit->setText(sshKeyPath);
    }
    else
    {
        // 如果默认路径不存在，尝试在目录中查找任何 RSA 密钥文件
        QDir sshDir(QFileInfo(sshKeyPath).absolutePath());
        QStringList keyFiles = sshDir.entryList(QStringList() << "id_rsa*", QDir::Files);
        if(!keyFiles.isEmpty())
        {
            // 使用找到的第一个 RSA 密钥文件
            ui->keyFileLineEdit->setText(sshDir.absoluteFilePath(keyFiles.first()));
        }
        else
        {
            // 没有找到密钥文件
            ui->keyFileLineEdit->clear();
        }
    }
}

void SSHClientWidget::on_connectButton_clicked()
{
    // 获取连接参数
    QString host = ui->hostLineEdit->text().trimmed();
    QString portStr = ui->portLineEdit->text().trimmed();
    QString username = ui->usernameLineEdit->text().trimmed();
    // 验证基本输入
    if(host.isEmpty() || username.isEmpty())
    {
        QMessageBox::warning(this, "连接错误", "请填写主机地址和用户名！");
        return;
    }
    // 解析端口
    bool ok;
    int port = portStr.toInt(&ok);
    if(!ok || port <= 0 || port > 65535)
    {
        QMessageBox::warning(this, "连接错误", "端口号无效，请输入1-65535之间的数字！");
        return;
    }
    // 更新UI状态
    ui->connectButton->setEnabled(false);
    appendOutput("", false, false, false); // 空行
    // 根据认证方式进行连接
    if(ui->radioButton_passwordAuth->isChecked())
    {
        // 密码认证
        QString password = ui->passwdEdit->text();
        if(password.isEmpty())
        {
            QMessageBox::warning(this, "连接错误", "请输入密码！");
            ui->connectButton->setEnabled(true);
            return;
        }
        appendOutput("▶ 正在连接到 " + host + ":" + portStr + " (密码认证)...", false, false, true);
        appendOutput("", false, false, false); // 空行
        // 尝试密码连接
        m_sshClient->connectToHostWithPassword(host, port, username, password);
    }
    else
    {
        // 密钥认证
        QString keyFilePath = ui->keyFileLineEdit->text().trimmed();
        QString keyPassword = ui->keyPasswordLineEdit->text();
        if(keyFilePath.isEmpty())
        {
            QMessageBox::warning(this, "连接错误", "请选择SSH密钥文件！");
            ui->connectButton->setEnabled(true);
            return;
        }
        appendOutput("▶ 正在连接到 " + host + ":" + portStr + " (密钥认证)...", false, false, true);
        appendOutput("", false, false, false); // 空行
        // 尝试密钥连接
        m_sshClient->connectToHost(host, port, username, keyFilePath, keyPassword);
    }
}

void SSHClientWidget::on_disconnectButton_clicked()
{
    if(m_sshClient && m_sshClient->isConnected())
    {
        ui->disconnectButton->setEnabled(false);
        appendOutput("", false, false, false); // 空行
        appendOutput("▶ 正在断开连接...", false, false, true);
        appendOutput("", false, false, false); // 空行
        m_sshClient->disconnect();
    }
}

void SSHClientWidget::on_executeButton_clicked()
{
    onCommandEntered();
}

void SSHClientWidget::on_checkNetworkButton_clicked()
{
    QString host = ui->hostLineEdit->text().trimmed();
    if(host.isEmpty())
    {
        QMessageBox::warning(this, "网络检测", "请输入主机地址！");
        return;
    }
    // 获取端口号
    bool ok;
    int port = ui->portLineEdit->text().toInt(&ok);
    if(!ok || port <= 0 || port > 65535)
    {
        QMessageBox::warning(this, "端口错误", "请输入有效的端口号(1-65535)");
        return;
    }
    ui->checkNetworkButton->setEnabled(false);
    appendOutput("", false, false, false); // 空行
    appendOutput(QString("▶ 正在检测到 %1:%2 的网络连接...").arg(host).arg(port), false, false, true);
    appendOutput("", false, false, false); // 空行
    m_sshClient->checkNetworkConnection(host, port);
}

void SSHClientWidget::on_browseKeyFileButton_clicked()
{
    QString keyFilePath = QFileDialog::getOpenFileName(this, "选择SSH密钥文件", QDir::homePath(), "所有文件 (*.*)");
    if(!keyFilePath.isEmpty())
    {
        ui->keyFileLineEdit->setText(keyFilePath);
    }
}

void SSHClientWidget::on_generateKeyButton_clicked()
{
    // 创建密钥生成对话框
    KeyGenDialog dialog(this);
    // 显示对话框
    if(dialog.exec() == QDialog::Accepted)
    {
        // 获取用户输入
        QString email = dialog.getEmail();
        QString keyType = dialog.getKeyType();
        int keyBits = dialog.getKeyBits();
        QString password = dialog.getPassword();
        // 构建命令
        QStringList args;
        QString sshKeygenCmd;
#ifdef Q_OS_WIN
        // Windows平台使用PowerShell执行ssh-keygen
        sshKeygenCmd = "powershell.exe";
        // 添加PowerShell启动参数，禁用配置文件加载和允许脚本执行
        args << "-NoProfile" << "-ExecutionPolicy" << "Bypass";
        // 检查密钥文件是否已存在
        QString keyPath = QDir::homePath() + "/.ssh/id_" + keyType.toLower();
        QString pubKeyPath = keyPath + ".pub";
        if(QFile::exists(keyPath) || QFile::exists(pubKeyPath))
        {
            // 弹窗询问用户是否删除现有密钥文件
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(this, "密钥文件已存在",
                                          "检测到已存在的SSH密钥文件，是否删除并重新生成？",
                                          QMessageBox::Yes | QMessageBox::No);
            if(reply == QMessageBox::No)
            {
                // 用户选择不删除，直接返回
                return;
            }
            // 用户选择删除，先删除现有文件
            if(QFile::exists(keyPath))
            {
                if(!QFile::remove(keyPath))
                {
                    QMessageBox::critical(this, "错误", "无法删除现有的私钥文件: " + keyPath);
                    return;
                }
                qInfo() << "已删除私钥文件: " << keyPath;
                appendOutput("已删除私钥文件: " + keyPath, false, false, true);
            }
            if(QFile::exists(pubKeyPath))
            {
                if(!QFile::remove(pubKeyPath))
                {
                    QMessageBox::critical(this, "错误", "无法删除现有的公钥文件: " + pubKeyPath);
                    return;
                }
                qInfo() << "已删除公钥文件: " << pubKeyPath;
                appendOutput("已删除公钥文件: " + pubKeyPath, false, false, true);
            }
        }
        // 构建完整的ssh-keygen命令，确保.ssh目录存在
        QString sshKeygenArgs = "& {";
        // 首先确保.ssh目录存在
        sshKeygenArgs += "if (!(Test-Path $env:USERPROFILE\\.ssh)) { New-Item -ItemType Directory -Path $env:USERPROFILE\\.ssh -Force | Out-Null }; ";
        // 执行ssh-keygen命令，使用正确的-N参数传递密码
        sshKeygenArgs += "ssh-keygen";
        // 添加密钥类型
        sshKeygenArgs += QString(" -t %1").arg(keyType);
        // 如果是RSA类型，添加密钥长度
        if(keyType.toLower() == "rsa")
        {
            sshKeygenArgs += QString(" -b %1").arg(keyBits);
        }
        // 如果有邮箱，添加注释
        if(!email.isEmpty())
        {
            sshKeygenArgs += QString(" -C \"%1\"").arg(email);
        }
        // 使用默认路径
        sshKeygenArgs += " -f $env:USERPROFILE\\.ssh\\id_" + keyType.toLower();
        // 添加密码参数（使用正确的转义方式）
        if(!password.isEmpty())
        {
            // 有密码时，使用双重转义确保PowerShell正确解析
            sshKeygenArgs += QString(" -N '\"%1\"'").arg(password);
        }
        else
        {
            // 无密码时，使用空字符串的正确转义方式
            sshKeygenArgs += " -N '\"\"'";
        }
        // 添加强制覆盖参数，避免交互式确认
        sshKeygenArgs += " -q";
        // 关闭PowerShell脚本块
        sshKeygenArgs += " }";
        args << "-Command" << sshKeygenArgs;
        qDebug() << "sshKeygenArgs: " << sshKeygenArgs;
#else
        // Linux/macOS平台直接使用ssh-keygen
        sshKeygenCmd = "ssh-keygen";
        // 首先确保.ssh目录存在
        QString sshDirPath = QDir::homePath() + "/.ssh";
        QDir sshDir(sshDirPath);
        if(!sshDir.exists())
        {
            if(!sshDir.mkpath(sshDirPath))
            {
                QMessageBox::critical(this, "错误", "无法创建.ssh目录: " + sshDirPath);
                ui->generateKeyButton->setEnabled(true);
                return;
            }
            // 设置.ssh目录权限为700（仅用户可读写执行）
            QFile::setPermissions(sshDirPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
        }
        // 构建ssh-keygen参数
        args << "-t" << keyType.toLower(); // 确保密钥类型为小写
        if(keyType.toLower() == "rsa")
        {
            args << "-b" << QString::number(keyBits);
        }
        if(!email.isEmpty())
        {
            args << "-C" << email;
        }
        // 使用默认路径（密钥类型小写）
        args << "-f" << QDir::homePath() + "/.ssh/id_" + keyType.toLower();
        // 添加密码参数
        if(!password.isEmpty())
        {
            args << "-N" << password;
        }
        else
        {
            args << "-N" << ""; // 空密码
        }
        // 如果密钥文件已存在，先删除以避免交互式确认
        QString keyPath = QDir::homePath() + "/.ssh/id_" + keyType.toLower();
        QString pubKeyPath = keyPath + ".pub";
        if(QFile::exists(keyPath))
        {
            QFile::remove(keyPath);
            qDebug() << "删除已存在的私钥文件: " << keyPath;
        }
        if(QFile::exists(pubKeyPath))
        {
            QFile::remove(pubKeyPath);
            qDebug() << "删除已存在的公钥文件: " << pubKeyPath;
        }
        // 添加静默参数，避免交互式确认
        args << "-q";
#endif
        // 禁用生成按钮，防止重复点击
        ui->generateKeyButton->setEnabled(false);
        // 显示正在生成的消息
        appendOutput("", false, false, false); // 空行
        appendOutput("▶ 正在生成SSH密钥，请稍候...", false, false, true);
        appendOutput("", false, false, false); // 空行
        qDebug() << "sshKeygenCmd: " << sshKeygenCmd;
        qDebug() << "args: " << args;
        // 记录完整的命令到日志（隐藏密码信息）
        QString logCommand = args.join(" ");
        if(!password.isEmpty())
        {
            logCommand = logCommand.replace(password, "[HIDDEN]");
        }
        qDebug() << "执行SSH密钥生成命令: " << sshKeygenCmd << logCommand;
        // 启动进程
        m_keyGenProcess->start(sshKeygenCmd, args);
        if(!m_keyGenProcess->waitForStarted())
        {
            qCritical() << "无法启动ssh-keygen进程";
            QMessageBox::critical(this, "错误", "无法启动ssh-keygen进程。请检查OpenSSH是否已安装。");
            ui->generateKeyButton->setEnabled(true);
            return;
        }
        qInfo() << "ssh-keygen进程已启动，等待完成...";
        // 设置进程超时（30秒）
        QTimer::singleShot(30000, this, [this]()
        {
            if(m_keyGenProcess && m_keyGenProcess->state() == QProcess::Running)
            {
                qWarning() << "SSH密钥生成超时，终止进程";
                m_keyGenProcess->kill();
                QMessageBox::warning(this, "警告", "SSH密钥生成超时，请重试。");
                ui->generateKeyButton->setEnabled(true);
            }
        });
    }
}

void SSHClientWidget::onKeyGenFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    // 重新启用生成按钮
    ui->generateKeyButton->setEnabled(true);
    // 读取命令输出
    QString stdOutput = QString::fromLocal8Bit(m_keyGenProcess->readAllStandardOutput());
    QString stdError = QString::fromLocal8Bit(m_keyGenProcess->readAllStandardError());
    qDebug() << "ssh-keygen命令完成，退出码：" << exitCode << "，退出状态：" << exitStatus;
    qDebug() << "标准输出：" << stdOutput;
    qDebug() << "标准错误：" << stdError;
    qDebug() << "SSH密钥生成命令：" << m_keyGenProcess->program() << m_keyGenProcess->arguments().join(" ");
    // 首先检查进程是否正常退出
    if(exitStatus != QProcess::NormalExit || exitCode != 0)
    {
        // 生成密钥失败
        QString error = QString::fromLocal8Bit(m_keyGenProcess->readAllStandardError());
        appendOutput("", false, false, false); // 空行
        appendOutput("▶ SSH密钥生成失败！", true, false, true);
        appendOutput("", false, false, false); // 空行
        appendOutput(error, true, false, false);
        QMessageBox::critical(this, "密钥生成失败", "SSH密钥生成失败，请检查错误信息：\n\n" + error);
        return;
    }
    // 进程正常退出，但需要验证密钥文件是否真正生成
    qDebug() << "进程正常退出，开始验证密钥文件是否生成...";
    // 获取生成的密钥文件路径
    QString keyFilePath;
    // 从进程参数中提取密钥类型
    QString keyType = "rsa"; // 默认值
    QStringList processArgs = m_keyGenProcess->arguments();
#ifdef Q_OS_WIN
    // Windows平台：从PowerShell命令字符串中提取密钥类型
    if(!processArgs.isEmpty())
    {
        QString fullCommand = processArgs.join(" ");
        QRegularExpression re(R"(-t\s+(\w+))");
        QRegularExpressionMatch match = re.match(fullCommand);
        if(match.hasMatch())
        {
            keyType = match.captured(1);
        }
    }
#else
    // Linux/macOS平台：从参数列表中查找 -t 参数
    for(int i = 0; i < processArgs.size() - 1; i++)
    {
        if(processArgs[i] == "-t")
        {
            keyType = processArgs[i + 1];
            break;
        }
    }
#endif
    qDebug() << "检测到的密钥类型：" << keyType;
#ifdef Q_OS_WIN
    // Windows平台可能生成大写或小写的文件名，需要检查多种可能的路径
    QStringList possiblePaths =
    {
        QDir::homePath() + "/.ssh/id_" + keyType.toLower(),
        QDir::homePath() + "/.ssh/id_" + keyType.toUpper(),
        QDir::homePath() + "\\.ssh\\id_" + keyType.toLower(),
        QDir::homePath() + "\\.ssh\\id_" + keyType.toUpper()
    };
    for(const QString &path : possiblePaths)
    {
        if(QFile::exists(path))
        {
            keyFilePath = path;
            qDebug() << "找到密钥文件：" << keyFilePath;
            break;
        }
    }
    // 如果还没找到，尝试列出.ssh目录中的所有文件进行匹配
    if(keyFilePath.isEmpty())
    {
        QDir sshDir(QDir::homePath() + "/.ssh");
        if(!sshDir.exists())
        {
            sshDir = QDir(QDir::homePath() + "\\.ssh");
        }
        if(sshDir.exists())
        {
            QStringList files = sshDir.entryList(QDir::Files);
            QString targetFileName = "id_" + keyType.toLower();
            for(const QString &file : files)
            {
                if(file.toLower() == targetFileName || file.toLower() == targetFileName.toUpper())
                {
                    keyFilePath = sshDir.absoluteFilePath(file);
                    qDebug() << "通过目录扫描找到密钥文件：" << keyFilePath;
                    break;
                }
            }
        }
    }
#else
    keyFilePath = QDir::homePath() + "/.ssh/id_" + keyType.toLower();
#endif
    // 验证私钥文件是否存在
    bool privateKeyExists = QFile::exists(keyFilePath);
    QString pubKeyPath = keyFilePath + ".pub";
    bool publicKeyExists = QFile::exists(pubKeyPath);
    qDebug() << "密钥文件验证结果：";
    qDebug() << "私钥文件 (" << keyFilePath << ") 存在：" << privateKeyExists;
    qDebug() << "公钥文件 (" << pubKeyPath << ") 存在：" << publicKeyExists;
    // 如果密钥文件不存在，则认为生成失败
    if(!privateKeyExists || !publicKeyExists)
    {
        // 列出.ssh目录中的所有文件以便调试
        QDir sshDir(QDir::homePath() + "/.ssh");
        QStringList files = sshDir.entryList(QDir::Files);
        QString errorMsg = "密钥文件生成失败：\n";
        if(!privateKeyExists)
        {
            errorMsg += "- 私钥文件不存在: " + keyFilePath + "\n";
        }
        if(!publicKeyExists)
        {
            errorMsg += "- 公钥文件不存在: " + pubKeyPath + "\n";
        }
        errorMsg += "\n.ssh目录中的文件: " + files.join(", ");
        errorMsg += "\n\n可能的解决方案：";
        errorMsg += "\n1. 确保已安装OpenSSH客户端";
        errorMsg += "\n2. 检查.ssh目录权限";
        errorMsg += "\n3. 尝试手动创建.ssh目录";
        errorMsg += "\n4. 检查防病毒软件是否阻止文件创建";
        // 添加进程输出信息
        QString processOutput = QString::fromLocal8Bit(m_keyGenProcess->readAllStandardOutput());
        QString processError = QString::fromLocal8Bit(m_keyGenProcess->readAllStandardError());
        if(!processOutput.isEmpty())
        {
            errorMsg += "\n\n进程输出: " + processOutput;
        }
        if(!processError.isEmpty())
        {
            errorMsg += "\n\n进程错误: " + processError;
        }
        appendOutput("", false, false, false); // 空行
        appendOutput("▶ SSH密钥生成失败：密钥文件未找到", true, false, true);
        appendOutput("", false, false, false); // 空行
        appendOutput(errorMsg, true, false, false);
        qWarning() << "密钥生成失败：" << errorMsg;
        QMessageBox::critical(this, "密钥生成失败", errorMsg);
        return;
    }
    // 密钥文件确实存在，生成成功
    appendOutput("", false, false, false); // 空行
    appendOutput("▶ SSH密钥生成成功！", false, false, true);
    appendOutput("", false, false, false); // 空行
    qDebug() << "SSH密钥生成成功，私钥：" << keyFilePath << "，公钥：" << pubKeyPath;
    // 自动填入密钥文件路径
    ui->keyFileLineEdit->setText(keyFilePath);
    // 读取并显示公钥内容
    QFile pubKeyFile(pubKeyPath);
    if(pubKeyFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QByteArray fileContent = pubKeyFile.readAll();
        QString pubKey = QString::fromUtf8(fileContent).trimmed();
        pubKeyFile.close();
        qDebug() << "成功读取公钥内容，长度：" << pubKey.length() << "，内容：" << pubKey.left(30) + "...";
        // 显示公钥内容和使用指南
        appendOutput("", false, false, false); // 空行
        appendOutput("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━", false, false, true);
        appendOutput("                                SSH公钥                                 ", false, false, true);
        appendOutput("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━", false, false, true);
        appendOutput("", false, false, false); // 空行
        appendOutput("请将以下公钥内容添加到远程服务器的authorized_keys文件中：", false, false, true);
        appendOutput("", false, false, false); // 空行
        appendOutput(pubKey, false, false, false);
        appendOutput("", false, false, false); // 空行
        appendOutput("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━", false, false, true);
        appendOutput("                              SSH密钥使用指南                              ", false, false, true);
        appendOutput("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━", false, false, true);
        appendOutput("", false, false, false); // 空行
        appendOutput("【1】将公钥添加到远程服务器：", false, false, true);
        appendOutput("", false, false, false); // 空行
        appendOutput("   ■ Linux/macOS服务器：", false, false, false);
        appendOutput("     a) 登录到服务器:    ssh 用户名@服务器地址", false, false, false);
        appendOutput("     b) 创建.ssh目录:    mkdir -p ~/.ssh", false, false, false);
        appendOutput("     c) 设置权限:        chmod 700 ~/.ssh", false, false, false);
        appendOutput("     d) 编辑密钥文件:    nano ~/.ssh/authorized_keys", false, false, false);
        appendOutput("     e) 粘贴上面的公钥内容，保存并退出", false, false, false);
        appendOutput("     f) 设置权限:        chmod 600 ~/.ssh/authorized_keys", false, false, false);
        appendOutput("", false, false, false); // 空行
        appendOutput("   ■ Windows服务器（OpenSSH）：", false, false, false);
        appendOutput("     a) 确保服务器已安装OpenSSH服务器", false, false, false);
        appendOutput("     b) 将公钥内容添加到 C:\\ProgramData\\ssh\\administrators_authorized_keys 文件中", false, false, false);
        appendOutput("     c) 设置适当的文件权限", false, false, false);
        appendOutput("", false, false, false); // 空行
        appendOutput("【2】使用SSH密钥连接：", false, false, true);
        appendOutput("", false, false, false); // 空行
        appendOutput("   ■ 命令行方式：ssh -i \"" + keyFilePath + "\" 用户名@服务器地址", false, false, false);
        appendOutput("   ■ 或使用本客户端，在连接设置中指定私钥文件", false, false, false);
        appendOutput("", false, false, false); // 空行
        appendOutput("【3】密钥安全提示：", false, false, true);
        appendOutput("", false, false, false); // 空行
        appendOutput("   ■ 请妥善保管私钥文件，不要分享给他人", false, false, false);
        appendOutput("   ■ 建议为私钥设置密码保护", false, false, false);
        appendOutput("   ■ 如果私钥泄露，请立即在所有服务器上删除对应的公钥", false, false, false);
        appendOutput("", false, false, false); // 空行
        appendOutput("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━", false, false, true);
#ifdef Q_OS_LINUX
        //linux部署秘钥文件到远程服务器
        appendOutput("开始部署密钥文件到远程服务器", false, false, true);
        if(deployKey())
        {
            appendOutput("密钥文件部署到远程服务器成功", false, false, true);
        }
        else
        {
            appendOutput("部署密钥文件到远程服务器失败", true, false, true);
        }
#endif
    }
    else
    {
        QString errorString = pubKeyFile.errorString();
        qWarning() << "无法打开公钥文件：" << errorString;
        appendOutput("\n无法读取公钥文件：" + errorString, true, false, true);
    }
}

void SSHClientWidget::onKeyGenError(QProcess::ProcessError error)
{
    // 重新启用生成按钮
    ui->generateKeyButton->setEnabled(true);
    // 获取进程状态信息
    QString program = m_keyGenProcess->program();
    QStringList arguments = m_keyGenProcess->arguments();
    qint64 pid = m_keyGenProcess->processId();
    qDebug() << "SSH密钥生成进程错误：" << error;
    qDebug() << "进程信息 - 程序：" << program << "，参数：" << arguments.join(" ") << "，PID：" << pid;
    QString errorMessage;
    QString suggestion;
    switch(error)
    {
        case QProcess::FailedToStart:
            errorMessage = "无法启动ssh-keygen程序，请确保已安装OpenSSH客户端。";
            suggestion = "请检查：\n1. OpenSSH是否已正确安装\n2. ssh-keygen命令是否在系统PATH中\n3. PowerShell是否可用";
            qDebug() << "错误详情：程序可能不存在或没有执行权限";
            break;
        case QProcess::Crashed:
            errorMessage = "ssh-keygen程序意外崩溃。";
            suggestion = "请检查：\n1. 密钥文件路径是否有效\n2. 磁盘空间是否充足\n3. 是否有足够的权限";
            qDebug() << "错误详情：进程在运行过程中崩溃";
            break;
        case QProcess::Timedout:
            errorMessage = "ssh-keygen程序执行超时。";
            suggestion = "请检查：\n1. 系统负载是否过高\n2. 密钥长度是否过大\n3. 重试生成密钥";
            qDebug() << "错误详情：进程执行超时";
            break;
        case QProcess::WriteError:
            errorMessage = "向ssh-keygen程序写入数据时发生错误。";
            suggestion = "请检查系统资源是否充足";
            qDebug() << "错误详情：向进程写入数据失败";
            break;
        case QProcess::ReadError:
            errorMessage = "从ssh-keygen程序读取数据时发生错误。";
            suggestion = "请检查系统资源是否充足";
            qDebug() << "错误详情：从进程读取数据失败";
            break;
        default:
            errorMessage = "执行ssh-keygen程序时发生未知错误。";
            suggestion = "请重试或联系技术支持";
            qDebug() << "错误详情：未知错误类型" << error;
            break;
    }
    // 尝试读取任何可能的输出
    QString stdOutput = QString::fromLocal8Bit(m_keyGenProcess->readAllStandardOutput());
    QString stdError = QString::fromLocal8Bit(m_keyGenProcess->readAllStandardError());
    qDebug() << "进程标准输出：" << stdOutput;
    qDebug() << "进程标准错误：" << stdError;
    // 分析常见错误并提供具体建议
    if(stdError.contains("option requires an argument"))
    {
        suggestion = "命令参数错误，这可能是程序bug，请联系开发者";
    }
    else if(stdError.contains("No such file or directory"))
    {
        suggestion = "文件路径不存在，请检查目标目录是否有效";
    }
    else if(stdError.contains("Permission denied"))
    {
#ifdef Q_OS_WIN
        suggestion = "权限不足，请以管理员身份运行程序或检查目录权限";
#else
        suggestion = "权限不足，请检查：\n1. .ssh目录权限（应为700）\n2. 是否有写入权限\n3. 尝试: chmod 700 ~/.ssh";
#endif
    }
    else if(stdError.contains("Saving key") && stdError.contains("failed"))
    {
        suggestion = "密钥保存失败，请检查：\n1. 磁盘空间是否充足\n2. .ssh目录权限\n3. 文件系统是否只读";
    }
    else if(stdError.contains("Invalid key type"))
    {
        suggestion = "不支持的密钥类型，请选择RSA、DSA、ECDSA或ED25519";
    }
    else if(stdError.contains("key size"))
    {
        suggestion = "密钥长度无效，RSA密钥建议使用2048或4096位";
    }
#ifndef Q_OS_WIN
    // Linux/macOS特定错误处理
    if(stdError.contains("ssh-keygen: command not found"))
    {
        suggestion = "ssh-keygen未安装，请安装OpenSSH：\n• Ubuntu/Debian: sudo apt-get install openssh-client\n• CentOS/RHEL: sudo yum install openssh-clients\n• macOS: 通常已预装，检查PATH环境变量";
    }
    else if(stdError.contains("Bad owner or permissions"))
    {
        suggestion = "文件权限错误，请执行：\nchmod 700 ~/.ssh\nchmod 600 ~/.ssh/id_*";
    }
    else if(stdError.contains("/dev/urandom"))
    {
        suggestion = "随机数生成器不可用，这可能是系统问题，请重启或联系系统管理员";
    }
#endif
    appendOutput("", false, false, false); // 空行
    appendOutput("▶ SSH密钥生成失败：" + errorMessage, true, false, true);
    appendOutput("", false, false, false); // 空行
    if(!stdError.isEmpty())
    {
        appendOutput("错误详情：" + stdError, true, false, false);
    }
    QMessageBox::critical(this, "密钥生成失败",
                          QString("错误: %1\n\n建议解决方案:\n%2\n\n详细错误信息:\n%3")
                          .arg(errorMessage)
                          .arg(suggestion)
                          .arg(stdError.isEmpty() ? "无详细错误信息" : stdError));
}

void SSHClientWidget::onCommandEntered()
{
    QString command = ui->commandLineEdit->text().trimmed();
    if(command.isEmpty())
    {
        return;
    }
    // 禁用命令输入，直到命令执行完成
    ui->commandLineEdit->setEnabled(false);
    ui->executeButton->setEnabled(false);
    // 显示命令（使用命令格式）
    appendOutput("", false, false, false); // 空行
    appendOutput("$ " + command, false, true, false);
    // 执行命令
    m_sshClient->executeCommand(command);
    // 清空命令输入框
    ui->commandLineEdit->clear();
}

void SSHClientWidget::onConnected()
{
    updateUIState(true);
    appendOutput("", false, false, false); // 空行
    appendOutput("▶ 连接成功！", false, false, true);
    appendOutput("", false, false, false); // 空行
}

void SSHClientWidget::onDisconnected()
{
    updateUIState(false);
    appendOutput("", false, false, false); // 空行
    appendOutput("▶ 已断开连接", false, false, true);
    appendOutput("", false, false, false); // 空行
}

void SSHClientWidget::onConnectionError(const QString &error)
{
    updateUIState(false);
    appendOutput("", false, false, false); // 空行
    appendOutput("▶ 连接错误: " + error, true, false, true);
    appendOutput("", false, false, false); // 空行
    QMessageBox::critical(this, "连接错误", "无法连接到服务器: " + error);
}

void SSHClientWidget::onCommandOutput(const QString &output)
{
    appendOutput(output, false, false, false);
    ui->commandLineEdit->setEnabled(true);
    ui->executeButton->setEnabled(true);
    ui->commandLineEdit->setFocus();
}

void SSHClientWidget::onCommandError(const QString &error)
{
    appendOutput(error, true, false, false);
    ui->commandLineEdit->setEnabled(true);
    ui->executeButton->setEnabled(true);
    ui->commandLineEdit->setFocus();
}

void SSHClientWidget::onNetworkCheckComplete(bool isConnected)
{
    ui->checkNetworkButton->setEnabled(true);
    if(isConnected)
    {
        appendOutput("", false, false, false); // 空行
        appendOutput("▶ 网络连接检测成功：可以连接到目标主机", false, false, true);
        appendOutput("", false, false, false); // 空行
        QMessageBox::information(this, "网络检测", "网络连接正常，可以连接到目标主机。");
    }
    else
    {
        appendOutput("", false, false, false); // 空行
        appendOutput("▶ 网络连接检测失败：无法连接到目标主机", true, false, true);
        appendOutput("", false, false, false); // 空行
        QMessageBox::warning(this, "网络检测", "网络连接异常，无法连接到目标主机。请检查网络设置或主机地址是否正确。");
    }
}

void SSHClientWidget::onNetworkCheckError(const QString &error)
{
    ui->checkNetworkButton->setEnabled(true);
    appendOutput("", false, false, false); // 空行
    appendOutput("▶ 网络检测错误: " + error, true, false, true);
    appendOutput("", false, false, false); // 空行
    QMessageBox::critical(this, "网络检测错误", "网络检测失败: " + error);
}

void SSHClientWidget::updateUIState(bool connected)
{
    // 更新连接相关控件状态
    ui->hostLineEdit->setEnabled(!connected);
    ui->portLineEdit->setEnabled(!connected);
    ui->usernameLineEdit->setEnabled(!connected);
    ui->keyFileLineEdit->setEnabled(!connected);
    ui->keyPasswordLineEdit->setEnabled(!connected);
    ui->browseKeyFileButton->setEnabled(!connected);
    ui->generateKeyButton->setEnabled(!connected);
    ui->checkNetworkButton->setEnabled(!connected);
    ui->connectButton->setEnabled(!connected);
    ui->disconnectButton->setEnabled(connected);
    // 更新命令执行相关控件状态
    ui->commandLineEdit->setEnabled(connected);
    ui->executeButton->setEnabled(connected);
    if(connected)
    {
        ui->commandLineEdit->setFocus();
    }
    else
    {
        ui->hostLineEdit->setFocus();
    }
}

void SSHClientWidget::updateAuthUIState()
{
    // 密钥认证相关控件始终可见
    ui->keyFileLabel->setVisible(true);
    ui->keyFileLineEdit->setVisible(true);
    ui->browseKeyFileButton->setVisible(true);
    ui->keyPasswordLabel->setVisible(true);
    ui->keyPasswordLineEdit->setVisible(true);
    ui->generateKeyButton->setVisible(true);
    // 更新启用状态
    if(!m_sshClient->isConnected())
    {
        ui->keyFileLineEdit->setEnabled(true);
        ui->keyPasswordLineEdit->setEnabled(true);
        ui->browseKeyFileButton->setEnabled(true);
        ui->generateKeyButton->setEnabled(true);
    }
}

void SSHClientWidget::appendOutput(const QString &text, bool isError, bool isCommand, bool isSystem)
{
    // 获取当前时间
    QString timestamp = QDateTime::currentDateTime().toString("[hh:mm:ss.zzz] ");
    // 设置文本颜色
    QTextCharFormat timestampFormat;
    timestampFormat.setForeground(QColor(128, 128, 128)); // 灰色时间戳
    timestampFormat.setFontFamily("Consolas"); // 使用等宽字体
    timestampFormat.setFontPointSize(9); // 设置字体大小
    QTextCharFormat textFormat;
    textFormat.setFontFamily("Consolas"); // 使用等宽字体
    textFormat.setFontPointSize(9); // 设置字体大小
    if(isError)
    {
        textFormat.setForeground(QColor(220, 50, 50)); // 错误信息为红色
    }
    else if(isCommand)
    {
        textFormat.setForeground(QColor(0, 150, 0)); // 命令为绿色
        textFormat.setFontWeight(QFont::Bold); // 加粗命令
    }
    else if(isSystem)
    {
        textFormat.setForeground(QColor(0, 100, 180)); // 系统消息为蓝色
        textFormat.setFontWeight(QFont::Medium); // 系统消息稍微加粗
    }
    else
    {
        textFormat.setForeground(QColor(30, 30, 30)); // 普通输出为深灰色
    }
    // 添加文本到输出区域
    QTextCursor cursor = ui->outputTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    // 先添加时间戳
    cursor.setCharFormat(timestampFormat);
    cursor.insertText(timestamp);
    // 再添加实际文本
    cursor.setCharFormat(textFormat);
    cursor.insertText(text + "\n");
    // 滚动到底部
    ui->outputTextEdit->setTextCursor(cursor);
    ui->outputTextEdit->ensureCursorVisible();
    // 同时记录到日志
    if(isError)
    {
        qWarning() << "UI输出：" << text;
    }
    else if(isCommand)
    {
        qDebug() << "命令输入：" << text;
    }
    else if(isSystem)
    {
        qInfo() << "系统消息：" << text;
    }
    else
    {
        qDebug() << "命令输出：" << text;
    }
}

// 重载原有的appendOutput函数，保持向后兼容
void SSHClientWidget::appendOutput(const QString &text, bool isError)
{
    appendOutput(text, isError, false, false);
}

// Linux检查工具是否可用
bool SSHClientWidget::isToolAvailable(const QString& tool)
{
    QString checkMsg = "[工具检查] 检查工具是否可用: " + tool;
    qDebug() << checkMsg;
    appendOutput(checkMsg);
    
    bool available = false;
#ifdef Q_OS_WIN
    // Windows平台使用where命令
    available = QProcess::execute("where", QStringList() << tool) == 0;
#else
    // Linux平台使用which命令
    available = QProcess::execute("which", QStringList() << tool) == 0;
#endif
    
    if(available)
    {
        QString availableMsg = "[工具检查] 工具 " + tool + " 可用";
        qDebug() << availableMsg;
        appendOutput(availableMsg);
    }
    else
    {
        QString unavailableMsg = "[工具检查] 工具 " + tool + " 不可用";
        qDebug() << unavailableMsg;
        appendOutput(unavailableMsg);
    }
    return available;
}

//从本地压缩包安装sshpass
bool SSHClientWidget::installSshpassFromLocal()
{
    QString startMsg = "[sshpass安装] 开始从本地安装包安装...";
    qDebug() << startMsg;
    appendOutput(startMsg);
    // 1. 检查本地安装包是否存在
    QString localTarPath = QCoreApplication::applicationDirPath() + "/sshpass/sshpass-1.10.tar.gz";
    QString checkMsg = "[sshpass安装] 检查本地安装包: " + localTarPath;
    qDebug() << checkMsg;
    appendOutput(checkMsg);
    if(!QFile::exists(localTarPath))
    {
        QString errorMsg = "[sshpass安装] 错误: 未找到本地安装包";
        qDebug() << errorMsg;
        appendOutput(errorMsg, true);
        QString fileMsg = "请确保存在文件: " + localTarPath;
        qDebug() << fileMsg;
        appendOutput(fileMsg, true);
        return false;
    }
    QString foundMsg = "[sshpass安装] 找到本地安装包";
    qDebug() << foundMsg;
    appendOutput(foundMsg);
    // 2. 创建临时目录
    QTemporaryDir tempDir;
    if(!tempDir.isValid())
    {
        QString errorMsg = "[sshpass安装] 错误: 无法创建临时目录";
        qDebug() << errorMsg;
        appendOutput(errorMsg, true);
        return false;
    }
    QString tempPath = tempDir.path();
    QString tempMsg = "[sshpass安装] 创建临时目录: " + tempPath;
    qDebug() << tempMsg;
    appendOutput(tempMsg);
    // 3. 复制安装包到临时目录
    QString tempTarPath = tempPath + "/sshpass-1.10.tar.gz";
    if(!QFile::copy(localTarPath, tempTarPath))
    {
        QString errorMsg = "[sshpass安装] 错误: 无法复制安装包到临时目录";
        qDebug() << errorMsg;
        appendOutput(errorMsg, true);
        return false;
    }
    QString copyMsg = "[sshpass安装] 已复制安装包到临时目录";
    qDebug() << copyMsg;
    appendOutput(copyMsg);
    // 4. 解压安装包
    QString extractMsg = "[sshpass安装] 开始解压安装包...";
    qDebug() << extractMsg;
    appendOutput(extractMsg);
    QProcess tarProcess;
    tarProcess.setWorkingDirectory(tempPath);
    tarProcess.start("tar", QStringList() << "xvf" << "sshpass-1.10.tar.gz");
    if(!tarProcess.waitForFinished(30000))    // 30秒超时
    {
        QString errorMsg = "[sshpass安装] 错误: 解压超时";
        qDebug() << errorMsg;
        appendOutput(errorMsg, true);
        return false;
    }
    if(tarProcess.exitCode() != 0)
    {
        QString errorMsg = "[sshpass安装] 错误: 解压失败，返回码: " + QString::number(tarProcess.exitCode());
        qDebug() << errorMsg;
        appendOutput(errorMsg, true);
        QString errorOutput = tarProcess.readAllStandardError();
        if(!errorOutput.isEmpty())
        {
            QString outputMsg = "错误输出: " + errorOutput;
            qDebug() << outputMsg;
            appendOutput(outputMsg, true);
        }
        return false;
    }
    QString extractDoneMsg = "[sshpass安装] 解压完成";
    qDebug() << extractDoneMsg;
    appendOutput(extractDoneMsg);
    // 5. 进入解压目录
    QString sourceDir = tempPath + "/sshpass-1.10";
    QString sourceDirMsg = "[sshpass安装] 进入源码目录: " + sourceDir;
    qDebug() << sourceDirMsg;
    appendOutput(sourceDirMsg);
    // 6. 执行configure
    QString configureMsg = "[sshpass安装] 正在执行configure...";
    qDebug() << configureMsg;
    appendOutput(configureMsg);
    QProcess configureProcess;
    configureProcess.setWorkingDirectory(sourceDir);
    configureProcess.start("./configure", QStringList());
    if(!configureProcess.waitForFinished(60000))    // 60秒超时
    {
        QString errorMsg = "[sshpass安装] 错误: configure超时";
        qDebug() << errorMsg;
        appendOutput(errorMsg, true);
        return false;
    }
    if(configureProcess.exitCode() != 0)
    {
        QString errorMsg = "[sshpass安装] 错误: configure失败，返回码: " + QString::number(configureProcess.exitCode());
        qDebug() << errorMsg;
        appendOutput(errorMsg, true);
        QString errorOutput = configureProcess.readAllStandardError();
        if(!errorOutput.isEmpty())
        {
            QString outputMsg = "错误输出: " + errorOutput;
            qDebug() << outputMsg;
            appendOutput(outputMsg, true);
        }
        return false;
    }
    QString configureDoneMsg = "[sshpass安装] configure完成";
    qDebug() << configureDoneMsg;
    appendOutput(configureDoneMsg);
    // 7. 执行make
    QString makeMsg = "[sshpass安装] 正在编译(make)...";
    qDebug() << makeMsg;
    appendOutput(makeMsg);
    QProcess makeProcess;
    makeProcess.setWorkingDirectory(sourceDir);
    makeProcess.start("make", QStringList());
    if(!makeProcess.waitForFinished(300000))    // 5分钟超时
    {
        QString errorMsg = "[sshpass安装] 错误: 编译超时";
        qDebug() << errorMsg;
        appendOutput(errorMsg, true);
        return false;
    }
    if(makeProcess.exitCode() != 0)
    {
        QString errorMsg = "[sshpass安装] 错误: 编译失败，返回码: " + QString::number(makeProcess.exitCode());
        qDebug() << errorMsg;
        appendOutput(errorMsg, true);
        QString errorOutput = makeProcess.readAllStandardError();
        if(!errorOutput.isEmpty())
        {
            QString outputMsg = "错误输出: " + errorOutput;
            qDebug() << outputMsg;
            appendOutput(outputMsg, true);
        }
        return false;
    }
    QString makeDoneMsg = "[sshpass安装] 编译完成";
    qDebug() << makeDoneMsg;
    appendOutput(makeDoneMsg);
    // 8. 执行make install
    QString installMsg = "[sshpass安装] 正在安装(sudo make install)...";
    qDebug() << installMsg;
    appendOutput(installMsg);
    QProcess installProcess;
    installProcess.setWorkingDirectory(sourceDir);
    installProcess.start("sudo", QStringList() << "make" << "install");
    if(!installProcess.waitForFinished(60000))    // 60秒超时
    {
        QString errorMsg = "[sshpass安装] 错误: 安装超时";
        qDebug() << errorMsg;
        appendOutput(errorMsg, true);
        return false;
    }
    if(installProcess.exitCode() != 0)
    {
        QString errorMsg = "[sshpass安装] 错误: 安装失败，返回码: " + QString::number(installProcess.exitCode());
        qDebug() << errorMsg;
        appendOutput(errorMsg, true);
        QString errorOutput = installProcess.readAllStandardError();
        if(!errorOutput.isEmpty())
        {
            QString outputMsg = "错误输出: " + errorOutput;
            qDebug() << outputMsg;
            appendOutput(outputMsg, true);
        }
        return false;
    }
    QString installDoneMsg = "[sshpass安装] 安装完成";
    qDebug() << installDoneMsg;
    appendOutput(installDoneMsg);
    // 9. 验证安装
    QString verifyMsg = "[sshpass安装] 验证安装结果...";
    qDebug() << verifyMsg;
    appendOutput(verifyMsg);
    QProcess whichProcess;
    whichProcess.start("which", QStringList() << "sshpass");
    whichProcess.waitForFinished();
    if(whichProcess.exitCode() != 0)
    {
        QString errorMsg = "[sshpass安装] 错误: 安装验证失败，sshpass未找到";
        qDebug() << errorMsg;
        appendOutput(errorMsg, true);
        return false;
    }
    QString sshpassPath = whichProcess.readAllStandardOutput().trimmed();
    QString successMsg = "[sshpass安装] 安装成功! sshpass路径: " + sshpassPath;
    qDebug() << successMsg;
    appendOutput(successMsg);
    return true;
}

//部署秘钥文件到远程服务器
bool SSHClientWidget::deployKey()
{
    if(ui->passwdEdit->text().isEmpty())
    {
        QString warningMsg = "[公钥部署] 警告: 未提供密码，将尝试无密码部署";
        qDebug() << warningMsg;
        appendOutput(warningMsg);
        return deployKeyWithoutPassword();
    }
    // 尝试sshpass方案
    if(isToolAvailable("sshpass"))
    {
        QString tryMsg = "[公钥部署] 尝试使用sshpass方案...";
        qDebug() << tryMsg;
        appendOutput(tryMsg);
        if(deployKeyWithSshpass())
        {
            return true;
        }
        QString failMsg = "[公钥部署] sshpass方案失败";
        qDebug() << failMsg;
        appendOutput(failMsg, true);
    }
    else
    {
        // 在工具不可用时尝试本地安装
        QString installMsg = "[公钥部署] sshpass未安装，尝试从本地安装包安装...";
        qDebug() << installMsg;
        appendOutput(installMsg);
        if(installSshpassFromLocal())
        {
            QString continueMsg = "[公钥部署] sshpass安装成功，继续部署...";
            qDebug() << continueMsg;
            appendOutput(continueMsg);
            if(deployKeyWithSshpass())
            {
                return true;
            }
        }
        else
        {
            QString errorMsg = "[公钥部署] 错误: sshpass安装失败";
            qDebug() << errorMsg;
            appendOutput(errorMsg, true);
            return false;
        }
    }
    QString needToolMsg = "[公钥部署] 错误: 需要sshpass工具来自动填充密码";
    qDebug() << needToolMsg;
    appendOutput(needToolMsg, true);
    QString installTipMsg = "请安装以下工具:";
    qDebug() << installTipMsg;
    appendOutput(installTipMsg);
    QString ubuntuMsg = "Ubuntu/Debian: sudo apt-get install sshpass";
    qDebug() << ubuntuMsg;
    appendOutput(ubuntuMsg);
    QString centosMsg = "CentOS/RHEL: sudo yum install sshpass";
    qDebug() << centosMsg;
    appendOutput(centosMsg);
    return false;
}

// sshpass方案
bool SSHClientWidget::deployKeyWithSshpass()
{
    QString password = ui->passwdEdit->text();
    QString keyPath = ui->keyFileLineEdit->text();
    QString user = ui->usernameLineEdit->text();
    QString host = ui->hostLineEdit->text();
    QString startMsg = "[sshpass] 开始使用sshpass部署公钥到 " + user + "@" + host;
    qDebug() << startMsg;
    appendOutput(startMsg);
    QStringList args;
    args << "-p" << password
         << "ssh-copy-id"
         << "-i" << keyPath + ".pub"
         << "-o" << "StrictHostKeyChecking=no"
         << "-f"  // 强制模式
         << user + "@" + host;
    QProcess process;
    process.start("sshpass", args);
    if(!process.waitForFinished(30000))   // 30秒超时
    {
        QString errorMsg = "[sshpass] 错误: 执行超时";
        qDebug() << errorMsg;
        appendOutput(errorMsg, true);
        process.terminate();
        return false;
    }
    int exitCode = process.exitCode();
    if(exitCode == 0)
    {
        QString successMsg = "[sshpass] 公钥部署成功";
        qDebug() << successMsg;
        appendOutput(successMsg);
        return true;
    }
    QString errorMsg = "[sshpass] 错误: 部署失败，返回码: " + QString::number(exitCode);
    qDebug() << errorMsg;
    appendOutput(errorMsg, true);
    QString errorOutput = process.readAllStandardError();
    if(!errorOutput.isEmpty())
    {
        QString outputMsg = "错误输出: " + errorOutput;
        qDebug() << outputMsg;
        appendOutput(outputMsg, true);
    }
    return false;
}

// 无密码部署检查
bool SSHClientWidget::deployKeyWithoutPassword()
{
    QString keyPath = ui->keyFileLineEdit->text();
    QString user = ui->usernameLineEdit->text();
    QString host = ui->hostLineEdit->text();
    QString checkMsg = "[无密码检查] 检查公钥是否已部署到 " + user + "@" + host;
    qDebug() << checkMsg;
    appendOutput(checkMsg);
    QStringList args;
    args << "-o" << "BatchMode=yes"
         << "-o" << "StrictHostKeyChecking=no"
         << user + "@" + host
         << "grep -q $(cut -d' ' -f2 " + keyPath + ".pub) ~/.ssh/authorized_keys 2>/dev/null";
    QProcess process;
    process.start("ssh", args);
    if(!process.waitForFinished(5000))
    {
        QString errorMsg = "[无密码检查] 错误: 执行超时";
        qDebug() << errorMsg;
        appendOutput(errorMsg, true);
        process.terminate();
        return false;
    }
    if(process.exitCode() == 0)
    {
        QString successMsg = "[无密码检查] 公钥已部署";
        qDebug() << successMsg;
        appendOutput(successMsg);
        return true;
    }
    QString errorMsg = "[无密码检查] 错误: 公钥未部署且未提供密码";
    qDebug() << errorMsg;
    appendOutput(errorMsg, true);
    QString manualMsg = "请手动执行: ssh-copy-id -i" + keyPath + ".pub " + user + "@" + host;
    qDebug() << manualMsg;
    appendOutput(manualMsg);
    return false;
}

void SSHClientWidget::onAuthModeChanged()
{
    bool isPasswordAuth = ui->radioButton_passwordAuth->isChecked();
    // 根据认证方式显示/隐藏相关控件
    ui->passwdEdit->setEnabled(isPasswordAuth);
    ui->keyFileLineEdit->setEnabled(!isPasswordAuth);
    ui->keyPasswordLineEdit->setEnabled(!isPasswordAuth);
    ui->browseKeyFileButton->setEnabled(!isPasswordAuth);
    ui->generateKeyButton->setEnabled(!isPasswordAuth);
    // 清空相关输入框（可选）
    if(isPasswordAuth)
    {
        ui->keyFileLineEdit->clear();
        ui->keyPasswordLineEdit->clear();
    }
    else
    {
        ui->passwdEdit->clear();
    }
}

