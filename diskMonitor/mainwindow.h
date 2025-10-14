#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QProgressBar>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QListWidget>
#include <QSplitter>
#include <QStorageInfo>
#include <QProcess>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QScrollArea>
#include <QFrame>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QApplication>
#include <QStyle>
#include <QPalette>
#include <QColor>
#include <QBrush>
#include <QFont>
#include <QFontMetrics>
#include <QSizePolicy>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QDateTime>
#include <QThread>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QCloseEvent>
#include <QApplication>
#include <QStandardPaths>
#include "processmonitor.h"
#include "historymanager.h"
#include <QSortFilterProxyModel>

// 前向声明
class HistoryDialog;
class ChartDialog;

// 自定义QTreeWidgetItem子类，用于处理数值排序
class ProcessTreeWidgetItem : public QTreeWidgetItem
{
public:
    explicit ProcessTreeWidgetItem(QTreeWidget *parent = nullptr);
    
    /**
     * @brief 重写比较操作符，实现正确的数值排序
     * @param other 要比较的另一个项目
     * @param column 比较的列
     * @return 如果当前项目应该排在other之前返回true
     */
    bool operator<(const QTreeWidgetItem &other) const override;
};

// 声明ProcessInfo为Qt元类型，以便在QVariant中使用
Q_DECLARE_METATYPE(ProcessInfo)

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct DriveInfo {
    QString driveLetter;
    QString displayName;
    qint64 totalSpace;
    qint64 usedSpace;
    qint64 freeSpace;
    double usagePercentage;
    QList<ProcessInfo> topProcesses; // 使用真实的ProcessInfo结构
    int processCount;
    int criticalProcessCount;
    bool isMonitored;
    QColor statusColor;
    qint64 totalDiskReadBytes;  // 新增：总磁盘读取字节数
    qint64 totalDiskWriteBytes; // 新增：总磁盘写入字节数
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateDiskInfo();
    void onDriveSelectionChanged();
    void onRefreshIntervalChanged();
    void onProcessItemDoubleClicked(QTreeWidgetItem* item, int column);
    void refreshData();
    void showAbout();
    void showSettings();
    void showHistoryDialog();
    void exportHistoryCSV();
    void exportHistoryExcel();
    void showChartDialog();
    void showRealTimeChart();
    void showHistoryTrendChart();
    void showMultiDriveChart();

private slots:
    // 托盘相关槽函数
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void showWindow();
    void hideWindow();
    void quitApplication();
    
    // 设置相关槽函数
    void toggleAutoStart(bool enabled);
    void toggleMinimizeToTray(bool enabled);
    void toggleStartMinimized(bool enabled);
    void toggleCloseToTray(bool enabled);

private slots:
    void onProcessDataUpdated();
    void onDriveStatsUpdated(const QString& driveLetter, const DriveProcessStats& stats);
    void onProcessMonitorError(const QString& error);
    void onProcessStarted(const ProcessInfo& process);
    void onProcessTerminated(quint32 processId);

private:
    void setupUI();
    void setupMenuBar();
    void setupStatusBar();
    void initializeDriveMonitoring();
    void initializeProcessMonitor();
    void initializeHistoryManager();
    void recordDriveHistory();
    void exportHistoryData(const QString &driveLetter, 
                          const QDateTime &startTime, 
                          const QDateTime &endTime,
                          ExportFormat format);

    // 新增的进程监控相关方法
    void updateDriveWithProcessStats(const QString& driveLetter, const DriveProcessStats& stats);
    void showProcessContextMenu(const QPoint& position);
    void terminateSelectedProcess();
    void showProcessDetails();
    void openSelectedProcessLocation();
    void scanDriveProcesses(DriveInfo& driveInfo);
    QColor calculateDriveColor(const DriveInfo& driveInfo);
    QString formatBytes(qint64 bytes);
    QWidget* createDriveWidget(const DriveInfo& driveInfo);
    bool eventFilter(QObject *obj, QEvent *event) override;
    
    // 托盘相关方法
    void setupSystemTray();
    void createTrayMenu();
    
    // 设置相关方法
    void loadSettings();
    void saveSettings();
    void setupAutoStart(bool enabled);
    bool isAutoStartEnabled();
    
    // 窗口事件处理
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void moveEvent(QMoveEvent *event) override;

    QString formatDiskUsage(qint64 bytes);
    QIcon getProcessIcon(const QString &executablePath);

    void updateDriveDisplay();
    void updateProcessList(const QString& driveLetter);
    void updateProcessListWithStats(const QString& driveLetter, const DriveProcessStats& stats);
    void openProcessLocation(const QString& processName);

signals:
    void driveDataUpdated();

private:
    Ui::MainWindow *ui;
    QTimer *refreshTimer;
    QList<DriveInfo> driveInfoList;
    QScrollArea *driveScrollArea;
    QWidget *driveContainer;
    QVBoxLayout *driveLayout;
    QTreeWidget *processTreeWidget;
    QComboBox *refreshIntervalCombo;
    QLabel *statusLabel;
    QPushButton *refreshButton;
    QSplitter *mainSplitter;
    
    // 进程监控器
    ProcessMonitor *processMonitor;
    QThread* monitorThread;
    
    // 当前选中的驱动器
    QString currentSelectedDrive;
    
    // 历史数据管理器
    HistoryManager *historyManager;
    
    // 历史数据查看对话框
    HistoryDialog *historyDialog;
    
    // 图表查看对话框
    ChartDialog *chartDialog;
    
    // 进程列表相关
    QMenu *processContextMenu;
    QAction *terminateProcessAction;
    QAction *showProcessDetailsAction;
    QAction *openProcessLocationAction;
    
    // 系统托盘相关
    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;
    QAction *showAction;
    QAction *hideAction;
    QAction *quitAction;
    
    // 设置相关
    QSettings *settings;
    bool minimizeToTray;
    bool startMinimized;
    bool autoStart;
    bool closeToTray;
    
    // 颜色方案
    QColor lowUsageColor;      // 低使用率颜色 (绿色)
    QColor mediumUsageColor;   // 中等使用率颜色 (黄色)
    QColor highUsageColor;     // 高使用率颜色 (橙色)
    QColor criticalUsageColor; // 危险使用率颜色 (红色)
};

#endif // MAINWINDOW_H
