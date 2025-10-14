#ifndef CHARTDIALOG_H
#define CHARTDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QSplitter>
#include <QCheckBox>
#include <QSpinBox>
#include <QSlider>
#include <QProgressBar>
#include <QTimer>
#include <QSettings>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QDebug>
#include "chartmanager.h"
#include "historymanager.h"

class ChartDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChartDialog(HistoryManager *historyManager, QWidget *parent = nullptr);
    ~ChartDialog();

    // 窗口管理
    void showChart(ChartType type, const QString &driveLetter = QString());
    void refreshCharts();
    void resetToDefaults();
    
    // 配置管理
    void loadSettings();
    void saveSettings();
    
    // 数据更新控制
    void startAutoRefresh(int intervalSeconds = 30);
    void stopAutoRefresh();
    bool isAutoRefreshActive() const;
    
protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    
public slots:
    void onRefreshClicked();
    void onExportClicked();
    void onResetClicked();
    void onAutoRefreshToggled(bool enabled);
    void onRefreshIntervalChanged(int seconds);
    void onChartTypeChanged(int index);
    void onDriveSelectionChanged(const QString &driveLetter);
    void onTimeRangeChanged(int index);
    void onCustomTimeRangeChanged();
    void onRealTimeToggled(bool enabled);
    void onStyleChanged();
    
private slots:
    void autoRefreshTimeout();
    void onChartCreated(ChartType type, QChartView *chartView);
    void onChartUpdated(ChartType type);
    void onChartExported(const QString &filePath);
    void onChartError(const QString &error);
    void updateStatusBar();
    void updateDriveList();
    
signals:
    void chartDialogClosed();
    void chartTypeSelected(ChartType type, const QString &driveLetter);
    void settingsChanged();
    
private:
    // 核心组件
    ChartManager *m_chartManager;
    HistoryManager *m_historyManager;
    
    // UI组件
    QTabWidget *m_tabWidget;
    QWidget *m_chartTab;
    QWidget *m_settingsTab;
    
    // 图表控制面板
    QGroupBox *m_controlGroup;
    QComboBox *m_chartTypeCombo;
    QComboBox *m_driveCombo;
    QComboBox *m_timeRangeCombo;
    QDateTimeEdit *m_startTimeEdit;
    QDateTimeEdit *m_endTimeEdit;
    QCheckBox *m_customRangeCheck;
    QPushButton *m_refreshBtn;
    QPushButton *m_exportBtn;
    QPushButton *m_resetBtn;
    
    // 实时更新控制
    QGroupBox *m_realTimeGroup;
    QCheckBox *m_realTimeCheck;
    QCheckBox *m_autoRefreshCheck;
    QSpinBox *m_refreshIntervalSpin;
    QLabel *m_statusLabel;
    QProgressBar *m_refreshProgress;
    
    // 样式控制
    QGroupBox *m_styleGroup;
    QCheckBox *m_showGridCheck;
    QCheckBox *m_showLegendCheck;
    QCheckBox *m_enableAnimationCheck;
    QSlider *m_opacitySlider;
    QLabel *m_opacityValueLabel;
    QComboBox *m_themeCombo;
    QComboBox *m_colorSchemeCombo;
    QPushButton *m_fullScreenBtn;
    QLabel *m_statusIndicator;
    
    // 图表显示区域
    QSplitter *m_chartSplitter;
    QWidget *m_chartContainer;
    
    // 状态和配置
    QTimer *m_autoRefreshTimer;
    QSettings *m_settings;
    QString m_currentDriveLetter;
    ChartType m_currentChartType;
    TimeRange m_currentTimeRange;
    bool m_isRealTimeEnabled;
    int m_refreshInterval;
    
    // 私有方法
    void setupUI();
    void setupChartTab();
    void setupSettingsTab();
    void setupControlPanel();
    void setupRealTimeControls();
    void setupStyleControls();
    void setupConnections();
    void setupStatusBar();
    
    void populateChartTypes();
    void populateDriveList();
    void populateTimeRanges();
    
    void updateControlsState();
    void updateCustomTimeRangeState();
    void updateRealTimeState();
    void updateChartDisplay();
    
    void applyChartStyle();
    void validateTimeRange();
    
    // 工具方法
    QString getChartTypeDisplayName(ChartType type);
    QString getTimeRangeDisplayName(TimeRange range);
    QIcon getChartTypeIcon(ChartType type);
    
    // 错误处理
    void logError(const QString &message);
    
    // 默认值
    static const int DEFAULT_REFRESH_INTERVAL = 30;
    static const ChartType DEFAULT_CHART_TYPE = ChartType::RealTimeUsage;
    static const TimeRange DEFAULT_TIME_RANGE = TimeRange::Last24Hours;
};

#endif // CHARTDIALOG_H