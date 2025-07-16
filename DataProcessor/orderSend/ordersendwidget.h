#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QUdpSocket>
#include <qthreadpool.h>
#include <QMessageBox>
#include <QHostAddress>
#include <QDateTime>
#include <qthread.h>
#include <QtConcurrent/QtConcurrent>
#include <QMetaType>

#include "tinyxml/tinyxml.h"
#include "sendworker.h"
#include "alldefine.h"

typedef QMap<QString, int> StringIntMap;
typedef QMap<QString, double> StringDoubleMap;
Q_DECLARE_METATYPE(StringIntMap)
Q_DECLARE_METATYPE(StringDoubleMap)


namespace Ui
{
    class OrderSendWidget;
}

/**
 * @brief 主窗口类，继承自QWidget，负责程序的主界面和核心逻辑
 */
class OrderSendWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OrderSendWidget(QWidget *parent = 0);  ///<构造函数
    ~OrderSendWidget();                             ///<析构函数

private slots:
    //按钮点击事件槽函数
    void on_pushButtonstart_clicked();       ///<开始发送按钮点击事件
    void on_pushButtonClearLog_clicked();    ///<清空日志按钮点击事件
    void on_pushButton_clicked();            ///<循环发送按钮点击事件
    void on_pushButtonStop_clicked();        ///<停止循环发送按钮点击事件

    void handleSendFinished();
    void handleLogMessage(QString level, QString message);
    void sendNextLoopCommand();

    void filterComboBoxItems(const QString &filterText);
signals:
    void logMessage(QString level, QString message);
    void startSendCommand(const QString &address, quint16 port,
                          const QString &comname, const QMap<QString, int>& commandIdMap,
                          const QMap<QString, int>& workIdMap, const QMap<QString, double>& timeMap,
                          bool comCrc, bool reCrc, bool comReply, bool havereturn,
                          bool ReReply, bool replycrc, bool returnErr, bool noShake,
                          bool isReplyTimeout, bool isReturnTimeout, bool isReReplyTimeout);

private:
    QThread* m_sendThread;
    SendWorker* m_sendWorker;

private:
    Ui::OrderSendWidget* ui;             ///<UI指针
    void getconfig();               ///<从XML配置文件读取配置信息
    void comboset();                ///<初始化下拉框选项

    //配置数据存储容器
    QMap<QString, int> m_commandId; ///<存储任务名称对应的命令ID
    QMap<QString, int> m_workId;    ///<存储任务名称对应的工作ID
    QMap<QString, double> m_time;   ///<存储任务名称对应的时间间隔

    bool runed;                     ///<循环发送运行标志
    bool stopRequested;             ///<停止请求标志

    int m_currentLoopIndex;
    bool m_loopRunning;
    void appendLogToView(const QString &html);
    QColor colorForLevel(const QString &level) const;
    ThemeColors m_logColors; //日志颜色配置

};

#endif //WIDGET_H
