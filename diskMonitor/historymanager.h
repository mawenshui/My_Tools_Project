#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QTimer>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QMutex>
#include <QThread>
#include "processmonitor.h"

// 历史数据记录结构
struct HistoryRecord {
    QDateTime timestamp;           // 记录时间戳
    QString driveLetter;          // 驱动器盘符
    QString displayName;          // 显示名称
    qint64 totalSpace;           // 总空间
    qint64 usedSpace;            // 已用空间
    qint64 freeSpace;            // 可用空间
    double usagePercentage;      // 使用百分比
    int processCount;            // 活跃进程数量
    int criticalProcessCount;    // 关键进程数量
    qint64 totalDiskReadBytes;   // 总磁盘读取字节数
    qint64 totalDiskWriteBytes;  // 总磁盘写入字节数
};

// 导出格式枚举
enum class ExportFormat {
    CSV,
    Excel
};

// 数据保留策略
enum class RetentionPolicy {
    OneWeek = 7,
    OneMonth = 30,
    ThreeMonths = 90,
    SixMonths = 180,
    OneYear = 365,
    Forever = -1
};

class HistoryManager : public QObject
{
    Q_OBJECT

public:
    explicit HistoryManager(QObject *parent = nullptr);
    ~HistoryManager();

    // 数据库操作
    bool initializeDatabase();
    void closeDatabase();
    
    // 数据记录
    bool recordDriveData(const HistoryRecord &record);
    bool recordDriveDataBatch(const QList<HistoryRecord> &records);
    
    // 数据查询
    QList<HistoryRecord> getHistoryData(const QString &driveLetter, 
                                       const QDateTime &startTime, 
                                       const QDateTime &endTime);
    QList<HistoryRecord> getRecentData(const QString &driveLetter, int hours = 24);
    QList<HistoryRecord> getAllDrivesData(const QDateTime &startTime, 
                                         const QDateTime &endTime);
    
    // 数据统计
    double getAverageUsage(const QString &driveLetter, const QDateTime &startTime, 
                          const QDateTime &endTime);
    double getMaxUsage(const QString &driveLetter, const QDateTime &startTime, 
                      const QDateTime &endTime);
    qint64 getTotalDataTransfer(const QString &driveLetter, const QDateTime &startTime, 
                               const QDateTime &endTime);
    
    // 数据导出
    bool exportToCSV(const QString &filePath, const QString &driveLetter, 
                    const QDateTime &startTime, const QDateTime &endTime);
    bool exportToExcel(const QString &filePath, const QString &driveLetter, 
                      const QDateTime &startTime, const QDateTime &endTime);
    
    // 数据管理
    void setRetentionPolicy(RetentionPolicy policy);
    RetentionPolicy getRetentionPolicy() const;
    bool cleanupOldData();
    qint64 getDatabaseSize();
    bool compressDatabase();
    
    // 配置管理
    void setRecordingInterval(int seconds);
    int getRecordingInterval() const;
    void setAutoCleanup(bool enabled);
    bool isAutoCleanupEnabled() const;
    
    // 状态查询
    bool isDatabaseReady() const;
    QString getLastError() const;
    int getRecordCount(const QString &driveLetter = QString()) const;
    
public slots:
    void startRecording();
    void stopRecording();
    void recordCurrentData();
    void performCleanup();
    
signals:
    void dataRecorded(const HistoryRecord &record);
    void recordingStarted();
    void recordingStopped();
    void cleanupCompleted(int removedRecords);
    void errorOccurred(const QString &error);
    void databaseSizeChanged(qint64 size);
    
private slots:
    void onRecordingTimer();
    void onCleanupTimer();
    
private:
    // 数据库相关
    QSqlDatabase database;
    QString databasePath;
    QString lastError;
    mutable QMutex databaseMutex;
    
    // 记录控制
    QTimer *recordingTimer;
    QTimer *cleanupTimer;
    int recordingInterval;  // 记录间隔（秒）
    bool isRecording;
    
    // 配置
    RetentionPolicy retentionPolicy;
    bool autoCleanupEnabled;
    
    // 私有方法
    bool createTables();
    bool executeQuery(const QString &queryString, const QVariantList &values = QVariantList());
    QSqlQuery prepareQuery(const QString &queryString);
    QString formatDateTime(const QDateTime &dateTime) const;
    QDateTime parseDateTime(const QString &dateTimeString) const;
    bool validateDatabaseConnection() const;
    void logError(const QString &operation, const QSqlError &error);
    
    // 数据转换
    HistoryRecord recordFromQuery(const QSqlQuery &query) const;
    QVariantList recordToVariantList(const HistoryRecord &record) const;
    
    // 文件操作
    QString getDefaultDatabasePath() const;
    bool ensureDirectoryExists(const QString &path);
};

// 声明为Qt元类型
Q_DECLARE_METATYPE(HistoryRecord)
Q_DECLARE_METATYPE(ExportFormat)
Q_DECLARE_METATYPE(RetentionPolicy)

#endif // HISTORYMANAGER_H