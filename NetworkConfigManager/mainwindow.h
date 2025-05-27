#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QListWidgetItem>
#include <QDir>
#include <QNetworkInterface>
#include <QProcess>
#include <QRegularExpression>
#include <QSharedMemory>
#include <QCheckBox>
#include <QPointer>
#include <QSettings>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QPropertyAnimation>

#include "Logger.h"
#include "configmanager.h"
#include "floatwindow.h"
#include "networkinterfacemanager.h"

//常量定义
const QString MAIN_WINDOW_TITLE = "IP配置管理器(by:mws)";  //主窗口标题
const QString APP_NAME = "NetworkConfigManager";   //应用名称
const QString posConfigPath = "config/posConfig.ini";

namespace Ui
{
    class MainWindow;
}

/**
 * @brief 主窗口类，负责管理网络配置和用户界面交互
 *
 * 该类继承自QMainWindow，提供网络配置管理、系统托盘集成、
 * 浮动窗口等功能，是应用程序的主控制类。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT  //Qt元对象系统宏

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     */
    explicit MainWindow(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~MainWindow();

protected:
    /**
     * @brief 重写关闭事件处理函数
     * @param event 关闭事件对象
     */
    void closeEvent(QCloseEvent *event) override;

private slots:
    //配置管理相关槽函数
    void onConfigSelected(QListWidgetItem *item);  //配置项被选中
    void onAddConfig();       //添加新配置
    void onUpdateConfig();    //更新配置
    void onDeleteConfig();    //删除配置
    void onApplyConfig();     //应用配置
    void updateConfigList();  //更新配置列表

    //网络接口相关槽函数
    void onInterfaceChanged(int index);  //网络接口选择变化
    void updateInterfaces();            //更新网络接口列表

    //IP方法相关槽函数
    void onIpMethodToggled(bool checked);  //IP获取方式切换(DHCP/静态)

    //浮动窗口相关槽函数
    void toggleFloatWindow(bool visible);  //切换浮动窗口显示状态
    void showFloatWindowMenu(const QPoint &pos);  //显示浮动窗口右键菜单

    //系统托盘相关槽函数
    void setupTrayIcon();  //初始化系统托盘
    void toggleAutostart(bool enabled);  //切换开机自启
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);  //托盘图标被激活

    //快速菜单相关槽函数
    void updateQuickMenu();  //更新快速菜单

    void on_networkInterfaceCombo_currentTextChanged(const QString &arg1);

private:
    Ui::MainWindow* ui;              //UI界面指针
    ConfigManager* m_configManager;  //配置管理器
    QMenu* m_quickMenu;              //快速菜单
    FloatWindow* m_floatWindow;      //浮动窗口
    QSystemTrayIcon* m_trayIcon;     //系统托盘图标
    QSharedMemory m_singleInstanceLock;  //共享内存锁(用于单实例检查)

    //网卡管理控件
    QComboBox* m_networkInterfaceCombo;
    QPushButton* m_enableInterfaceBtn;
    QPushButton* m_disableInterfaceBtn;
    QPushButton* m_refreshInterfacesBtn;

    //状态变量
    bool m_floatVisible;       //浮动窗口是否可见
    bool m_autostart;          //是否开机自启
    QString m_currentConfig;    //当前选中的配置名称
    QString m_currentInterface; //当前选中的网络接口
    bool m_lastConfigSuccess;  //上次配置是否成功
    QLabel* m_statusIndicator; //状态指示灯

    //UI初始化相关方法
    void setupUi();            //初始化UI组件
    void setupConnections();   //建立信号槽连接
    void loadAndApplyStyleSheet(const QString &styleSheetPath);


    //配置操作相关方法
    void clearFields();        //清空表单字段
    bool validateIpConfig(const QVariantMap &config);  //验证IP配置有效性
    QVariantMap getCurrentNetworkConfig(const QString &InterfaceName);  //获取当前网络配置
    QVariantList getAllNetworkConfigs();  //获取所有网络配置
    bool compareConfigs(const QVariantMap &current, const QVariantMap &saved);  //比较两个配置

    //工具方法
    QString getActiveConfigName() const;  //获取活动配置名称
    void saveWindowState();     //保存窗口状态
    void showAdminWarning();    //显示管理员权限警告
    void initializeApplication();  //初始化应用程序
    void disableAdminFunctions();  //禁用需要管理员权限的功能
    QVariantMap getCurrentFormConfig() const;  //获取当前表单配置
    bool isDhcpEnabled(const QString &dhcpOutput);  //检查DHCP是否启用
    void restoreWindowState();
    bool checkSingleInstance();

    //保存悬浮窗坐标
    void saveFloatWindowPosition();
    void loadFloatWindowPosition();
    void initSettings(const QString &posConfigPath);

    //网卡管理槽函数
    void refreshNetworkInterfaces();
    void onEnableInterface();
    void onEnableInterface(const QString &interfaceName);
    void onDisableInterface();
    void onDisableInterface(const QString &interfaceName);
    void updateInterfaceControls();

    void showConfigResult(bool success, const QString &message);


    QSettings* m_settings = nullptr; // 改为指针以便灵活控制
    QString m_posConfigPath;

};

#endif //MAINWINDOW_H
