//customdatasender.h
//自定义数据发送功能

#ifndef CUSTOMDATASENDER_H
#define CUSTOMDATASENDER_H

#include <QObject>
#include <QUdpSocket>  //用于UDP通信
#include <QTimer>      //用于定时发送
#include <QTextEdit>   //用于显示日志和输入数据
#include <QLineEdit>   //用于输入地址和间隔时间
#include <QCheckBox>   //用于选择是否循环发送
#include <QPushButton> //用于控制按钮
#include <QMessageBox> //用于显示错误提示
#include <QDateTime>   //用于生成日志时间戳

class CustomDataSender : public QObject
{
    Q_OBJECT
public:
    //构造函数，初始化UI控件和参数
    explicit CustomDataSender(QTextEdit *textEdit,       //数据输入框
                              QLineEdit *addrEdit,       //地址输入框
                              QLineEdit *intervalEdit,  //发送间隔输入框
                              QCheckBox *loopCheck,     //循环发送复选框
                              QPushButton *startBtn,    //开始按钮
                              QPushButton *pauseBtn,    //暂停按钮
                              QPushButton *stopBtn,     //停止按钮
                              QTextEdit *logView,       //日志显示框
                              QObject *parent = nullptr);
    ~CustomDataSender(); //析构函数，清理资源

private slots:
    void onStartClicked();  //开始按钮点击事件
    void onPauseClicked();  //暂停按钮点击事件
    void onStopClicked();   //停止按钮点击事件
    void sendNextFrame();   //定时器触发，发送下一帧数据

private:
    QList<QByteArray> parseFrames() const; //解析输入的十六进制数据帧
    QByteArray hexStringToByteArray(const QString &hexStr) const; //将十六进制字符串转换为字节数组
    void appendLog(const QString &message, bool isError = false); //添加日志到日志显示框

    //UI控件
    QTextEdit *m_textEdit;     //数据输入框
    QLineEdit *m_addrEdit;     //地址输入框
    QLineEdit *m_intervalEdit; //发送间隔输入框
    QCheckBox *m_loopCheck;    //循环发送复选框
    QPushButton *m_startBtn;   //开始按钮
    QPushButton *m_pauseBtn;   //暂停按钮
    QPushButton *m_stopBtn;    //停止按钮
    QTextEdit *m_logView;      //日志显示框

    //UDP与定时器
    QUdpSocket *m_udpSocket;      //UDP Socket，用于发送数据
    QTimer *m_timer;              //定时器，控制发送间隔
    QList<QByteArray> m_frames;   //存储解析后的数据帧
    int m_currentFrameIndex;      //当前发送的数据帧索引
    QString m_currentIp;          //当前目标IP地址
    int m_currentPort;            //当前目标端口
};

#endif //CUSTOMDATASENDER_H
