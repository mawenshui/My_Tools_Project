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
#include <QSpinBox>

#include "tinyxml/tinyxml.h"
#include "sendworker.h"
#include "alldefine.h"
#include "../configmanager.h"
// 新增：倍率设置与帮助对话框
#include "multipliersettingsdialog.h"
#include "multiplierhelpdialog.h"

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
    void on_btnLoopSingle_clicked();         ///<单个循环发送按钮点击事件

    void handleSendFinished();
    void handleLogMessage(QString level, QString message);
    void sendNextLoopCommand();

    void filterComboBoxItems(const QString &filterText);
    /**
     * @brief 打开倍率设置对话框
     */
    void onOpenMultiplierSettings();
    /**
     * @brief 打开倍率帮助对话框
     */
    void onOpenMultiplierHelp();
signals:
    void logMessage(QString level, QString message);
    void startSendCommand(const QString &address, quint16 port,
                          const QString &comname, const QMap<QString, int>& commandIdMap,
                          const QMap<QString, int>& workIdMap, const QMap<QString, double>& timeMap,
                          bool comCrc, bool reCrc, bool comReply, bool havereturn,
                          bool ReReply, bool replycrc, bool returnErr, bool noShake,
                          bool isReplyTimeout, bool isReturnTimeout, bool isReReplyTimeout,
                          int replyTimeoutMs, int returnTimeoutMs, int reReplyTimeoutMs,
                          int flowMode);

private:
    QThread* m_sendThread;
    SendWorker* m_sendWorker;
    ConfigManager* m_flowConfig;  ///< 本地配置管理器（监听倍率热更新）
    double m_mulFast;             ///< 当前快速流程倍率
    double m_mulNormal;           ///< 当前正常流程倍率
    double m_mulChecked;          ///< 当前勾选超时倍率
    
    bool m_isSingleLoopMode;      ///< 是否为单指令循环模式

private:
    Ui::OrderSendWidget* ui;             ///<UI指针
    void getconfig();               ///<从XML配置文件读取配置信息
    void comboset();                ///<初始化下拉框选项

    /**
     * @brief 根据当前选择的指令更新 groupBox_5 三项超时显示
     */
    void updateTimeoutUIForSelection();

    /**
     * @brief 发送完成后恢复当前指令的默认超时显示
     */
    void restoreDefaultTimeoutsForSelection();

    /**
     * @brief 根据勾选框状态启用/禁用三个超时编辑框
     */
    void updateTimeoutEditEnableState();

    // 读取并应用倍率配置到成员与UI
    void loadMultipliersFromConfig();
    void applyMultiplierTextToUI();

    /**
     * @brief 发送期间锁定/解锁界面可编辑控件
     * @param locked true=锁定（禁用倍率、流程模式、超时相关编辑），false=解锁
     */
    void setUILockedForSending(bool locked);

private:

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

    bool m_uiLocked;                ///<发送期间界面锁定标志
};

#endif //WIDGET_H
