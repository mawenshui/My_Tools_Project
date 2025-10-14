#ifndef CHARTMANAGER_H
#define CHARTMANAGER_H

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QSplitter>
#include <QTimer>
#include <QDateTime>
#include <QDebug>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QAreaSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QValueAxis>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QLegend>
#include <QtCharts/QScatterSeries>
#include "historymanager.h"

QT_CHARTS_USE_NAMESPACE

// 图表类型枚举
enum class ChartType {
    RealTimeUsage,      // 实时使用率曲线
    HistoryTrend,       // 历史趋势图
    MultiDriveCompare,  // 多驱动器对比
    DataTransfer,       // 数据传输量图表
    ProcessActivity     // 进程活动图表
};

// 时间范围枚举
enum class TimeRange {
    LastHour = 1,
    Last6Hours = 6,
    Last24Hours = 24,
    LastWeek = 168,
    LastMonth = 720,
    Custom = -1
};

// 图表样式配置
struct ChartStyle {
    QColor backgroundColor;     // 背景颜色
    QColor gridColor;          // 网格颜色
    QColor textColor;          // 文字颜色
    QFont titleFont;           // 标题字体
    QFont labelFont;           // 标签字体
    bool showGrid;             // 显示网格
    bool showLegend;           // 显示图例
    bool enableAnimation;      // 启用动画
};

class ChartManager : public QObject
{
    Q_OBJECT

public:
    explicit ChartManager(HistoryManager *historyManager, QObject *parent = nullptr);
    ~ChartManager();

    // 图表创建和管理
    QWidget* createChartWidget(QWidget *parent = nullptr);
    QChartView* createChart(ChartType type, const QString &driveLetter = QString());
    
    // 数据更新
    void updateRealTimeChart(const QString &driveLetter, double usagePercentage);
    void updateHistoryChart(const QString &driveLetter, TimeRange range);
    void updateMultiDriveChart(const QStringList &driveLetters, TimeRange range);
    void updateDataTransferChart(const QString &driveLetter, TimeRange range);
    
    // 图表配置
    void setChartStyle(const ChartStyle &style);
    ChartStyle getChartStyle() const;
    void setTimeRange(TimeRange range);
    TimeRange getTimeRange() const;
    void setCustomTimeRange(const QDateTime &start, const QDateTime &end);
    
    // 图表导出
    bool exportChart(ChartType type, const QString &filePath, const QSize &size = QSize(800, 600));
    bool exportAllCharts(const QString &directory);
    
    // 实时数据管理
    void startRealTimeUpdate(int intervalMs = 5000);
    void stopRealTimeUpdate();
    bool isRealTimeUpdateActive() const;
    
    // 图表显示控制
    void showChart(ChartType type);
    void hideChart(ChartType type);
    void toggleChart(ChartType type);
    
    // 数据查询辅助
    QList<HistoryRecord> getChartData(const QString &driveLetter, TimeRange range);
    QDateTime getStartTimeForRange(TimeRange range);
    
public slots:
    void refreshAllCharts();
    void refreshChart(ChartType type);
    void onDriveSelectionChanged(const QString &driveLetter);
    void onTimeRangeChanged(TimeRange range);
    void onRealTimeDataUpdate();
    
signals:
    void chartCreated(ChartType type, QChartView *chartView);
    void chartUpdated(ChartType type);
    void chartExported(const QString &filePath);
    void realTimeUpdateStarted();
    void realTimeUpdateStopped();
    void errorOccurred(const QString &error);
    
private slots:
    void updateRealTimeCharts();
    void onHistoryDataRecorded(const HistoryRecord &record);
    
private:
    // 核心组件
    HistoryManager *m_historyManager;
    QTimer *m_realTimeTimer;
    
    // 图表存储
    QMap<ChartType, QChartView*> m_charts;
    QMap<ChartType, QChart*> m_chartObjects;
    
    // 实时数据缓存
    QMap<QString, QLineSeries*> m_realTimeSeriesMap;
    QMap<QString, QList<QPointF>> m_realTimeDataCache;
    
    // 配置
    ChartStyle m_chartStyle;
    TimeRange m_currentTimeRange;
    QDateTime m_customStartTime;
    QDateTime m_customEndTime;
    QString m_currentDriveLetter;
    int m_realTimeUpdateInterval;
    int m_maxRealTimePoints;
    
    // 私有方法
    void initializeChartStyle();
    void setupChart(QChart *chart, const QString &title);
    void applyStyleToChart(QChart *chart);
    
    // 图表创建方法
    QChartView* createRealTimeChart(const QString &driveLetter);
    QChartView* createHistoryTrendChart(const QString &driveLetter);
    QChartView* createMultiDriveCompareChart(const QStringList &driveLetters);
    QChartView* createDataTransferChart(const QString &driveLetter);
    QChartView* createProcessActivityChart(const QString &driveLetter);
    
    // 数据处理方法
    void addRealTimePoint(const QString &driveLetter, double value);
    void limitRealTimePoints(QLineSeries *series);
    QLineSeries* createSeriesFromHistoryData(const QList<HistoryRecord> &data, 
                                             const QString &seriesName);
    
    // 工具方法
    QString formatBytes(qint64 bytes);
    QString getChartTitle(ChartType type, const QString &driveLetter = QString());
    QColor getSeriesColor(int index);
    void logError(const QString &message);
};

#endif // CHARTMANAGER_H