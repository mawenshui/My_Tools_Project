#include "chartmanager.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QSplitter>
#include <QDateTime>
#include <QTimer>
#include <QDebug>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>
#include <QRandomGenerator>
#include <cmath>
#include <QDir>
#include <QPixmap>
#include <QPainter>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <cstdlib>

ChartManager::ChartManager(HistoryManager *historyManager, QObject *parent)
    : QObject(parent)
    , m_historyManager(historyManager)
    , m_realTimeTimer(new QTimer(this))
    , m_currentTimeRange(TimeRange::Last24Hours)
    , m_currentDriveLetter("C:")
    , m_realTimeUpdateInterval(5000)
    , m_maxRealTimePoints(100)
{
    // 初始化图表样式
    initializeChartStyle();
    
    // 连接实时更新定时器
    connect(m_realTimeTimer, &QTimer::timeout, this, &ChartManager::updateRealTimeCharts);
    
    // 连接历史数据管理器信号
    if (m_historyManager) {
        connect(m_historyManager, &HistoryManager::dataRecorded,
                this, &ChartManager::onHistoryDataRecorded);
    }
    
    qDebug() << "[ChartManager] 图表管理器初始化完成";
}

ChartManager::~ChartManager()
{
    // 停止实时更新
    stopRealTimeUpdate();
    
    // 清理图表对象
    for (auto it = m_charts.begin(); it != m_charts.end(); ++it) {
        if (it.value()) {
            delete it.value();
        }
    }
    m_charts.clear();
    
    // 清理实时数据缓存
    for (auto it = m_realTimeSeriesMap.begin(); it != m_realTimeSeriesMap.end(); ++it) {
        if (it.value()) {
            delete it.value();
        }
    }
    m_realTimeSeriesMap.clear();
    m_realTimeDataCache.clear();
    
    qDebug() << "[ChartManager] 图表管理器已销毁";
}

void ChartManager::initializeChartStyle()
{
    // 使用更现代和友好的配色方案
    m_chartStyle.backgroundColor = QColor(255, 255, 255);  // 纯白背景
    m_chartStyle.gridColor = QColor(230, 230, 230);       // 更淡的网格线
    m_chartStyle.textColor = QColor(60, 60, 60);          // 深灰色文字
    m_chartStyle.titleFont = QFont("Microsoft YaHei", 14, QFont::Bold);  // 更大的标题字体
    m_chartStyle.labelFont = QFont("Microsoft YaHei", 11);              // 更大的标签字体
    m_chartStyle.showGrid = true;
    m_chartStyle.showLegend = true;
    m_chartStyle.enableAnimation = false;  // 禁用动画避免崩溃
    
    qDebug() << "[ChartManager] 图表样式初始化完成";
}

void ChartManager::setupChart(QChart *chart, const QString &title)
{
    if (!chart) {
        qDebug() << "[ChartManager] 警告: 尝试设置空图表";
        return;
    }
    
    try {
        // 设置标题和字体
        chart->setTitle(title);
        chart->setTitleFont(m_chartStyle.titleFont);
        chart->setTitleBrush(QBrush(m_chartStyle.textColor));
        
        // 设置背景
        chart->setBackgroundBrush(QBrush(m_chartStyle.backgroundColor));
        chart->setBackgroundRoundness(8);  // 圆角背景
        
        // 设置边距，让图表更美观
        chart->setMargins(QMargins(20, 20, 20, 20));
        
        // 配置图例
        if (m_chartStyle.showLegend) {
            chart->legend()->setVisible(true);
            chart->legend()->setAlignment(Qt::AlignBottom);
            chart->legend()->setFont(m_chartStyle.labelFont);
            chart->legend()->setLabelColor(m_chartStyle.textColor);
            chart->legend()->setBackgroundVisible(false);  // 透明图例背景
        } else {
            chart->legend()->setVisible(false);
        }
        
        // 禁用动画避免崩溃
        chart->setAnimationOptions(QChart::NoAnimation);
        
        qDebug() << "[ChartManager] 图表样式设置完成:" << title;
        
    } catch (const std::exception &e) {
        qDebug() << "[ChartManager] 设置图表样式时发生异常:" << e.what();
    }
}

QChartView* ChartManager::createChart(ChartType type, const QString &driveLetter)
{
    // 参数验证
    if (driveLetter.isEmpty() && type != ChartType::MultiDriveCompare) {
        logError("创建图表失败：驱动器字母为空");
        return nullptr;
    }
    
    QChartView* chartView = nullptr;
    
    try {
        // 根据图表类型调用相应的创建方法
        switch (type) {
        case ChartType::RealTimeUsage:
            chartView = createRealTimeChart(driveLetter);
            break;
        case ChartType::HistoryTrend:
            chartView = createHistoryTrendChart(driveLetter);
            break;
        case ChartType::MultiDriveCompare:
            {
                QStringList drives;
                if (!driveLetter.isEmpty()) {
                    drives << driveLetter;
                } else {
                    // 获取所有可用驱动器
                    drives << "C:" << "D:" << "E:";
                }
                chartView = createMultiDriveCompareChart(drives);
            }
            break;
        case ChartType::DataTransfer:
            chartView = createDataTransferChart(driveLetter);
            break;
        case ChartType::ProcessActivity:
            chartView = createProcessActivityChart(driveLetter);
            break;
        default:
            logError(QString("创建图表失败：未知图表类型 %1").arg(static_cast<int>(type)));
            return nullptr;
        }
        
        if (chartView) {
            // 缓存图表视图
            m_charts[type] = chartView;
            qDebug() << QString("[ChartManager] 图表创建成功 - 类型: %1, 驱动器: %2")
                        .arg(static_cast<int>(type)).arg(driveLetter);
            
            // 发出图表创建信号
            emit chartCreated(type, chartView);
        } else {
            logError(QString("创建图表失败：图表类型 %1 创建方法返回空指针").arg(static_cast<int>(type)));
        }
        
    } catch (const std::exception &e) {
        logError(QString("创建图表失败：发生异常 - %1").arg(e.what()));
        return nullptr;
    } catch (...) {
        logError("创建图表失败：发生未知异常");
        return nullptr;
    }
    
    return chartView;
}

QChartView* ChartManager::createDataTransferChart(const QString &driveLetter)
{
    // 参数验证
    if (driveLetter.isEmpty()) {
        logError("创建数据传输图表失败：驱动器字母为空");
        return nullptr;
    }
    
    // 检查历史管理器
    if (!m_historyManager) {
        logError("创建数据传输图表失败：历史管理器未初始化");
        return nullptr;
    }
    
    try {
        // 声明变量
        QChart *chart = nullptr;
        QLineSeries *readSeries = nullptr;
        QLineSeries *writeSeries = nullptr;
        QChartView *chartView = nullptr;
        
        // 创建图表对象
        chart = new QChart();
        if (!chart) {
            logError("创建数据传输图表失败：无法创建图表对象");
            return nullptr;
        }
        
        chart->setTitle(QString("%1 数据传输量").arg(driveLetter));
        
        // 获取指定时间范围内的历史数据
        QDateTime startTime = getStartTimeForRange(m_currentTimeRange);
        QDateTime endTime = QDateTime::currentDateTime();
        QList<HistoryRecord> historyData = m_historyManager->getHistoryData(driveLetter, startTime, endTime);
        
        if (historyData.isEmpty()) {
            qDebug() << QString("[ChartManager] 警告: 驱动器 %1 在指定时间范围内没有历史数据").arg(driveLetter);
            // 即使没有数据也创建空图表
        }
        
        // 创建读取速度系列
        readSeries = new QLineSeries();
        if (!readSeries) {
            logError("创建数据传输图表失败：无法创建读取系列");
            delete chart;
            return nullptr;
        }
        
        readSeries->setName("读取速度");
        readSeries->setColor(getSeriesColor(0));
        
        // 创建写入速度系列
        writeSeries = new QLineSeries();
        if (!writeSeries) {
            logError("创建数据传输图表失败：无法创建写入系列");
            delete readSeries;
            delete chart;
            return nullptr;
        }
        
        writeSeries->setName("写入速度");
        writeSeries->setColor(getSeriesColor(1));
        
        // 填充数据
        for (const HistoryRecord &record : historyData) {
            qint64 timestamp = record.timestamp.toMSecsSinceEpoch();
            readSeries->append(timestamp, record.totalDiskReadBytes / (1024.0 * 1024.0)); // 转换为MB
            writeSeries->append(timestamp, record.totalDiskWriteBytes / (1024.0 * 1024.0));
        }
        
        chart->addSeries(readSeries);
        chart->addSeries(writeSeries);
        
        // 设置坐标轴
        QDateTimeAxis *axisX = new QDateTimeAxis();
        if (!axisX) {
            logError("创建数据传输图表失败：无法创建X轴");
            delete writeSeries;
            delete readSeries;
            delete chart;
            return nullptr;
        }
        
        axisX->setTickCount(6);
        axisX->setFormat("MM-dd hh:mm");
        axisX->setTitleText("时间");
        axisX->setRange(startTime, endTime);
        chart->addAxis(axisX, Qt::AlignBottom);
        
        QValueAxis *axisY = new QValueAxis();
        if (!axisY) {
            logError("创建数据传输图表失败：无法创建Y轴");
            delete writeSeries;
            delete readSeries;
            delete chart;
            return nullptr;
        }
        
        axisY->setTitleText("数据量 (MB)");
        chart->addAxis(axisY, Qt::AlignLeft);
        
        // 附加系列到坐标轴
        readSeries->attachAxis(axisX);
        readSeries->attachAxis(axisY);
        writeSeries->attachAxis(axisX);
        writeSeries->attachAxis(axisY);
        
        // 应用样式
        setupChart(chart, getChartTitle(ChartType::DataTransfer, driveLetter));
        
        // 创建图表视图
        chartView = new QChartView(chart);
        if (!chartView) {
            logError("创建数据传输图表失败：无法创建图表视图");
            delete writeSeries;
            delete readSeries;
            delete chart;
            return nullptr;
        }
        
        chartView->setRenderHint(QPainter::Antialiasing);
        
        // 缓存图表对象
        m_chartObjects[ChartType::DataTransfer] = chart;
        
        qDebug() << QString("[ChartManager] 数据传输图表创建成功 - 驱动器: %1").arg(driveLetter);
        
        return chartView;
        
    } catch (const std::bad_alloc &e) {
        logError(QString("创建数据传输图表失败：内存分配异常 - %1").arg(e.what()));
        return nullptr;
        
    } catch (const std::exception &e) {
        logError(QString("创建数据传输图表失败：发生异常 - %1").arg(e.what()));
        return nullptr;
        
    } catch (...) {
        logError("创建数据传输图表失败：发生未知异常");
        return nullptr;
    }
}

QColor ChartManager::getSeriesColor(int index)
{
    // 使用更现代和友好的配色方案
    static QList<QColor> colors = {
        QColor(74, 144, 226),   // 现代蓝色
        QColor(80, 200, 120),   // 清新绿色
        QColor(255, 107, 107),  // 温和红色
        QColor(162, 155, 254),  // 柔和紫色
        QColor(255, 193, 7),    // 明亮黄色
        QColor(255, 138, 101),  // 温暖橙色
        QColor(77, 182, 172),   // 清爽青色
        QColor(149, 175, 192),  // 优雅灰色
        QColor(255, 99, 132),   // 粉红色
        QColor(54, 162, 235)    // 深蓝色
    };
    
    return colors[index % colors.size()];
}

QString ChartManager::getChartTitle(ChartType type, const QString &driveLetter)
{
    switch (type) {
    case ChartType::RealTimeUsage:
        return QString("%1 实时使用率").arg(driveLetter);
    case ChartType::HistoryTrend:
        return QString("%1 历史使用趋势").arg(driveLetter);
    case ChartType::MultiDriveCompare:
        return "多驱动器使用率对比";
    case ChartType::DataTransfer:
        return QString("%1 数据传输量").arg(driveLetter);
    case ChartType::ProcessActivity:
        return QString("%1 进程活动").arg(driveLetter);
    default:
        return "未知图表";
    }
}

QDateTime ChartManager::getStartTimeForRange(TimeRange range)
{
    QDateTime now = QDateTime::currentDateTime();
    
    switch (range) {
    case TimeRange::LastHour:
        return now.addSecs(-3600);
    case TimeRange::Last6Hours:
        return now.addSecs(-6 * 3600);
    case TimeRange::Last24Hours:
        return now.addSecs(-24 * 3600);
    case TimeRange::LastWeek:
        return now.addDays(-7);
    case TimeRange::LastMonth:
        return now.addDays(-30);
    case TimeRange::Custom:
        return m_customStartTime;
    default:
        return now.addSecs(-24 * 3600);
    }
}

void ChartManager::logError(const QString &message)
{
    // 使用qCritical输出错误信息，确保在发布版本中也能看到
    qCritical() << "[ChartManager] 错误:" << message;
    
    // 同时输出到调试信息
    qDebug() << "[ChartManager] 详细错误信息:" << message;
    
    // 发出错误信号
    emit errorOccurred(message);
}

void ChartManager::refreshAllCharts()
{
    for (auto it = m_charts.begin(); it != m_charts.end(); ++it) {
        refreshChart(it.key());
    }
    qDebug() << "[ChartManager] 刷新所有图表";
}

void ChartManager::refreshChart(ChartType type)
{
    // 简单的刷新实现
    if (m_charts.contains(type)) {
        qDebug() << QString("[ChartManager] 刷新图表类型: %1").arg(static_cast<int>(type));
        emit chartUpdated(type);
    }
}

void ChartManager::onDriveSelectionChanged(const QString &driveLetter)
{
    m_currentDriveLetter = driveLetter;
    refreshAllCharts();
    qDebug() << "[ChartManager] 驱动器选择变更:" << driveLetter;
}

void ChartManager::onTimeRangeChanged(TimeRange range)
{
    m_currentTimeRange = range;
    refreshAllCharts();
    qDebug() << "[ChartManager] 时间范围变更:" << static_cast<int>(range);
}

void ChartManager::onRealTimeDataUpdate()
{
    // 处理实时数据更新
    if (m_historyManager && m_historyManager->isDatabaseReady()) {
        updateRealTimeCharts();
        qDebug() << "[ChartManager] 实时数据已更新";
    }
}

bool ChartManager::exportChart(ChartType type, const QString &filePath, const QSize &size)
{
    if (!m_charts.contains(type)) {
        logError("要导出的图表不存在");
        return false;
    }
    
    QChartView *chartView = m_charts[type];
    if (!chartView) {
        logError("图表视图为空");
        return false;
    }
    
    try {
        QPixmap pixmap = chartView->grab(QRect(QPoint(), size));
        bool success = pixmap.save(filePath);
        
        if (success) {
            emit chartExported(filePath);
            qDebug() << "[ChartManager] 图表导出成功:" << filePath;
        } else {
            logError("图表导出失败: " + filePath);
        }
        
        return success;
        
    } catch (const std::exception &e) {
        logError(QString("导出图表时发生异常: %1").arg(e.what()));
        return false;
    }
}

void ChartManager::startRealTimeUpdate(int intervalMs)
{
    m_realTimeUpdateInterval = intervalMs;
    m_realTimeTimer->start(intervalMs);
    emit realTimeUpdateStarted();
    qDebug() << "[ChartManager] 开始实时更新，间隔:" << intervalMs << "ms";
}

void ChartManager::setChartStyle(const ChartStyle &style)
{
    m_chartStyle = style;
    
    // 更新所有现有图表的样式
    for (auto it = m_charts.begin(); it != m_charts.end(); ++it) {
        if (it.value() && it.value()->chart()) {
            applyStyleToChart(it.value()->chart());
        }
    }
    
    qDebug() << "[ChartManager] 图表样式已更新";
}

void ChartManager::applyStyleToChart(QChart *chart)
{
    if (!chart) return;
    
    // 应用背景色
    chart->setBackgroundBrush(QBrush(m_chartStyle.backgroundColor));
    
    // 应用标题样式
    chart->setTitleFont(m_chartStyle.titleFont);
    chart->setTitleBrush(QBrush(m_chartStyle.textColor));
    
    // 应用图例样式
    if (m_chartStyle.showLegend) {
        chart->legend()->setVisible(true);
        chart->legend()->setFont(m_chartStyle.labelFont);
        chart->legend()->setLabelBrush(QBrush(m_chartStyle.textColor));
    } else {
        chart->legend()->setVisible(false);
    }
    
    // 应用动画设置
    chart->setAnimationOptions(m_chartStyle.enableAnimation ? QChart::SeriesAnimations : QChart::NoAnimation);
    
    qDebug() << "[ChartManager] 图表样式已应用";
}

// 图表创建方法实现
QChartView* ChartManager::createRealTimeChart(const QString &driveLetter)
{
    try {
        QChart *chart = new QChart();
        chart->setTitle(QString("%1 实时使用率").arg(driveLetter));
        
        // 创建一个简单的线性系列
        QLineSeries *series = new QLineSeries();
        series->setName("使用率 (%)");
        series->setColor(getSeriesColor(0));
        
        // 添加一些示例数据点
        QDateTime now = QDateTime::currentDateTime();
        for (int i = 0; i < 10; i++) {
            series->append(now.addSecs(-i * 60).toMSecsSinceEpoch(), qrand() % 100);
        }
        
        chart->addSeries(series);
        
        // 设置坐标轴
        QDateTimeAxis *axisX = new QDateTimeAxis();
        axisX->setFormat("hh:mm");
        axisX->setTitleText("时间");
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);
        
        QValueAxis *axisY = new QValueAxis();
        axisY->setRange(0, 100);
        axisY->setTitleText("使用率 (%)");
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
        
        setupChart(chart, chart->title());
        
        QChartView *chartView = new QChartView(chart);
        chartView->setRenderHint(QPainter::Antialiasing);
        
        return chartView;
        
    } catch (const std::exception &e) {
        logError(QString("创建实时图表失败：%1").arg(e.what()));
        return nullptr;
    }
}

QChartView* ChartManager::createHistoryTrendChart(const QString &driveLetter)
{
    try {
        QChart *chart = new QChart();
        chart->setTitle(QString("%1 历史使用趋势").arg(driveLetter));
        
        // 创建一个简单的线性系列
        QLineSeries *series = new QLineSeries();
        series->setName("使用率 (%)");
        series->setColor(getSeriesColor(1));
        
        // 获取历史数据
        if (m_historyManager) {
            QDateTime startTime = getStartTimeForRange(m_currentTimeRange);
            QDateTime endTime = QDateTime::currentDateTime();
            QList<HistoryRecord> historyData = m_historyManager->getHistoryData(driveLetter, startTime, endTime);
            
            for (const HistoryRecord &record : historyData) {
                series->append(record.timestamp.toMSecsSinceEpoch(), record.usagePercentage);
            }
        }
        
        // 如果没有数据，添加一些示例数据
        if (series->count() == 0) {
            QDateTime now = QDateTime::currentDateTime();
            for (int i = 0; i < 24; i++) {
                series->append(now.addSecs(-i * 3600).toMSecsSinceEpoch(), qrand() % 100);
            }
        }
        
        chart->addSeries(series);
        
        // 设置坐标轴
        QDateTimeAxis *axisX = new QDateTimeAxis();
        axisX->setFormat("MM-dd hh:mm");
        axisX->setTitleText("时间");
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);
        
        QValueAxis *axisY = new QValueAxis();
        axisY->setRange(0, 100);
        axisY->setTitleText("使用率 (%)");
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
        
        setupChart(chart, chart->title());
        
        QChartView *chartView = new QChartView(chart);
        chartView->setRenderHint(QPainter::Antialiasing);
        
        return chartView;
        
    } catch (const std::exception &e) {
        logError(QString("创建历史趋势图表失败：%1").arg(e.what()));
        return nullptr;
    }
}

QChartView* ChartManager::createMultiDriveCompareChart(const QStringList &driveLetters)
{
    try {
        QChart *chart = new QChart();
        chart->setTitle("多驱动器使用率对比");
        
        int colorIndex = 0;
        for (const QString &drive : driveLetters) {
            QLineSeries *series = new QLineSeries();
            series->setName(drive);
            series->setColor(getSeriesColor(colorIndex++));
            
            // 添加示例数据
            QDateTime now = QDateTime::currentDateTime();
            for (int i = 0; i < 10; i++) {
                series->append(now.addSecs(-i * 60).toMSecsSinceEpoch(), qrand() % 100);
            }
            
            chart->addSeries(series);
        }
        
        // 设置坐标轴
        QDateTimeAxis *axisX = new QDateTimeAxis();
        axisX->setFormat("hh:mm");
        axisX->setTitleText("时间");
        chart->addAxis(axisX, Qt::AlignBottom);
        
        QValueAxis *axisY = new QValueAxis();
        axisY->setRange(0, 100);
        axisY->setTitleText("使用率 (%)");
        chart->addAxis(axisY, Qt::AlignLeft);
        
        // 将所有系列附加到坐标轴
        foreach (QAbstractSeries *series, chart->series()) {
            series->attachAxis(axisX);
            series->attachAxis(axisY);
        }
        
        setupChart(chart, chart->title());
        
        QChartView *chartView = new QChartView(chart);
        chartView->setRenderHint(QPainter::Antialiasing);
        
        return chartView;
        
    } catch (const std::exception &e) {
        logError(QString("创建多驱动器对比图表失败：%1").arg(e.what()));
        return nullptr;
    }
}

QChartView* ChartManager::createProcessActivityChart(const QString &driveLetter)
{
    try {
        QChart *chart = new QChart();
        chart->setTitle(QString("%1 进程活动").arg(driveLetter));
        
        // 创建一个简单的柱状图
        QBarSeries *series = new QBarSeries();
        QBarSet *set = new QBarSet("进程数量");
        
        // 添加示例数据
        *set << 10 << 15 << 8 << 12 << 20 << 18;
        series->append(set);
        
        chart->addSeries(series);
        
        // 设置坐标轴
        QStringList categories;
        categories << "系统" << "应用" << "服务" << "驱动" << "用户" << "其他";
        
        QBarCategoryAxis *axisX = new QBarCategoryAxis();
        axisX->append(categories);
        axisX->setTitleText("进程类型");
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);
        
        QValueAxis *axisY = new QValueAxis();
        axisY->setRange(0, 25);
        axisY->setTitleText("进程数量");
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
        
        setupChart(chart, chart->title());
        
        QChartView *chartView = new QChartView(chart);
        chartView->setRenderHint(QPainter::Antialiasing);
        
        return chartView;
        
    } catch (const std::exception &e) {
        logError(QString("创建进程活动图表失败：%1").arg(e.what()));
        return nullptr;
    }
}

// 其他必要的空实现函数
void ChartManager::updateRealTimeCharts() {}
void ChartManager::stopRealTimeUpdate() {}
void ChartManager::onHistoryDataRecorded(const HistoryRecord &record) { Q_UNUSED(record); }