#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QThread>
#include <QMutex>
#include <QProgressDialog>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <QLoggingCategory>
#include <QApplication>
#include <QElapsedTimer>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QComboBox>
#include <QToolBar>

#include "alldefine.h"
#include "workerclass.h"
#include "configmanager.h"
#include "customdatasender.h"
#include "orderSend/ordersendwidget.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

/**
 * @brief 主窗口类
 * 负责管理用户界面、配置、日志和工作线程。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    //退出前清理资源
    void cleanupBeforeExit();

protected:
    //重写关闭事件
    void closeEvent(QCloseEvent *event) override;

private slots:
    //按钮点击事件槽函数
    void onBrowseClicked();          //浏览目录
    void onAddAddressClicked();      //添加地址
    void onRemoveAddressClicked();   //移除地址
    void onStartClicked();           //开始处理
    void onPauseClicked();           //暂停处理
    void onStopClicked();            //停止处理

    //处理日志、进度和统计信息的槽函数
    void handleLog(const QString &level, const QString &msg); //处理日志
    void handleProgress(int value);                           //处理进度
    void handleStats(int success, int failed);                //处理统计信息

    //日志过滤下拉框变化事件
    void onLogFilterChanged(int index);

private:
    //初始化函数
    void initUI();                  //初始化用户界面
    void initTab_2();               //初始化第二个标签页
    void loadAndApplyStyleSheet(const QString &styleSheetPath); //加载样式表
    void loadConfig();              //加载配置
    void saveConfig();              //保存配置
    void setupConnections();        //设置信号槽连接
    void setupLogFilter();          //设置日志过滤器
    void updateButtonStates(bool isRunning, bool isPaused); //更新按钮状态

    //配置验证函数
    QStringList getAddressList(); //获取所有地址
    QStringList getSelectedAddressList();

    bool validateConfig(const QStringList addrList);          //验证配置是否有效
    bool validateAddress(const QString &addr, QHostAddress &ip, quint16 &port); //验证地址格式
    bool isDuplicateAddress(const QString &addr) const; //检查地址是否重复

    //UI 操作函数
    void addAddressToUI(const QString &addr); //添加地址到 UI

    //日志相关函数
    QColor colorForLevel(const QString &level) const; //根据日志级别返回颜色
    void appendLogToView(const QString &html);       //添加日志到视图
    void refreshLogView();                           //刷新日志视图

    //工作线程信号槽连接管理
    void connectWorkerSlots();      //连接工作线程信号槽
    void disConnectWorkerSlots();   //断开工作线程信号槽

private:
    QScopedPointer<Ui::MainWindow> ui; //UI 对象
    ConfigManager m_configManager;     //配置管理器
    QTimer m_saveTimer;                //配置保存防抖定时器
    mutable QMutex m_configMutex;      //配置访问互斥锁
    QThread m_workerThread;            //工作线程
    QScopedPointer<WorkerClass> m_worker; //工作类对象
    QTextCharFormat m_logFormat;       //日志文本格式
    QComboBox *m_pLogFilterCombo;      //日志过滤下拉框
    QMap<QString, QList<LogEntry >> m_logEntriesByLevel; //日志条目缓存
    QString m_currentFilterLevel; //当前日志过滤级别
    ThemeColors m_logColors; //日志颜色配置

};

#endif //MAINWINDOW_H
