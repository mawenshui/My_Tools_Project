#ifndef FAULTALARMWIDGET_H
#define FAULTALARMWIDGET_H

#include <QWidget>
#include <QUdpSocket>
#include <QTimer>
#include <QDateTime>
#include <QMessageBox>
#include <QHostAddress>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTextEdit>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QSplitter>
#include <QTabWidget>
#include <QScrollArea>
#include <QFont>
#include <QSizePolicy>
#include "alldefine.h"

namespace Ui
{
    class FaultAlarmWidget;
}

/**
 * @brief 故障告警数据帧处理类
 * 实现故障告警数据帧的创建、编辑和发送功能
 */
class FaultAlarmWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FaultAlarmWidget(QWidget *parent = nullptr);
    ~FaultAlarmWidget();

    /**
     * @brief 故障告警数据结构
     */
    struct FaultAlarmData
    {
        //帧头信息
        quint8 control = 0x01;              //控制字节
        quint16 dataLength = 0;             //数据域长度
        quint16 topicNumber = 0;            //主题号
        quint8 sourceDevice = 0;            //源设备号
        quint8 destDevice = 0;              //目的设备号
        quint32 timestamp = 0;              //数据产生时刻

        //数据域信息
        quint8 alarmIdentifier = 0x01;      //告警标识符 (0x01:故障码, 0x02:预警码)
        quint16 faultCode = 0;              //故障码/预警码
        quint8 isolationFlag = 0;           //是否需要隔离标志位
        quint8 faultLevel = 0;              //故障等级
        float warningValue = 0.0f;          //数据预警数值
        float warningThreshold = 0.0f;      //数据预警判定阈值
        QString uniqueId = "";              //唯一标识码 (24字节)
    };

private slots:
    /**
     * @brief 发送配置相关槽函数
     */
    void onSendSingleFrame();           //发送单帧数据
    void onStartContinuousSend();       //开始连续发送
    void onStopContinuousSend();        //停止连续发送
    void onClearLog();                  //清空日志

    /**
     * @brief 数据配置相关槽函数
     */
    void onAlarmTypeChanged();          //告警类型改变
    void onGenerateUniqueId();          //生成唯一标识码
    void onUniqueIdSegmentChanged();    //唯一标识码分段输入变化处理
    void onCalculateCheckCode();        //计算校验码
    void onUniqueIdResultChanged();     //唯一标识码结果编辑变化处理
    void onLoadPreset();                //加载预设配置
    void onSavePreset();                //保存预设配置
    void onResetData();                 //重置数据
    void onFormatChanged();             //数值格式改变

    /**
     * @brief 定时器槽函数
     */
    void onSendTimer();                 //定时发送
    void onSaveConfigurationDelayed();  //延迟保存配置

private:
    /**
     * @brief 初始化函数
     */
    void initUI();                      //初始化用户界面
    void initConnections();             //初始化信号槽连接
    void loadStyleSheet();              //加载样式表

    /**
     * @brief UI创建函数
     */
    QWidget* createDataConfigWidget();  //创建数据配置部件
    QWidget* createSendConfigWidget();  //创建发送配置部件
    QWidget* createLogWidget();         //创建日志显示部件

    /**
     * @brief 数据处理函数
     */
    QByteArray buildDataFrame();        //构建数据帧
    quint16 calculateChecksum(const QByteArray &data); //计算校验和
    void updateDataLength();            //更新数据长度
    void updateTimestamp();             //更新时间戳
    QString generateUniqueId();         //生成唯一标识码

    /**
     * @brief UI更新函数
     */
    void updateUI();                    //更新界面显示
    void updateSendButtonStates();      //更新发送按钮状态
    void appendLog(const QString &level, const QString &message); //添加日志

    /**
     * @brief 配置管理函数
     */
    QString getConfigPath();
    void saveConfiguration();           //保存配置
    void loadConfiguration();           //加载配置
    QJsonObject dataToJson() const;     //数据转JSON
    void jsonToData(const QJsonObject &json); //JSON转数据

private:
    Ui::FaultAlarmWidget* ui;

    //网络相关
    QUdpSocket* m_udpSocket;            //UDP套接字
    QTimer* m_sendTimer;                //发送定时器
    QTimer* m_saveTimer;                //配置保存定时器（防抖）

    //数据相关
    FaultAlarmData m_currentData;       //当前数据
    bool m_isContinuousSending;         //是否正在连续发送

    //UI组件指针
    QWidget* m_dataConfigWidget;        //数据配置部分
    QWidget* m_sendConfigWidget;        //发送配置部分
    QWidget* m_logWidget;               //日志显示部分

    //数据配置控件
    QComboBox* m_controlCombo;          //控制位选择
    QComboBox* m_formatCombo;           //数值格式选择
    QComboBox* m_alarmTypeCombo;        //告警类型选择
    QSpinBox* m_faultCodeSpin;          //故障码输入
    QComboBox* m_isolationCombo;        //隔离标志选择
    QComboBox* m_faultLevelCombo;       //故障等级选择
    QDoubleSpinBox* m_warningValueSpin; //预警数值输入
    QDoubleSpinBox* m_warningThresholdSpin; //预警阈值输入
    //唯一标识码分段输入控件
    QLineEdit* m_orgCodeEdit;            //组织机构代码输入 (9位)
    QLineEdit* m_productDateEdit;        //生产日期输入 (8位)
    QLineEdit* m_serialCodeEdit;         //序列码输入 (6位)
    QLineEdit* m_checkCodeEdit;          //校验位输入 (1位)
    QLineEdit* m_uniqueIdResultEdit;     //唯一标识码结果显示和编辑
    QSpinBox* m_topicNumberSpin;        //主题号输入
    QSpinBox* m_sourceDeviceSpin;       //源设备号输入
    QSpinBox* m_destDeviceSpin;         //目的设备号输入

    //发送配置控件
    QLineEdit* m_targetIpEdit;          //目标IP输入
    QSpinBox* m_targetPortSpin;         //目标端口输入
    QSpinBox* m_intervalSpin;           //发送间隔输入
    QPushButton* m_sendSingleBtn;       //发送单帧按钮
    QPushButton* m_startContinuousBtn;  //开始连续发送按钮
    QPushButton* m_stopContinuousBtn;   //停止连续发送按钮

    //日志显示控件
    QTextEdit* m_logTextEdit;           //日志显示区域
    QPushButton* m_clearLogBtn;         //清空日志按钮
};

#endif //FAULTALARMWIDGET_H
