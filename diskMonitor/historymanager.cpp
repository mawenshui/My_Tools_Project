#include "historymanager.h"
#include <QApplication>
#include <QSqlRecord>
#include <QVariant>
#include <QFileInfo>
#include <QTextStream>
#include <QCoreApplication>

HistoryManager::HistoryManager(QObject *parent)
    : QObject(parent)
    , recordingInterval(300)  // 默认5分钟记录一次
    , isRecording(false)
    , retentionPolicy(RetentionPolicy::OneMonth)
    , autoCleanupEnabled(true)
{
    // 初始化定时器
    recordingTimer = new QTimer(this);
    cleanupTimer = new QTimer(this);
    
    connect(recordingTimer, &QTimer::timeout, this, &HistoryManager::onRecordingTimer);
    connect(cleanupTimer, &QTimer::timeout, this, &HistoryManager::onCleanupTimer);
    
    // 设置清理定时器为每天执行一次
    cleanupTimer->start(24 * 60 * 60 * 1000); // 24小时
    
    // 获取默认数据库路径
    databasePath = getDefaultDatabasePath();
}

HistoryManager::~HistoryManager()
{
    stopRecording();
    closeDatabase();
}

bool HistoryManager::initializeDatabase()
{
    QMutexLocker locker(&databaseMutex);
    
    // 确保数据库目录存在
    QFileInfo dbFileInfo(databasePath);
    if (!ensureDirectoryExists(dbFileInfo.absolutePath())) {
        lastError = QString("无法创建数据库目录: %1").arg(dbFileInfo.absolutePath());
        return false;
    }
    
    // 创建数据库连接
    database = QSqlDatabase::addDatabase("QSQLITE", "HistoryDB");
    database.setDatabaseName(databasePath);
    
    if (!database.open()) {
        lastError = QString("无法打开数据库: %1").arg(database.lastError().text());
        return false;
    }
    
    // 创建表结构
    if (!createTables()) {
        return false;
    }
    
    qDebug() << "历史数据库初始化成功:" << databasePath;
    return true;
}

void HistoryManager::closeDatabase()
{
    QMutexLocker locker(&databaseMutex);
    
    if (database.isOpen()) {
        database.close();
        qDebug() << "历史数据库已关闭";
    }
}

bool HistoryManager::createTables()
{
    // 创建历史记录表
    QString createTableQuery = R"(
        CREATE TABLE IF NOT EXISTS drive_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT NOT NULL,
            drive_letter TEXT NOT NULL,
            display_name TEXT,
            total_space INTEGER,
            used_space INTEGER,
            free_space INTEGER,
            usage_percentage REAL,
            process_count INTEGER,
            critical_process_count INTEGER,
            total_read_bytes INTEGER,
            total_write_bytes INTEGER,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    if (!executeQuery(createTableQuery)) {
        return false;
    }
    
    // 创建索引以提高查询性能
    QString createIndexQuery1 = "CREATE INDEX IF NOT EXISTS idx_drive_timestamp ON drive_history(drive_letter, timestamp)";
    QString createIndexQuery2 = "CREATE INDEX IF NOT EXISTS idx_timestamp ON drive_history(timestamp)";
    
    if (!executeQuery(createIndexQuery1) || !executeQuery(createIndexQuery2)) {
        return false;
    }
    
    // 创建配置表
    QString createConfigQuery = R"(
        CREATE TABLE IF NOT EXISTS config (
            key TEXT PRIMARY KEY,
            value TEXT,
            updated_at TEXT DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    return executeQuery(createConfigQuery);
}

bool HistoryManager::recordDriveData(const HistoryRecord &record)
{
    QMutexLocker locker(&databaseMutex);
    
    if (!validateDatabaseConnection()) {
        return false;
    }
    
    QString insertQuery = R"(
        INSERT INTO drive_history (
            timestamp, drive_letter, display_name, total_space, used_space, 
            free_space, usage_percentage, process_count, critical_process_count,
            total_read_bytes, total_write_bytes
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";
    
    QVariantList values = recordToVariantList(record);
    
    if (executeQuery(insertQuery, values)) {
        emit dataRecorded(record);
        return true;
    }
    
    return false;
}

bool HistoryManager::recordDriveDataBatch(const QList<HistoryRecord> &records)
{
    QMutexLocker locker(&databaseMutex);
    
    if (!validateDatabaseConnection()) {
        return false;
    }
    
    database.transaction();
    
    QString insertQuery = R"(
        INSERT INTO drive_history (
            timestamp, drive_letter, display_name, total_space, used_space, 
            free_space, usage_percentage, process_count, critical_process_count,
            total_read_bytes, total_write_bytes
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";
    
    QSqlQuery query = prepareQuery(insertQuery);
    
    for (const HistoryRecord &record : records) {
        QVariantList values = recordToVariantList(record);
        
        for (int i = 0; i < values.size(); ++i) {
            query.addBindValue(values[i]);
        }
        
        if (!query.exec()) {
            logError("批量插入记录", query.lastError());
            database.rollback();
            return false;
        }
    }
    
    if (database.commit()) {
        for (const HistoryRecord &record : records) {
            emit dataRecorded(record);
        }
        return true;
    } else {
        logError("提交事务", database.lastError());
        return false;
    }
}

QList<HistoryRecord> HistoryManager::getHistoryData(const QString &driveLetter, 
                                                   const QDateTime &startTime, 
                                                   const QDateTime &endTime)
{
    QMutexLocker locker(&databaseMutex);
    QList<HistoryRecord> records;
    
    if (!validateDatabaseConnection()) {
        return records;
    }
    
    QString queryString = R"(
        SELECT * FROM drive_history 
        WHERE drive_letter = ? AND timestamp BETWEEN ? AND ?
        ORDER BY timestamp ASC
    )";
    
    QSqlQuery query = prepareQuery(queryString);
    query.addBindValue(driveLetter);
    query.addBindValue(formatDateTime(startTime));
    query.addBindValue(formatDateTime(endTime));
    
    if (query.exec()) {
        while (query.next()) {
            records.append(recordFromQuery(query));
        }
    } else {
        logError("查询历史数据", query.lastError());
    }
    
    return records;
}

QList<HistoryRecord> HistoryManager::getRecentData(const QString &driveLetter, int hours)
{
    QDateTime endTime = QDateTime::currentDateTime();
    QDateTime startTime = endTime.addSecs(-hours * 3600);
    return getHistoryData(driveLetter, startTime, endTime);
}

QList<HistoryRecord> HistoryManager::getAllDrivesData(const QDateTime &startTime, 
                                                     const QDateTime &endTime)
{
    QMutexLocker locker(&databaseMutex);
    QList<HistoryRecord> records;
    
    if (!validateDatabaseConnection()) {
        return records;
    }
    
    QString queryString = R"(
        SELECT * FROM drive_history 
        WHERE timestamp BETWEEN ? AND ?
        ORDER BY timestamp ASC, drive_letter ASC
    )";
    
    QSqlQuery query = prepareQuery(queryString);
    query.addBindValue(formatDateTime(startTime));
    query.addBindValue(formatDateTime(endTime));
    
    if (query.exec()) {
        while (query.next()) {
            records.append(recordFromQuery(query));
        }
    } else {
        logError("查询所有驱动器数据", query.lastError());
    }
    
    return records;
}

double HistoryManager::getAverageUsage(const QString &driveLetter, 
                                      const QDateTime &startTime, 
                                      const QDateTime &endTime)
{
    QMutexLocker locker(&databaseMutex);
    
    if (!validateDatabaseConnection()) {
        return 0.0;
    }
    
    QString queryString = R"(
        SELECT AVG(usage_percentage) as avg_usage 
        FROM drive_history 
        WHERE drive_letter = ? AND timestamp BETWEEN ? AND ?
    )";
    
    QSqlQuery query = prepareQuery(queryString);
    query.addBindValue(driveLetter);
    query.addBindValue(formatDateTime(startTime));
    query.addBindValue(formatDateTime(endTime));
    
    if (query.exec() && query.next()) {
        return query.value("avg_usage").toDouble();
    }
    
    return 0.0;
}

double HistoryManager::getMaxUsage(const QString &driveLetter, 
                                  const QDateTime &startTime, 
                                  const QDateTime &endTime)
{
    QMutexLocker locker(&databaseMutex);
    
    if (!validateDatabaseConnection()) {
        return 0.0;
    }
    
    QString queryString = R"(
        SELECT MAX(usage_percentage) as max_usage 
        FROM drive_history 
        WHERE drive_letter = ? AND timestamp BETWEEN ? AND ?
    )";
    
    QSqlQuery query = prepareQuery(queryString);
    query.addBindValue(driveLetter);
    query.addBindValue(formatDateTime(startTime));
    query.addBindValue(formatDateTime(endTime));
    
    if (query.exec() && query.next()) {
        return query.value("max_usage").toDouble();
    }
    
    return 0.0;
}

qint64 HistoryManager::getTotalDataTransfer(const QString &driveLetter, 
                                           const QDateTime &startTime, 
                                           const QDateTime &endTime)
{
    QMutexLocker locker(&databaseMutex);
    
    if (!validateDatabaseConnection()) {
        return 0;
    }
    
    QString queryString = R"(
        SELECT SUM(total_read_bytes + total_write_bytes) as total_transfer 
        FROM drive_history 
        WHERE drive_letter = ? AND timestamp BETWEEN ? AND ?
    )";
    
    QSqlQuery query = prepareQuery(queryString);
    query.addBindValue(driveLetter);
    query.addBindValue(formatDateTime(startTime));
    query.addBindValue(formatDateTime(endTime));
    
    if (query.exec() && query.next()) {
        return query.value("total_transfer").toLongLong();
    }
    
    return 0;
}

bool HistoryManager::exportToCSV(const QString &filePath, const QString &driveLetter, 
                                const QDateTime &startTime, const QDateTime &endTime)
{
    QList<HistoryRecord> records = getHistoryData(driveLetter, startTime, endTime);
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        lastError = QString("无法创建CSV文件: %1").arg(file.errorString());
        return false;
    }
    
    QTextStream out(&file);
    out.setCodec("UTF-8");
    
    // 写入CSV头部
    out << "时间戳,驱动器,显示名称,总空间(GB),已用空间(GB),可用空间(GB),使用率(%),"
        << "活跃进程数,关键进程数,读取字节数,写入字节数\n";
    
    // 写入数据
    for (const HistoryRecord &record : records) {
        out << record.timestamp.toString("yyyy-MM-dd hh:mm:ss") << ","
            << record.driveLetter << ","
            << record.displayName << ","
            << QString::number(record.totalSpace / (1024.0 * 1024.0 * 1024.0), 'f', 2) << ","
            << QString::number(record.usedSpace / (1024.0 * 1024.0 * 1024.0), 'f', 2) << ","
            << QString::number(record.freeSpace / (1024.0 * 1024.0 * 1024.0), 'f', 2) << ","
            << QString::number(record.usagePercentage, 'f', 2) << ","
            << record.processCount << ","
            << record.criticalProcessCount << ","
            << record.totalDiskReadBytes << ","
            << record.totalDiskWriteBytes << "\n";
    }
    
    file.close();
    return true;
}

bool HistoryManager::exportToExcel(const QString &filePath, const QString &driveLetter, 
                                  const QDateTime &startTime, const QDateTime &endTime)
{
    // 简化实现：导出为CSV格式，可以被Excel打开
    // 在实际项目中，可以使用QXlsx库来生成真正的Excel文件
    QString csvPath = filePath;
    if (!csvPath.endsWith(".csv", Qt::CaseInsensitive)) {
        csvPath += ".csv";
    }
    
    return exportToCSV(csvPath, driveLetter, startTime, endTime);
}

void HistoryManager::setRetentionPolicy(RetentionPolicy policy)
{
    retentionPolicy = policy;
    
    // 保存配置到数据库
    QString updateQuery = "INSERT OR REPLACE INTO config (key, value) VALUES ('retention_policy', ?)";
    executeQuery(updateQuery, {static_cast<int>(policy)});
}

RetentionPolicy HistoryManager::getRetentionPolicy() const
{
    return retentionPolicy;
}

bool HistoryManager::cleanupOldData()
{
    QMutexLocker locker(&databaseMutex);
    
    if (!validateDatabaseConnection() || retentionPolicy == RetentionPolicy::Forever) {
        return true;
    }
    
    QDateTime cutoffTime = QDateTime::currentDateTime().addDays(-static_cast<int>(retentionPolicy));
    
    QString deleteQuery = "DELETE FROM drive_history WHERE timestamp < ?";
    QSqlQuery query = prepareQuery(deleteQuery);
    query.addBindValue(formatDateTime(cutoffTime));
    
    if (query.exec()) {
        int removedRecords = query.numRowsAffected();
        qDebug() << "清理了" << removedRecords << "条历史记录";
        emit cleanupCompleted(removedRecords);
        return true;
    } else {
        logError("清理历史数据", query.lastError());
        return false;
    }
}

qint64 HistoryManager::getDatabaseSize()
{
    QFileInfo fileInfo(databasePath);
    if (fileInfo.exists()) {
        qint64 size = fileInfo.size();
        emit databaseSizeChanged(size);
        return size;
    }
    return 0;
}

bool HistoryManager::compressDatabase()
{
    QMutexLocker locker(&databaseMutex);
    
    if (!validateDatabaseConnection()) {
        return false;
    }
    
    // 执行VACUUM命令来压缩数据库
    QString vacuumQuery = "VACUUM";
    if (executeQuery(vacuumQuery)) {
        qDebug() << "数据库压缩完成";
        getDatabaseSize(); // 更新大小信息
        return true;
    }
    
    return false;
}

void HistoryManager::setRecordingInterval(int seconds)
{
    recordingInterval = qMax(60, seconds); // 最小1分钟
    
    if (isRecording) {
        recordingTimer->setInterval(recordingInterval * 1000);
    }
    
    // 保存配置
    QString updateQuery = "INSERT OR REPLACE INTO config (key, value) VALUES ('recording_interval', ?)";
    executeQuery(updateQuery, {recordingInterval});
}

int HistoryManager::getRecordingInterval() const
{
    return recordingInterval;
}

void HistoryManager::setAutoCleanup(bool enabled)
{
    autoCleanupEnabled = enabled;
    
    if (enabled) {
        cleanupTimer->start(24 * 60 * 60 * 1000); // 24小时
    } else {
        cleanupTimer->stop();
    }
    
    // 保存配置
    QString updateQuery = "INSERT OR REPLACE INTO config (key, value) VALUES ('auto_cleanup', ?)";
    executeQuery(updateQuery, {enabled ? 1 : 0});
}

bool HistoryManager::isAutoCleanupEnabled() const
{
    return autoCleanupEnabled;
}

bool HistoryManager::isDatabaseReady() const
{
    QMutexLocker locker(&databaseMutex);
    return database.isOpen() && database.isValid();
}

QString HistoryManager::getLastError() const
{
    return lastError;
}

int HistoryManager::getRecordCount(const QString &driveLetter) const
{
    QMutexLocker locker(&databaseMutex);
    
    if (!validateDatabaseConnection()) {
        return 0;
    }
    
    QString queryString;
    QSqlQuery query(database);
    
    if (driveLetter.isEmpty()) {
        queryString = "SELECT COUNT(*) as count FROM drive_history";
        query.prepare(queryString);
    } else {
        queryString = "SELECT COUNT(*) as count FROM drive_history WHERE drive_letter = ?";
        query.prepare(queryString);
        query.addBindValue(driveLetter);
    }
    
    if (query.exec() && query.next()) {
        return query.value("count").toInt();
    }
    
    return 0;
}

void HistoryManager::startRecording()
{
    if (!isRecording && isDatabaseReady()) {
        isRecording = true;
        recordingTimer->start(recordingInterval * 1000);
        emit recordingStarted();
        qDebug() << "历史数据记录已启动，间隔:" << recordingInterval << "秒";
    }
}

void HistoryManager::stopRecording()
{
    if (isRecording) {
        isRecording = false;
        recordingTimer->stop();
        emit recordingStopped();
        qDebug() << "历史数据记录已停止";
    }
}

void HistoryManager::recordCurrentData()
{
    // 这个方法需要从MainWindow获取当前的驱动器数据
    // 在实际使用中，会通过信号槽连接到MainWindow的数据更新
    qDebug() << "记录当前数据 - 需要从MainWindow获取数据";
}

void HistoryManager::performCleanup()
{
    if (autoCleanupEnabled) {
        cleanupOldData();
        compressDatabase();
    }
}

void HistoryManager::onRecordingTimer()
{
    recordCurrentData();
}

void HistoryManager::onCleanupTimer()
{
    performCleanup();
}

// 私有方法实现

bool HistoryManager::executeQuery(const QString &queryString, const QVariantList &values)
{
    QSqlQuery query = prepareQuery(queryString);
    
    for (const QVariant &value : values) {
        query.addBindValue(value);
    }
    
    if (query.exec()) {
        return true;
    } else {
        logError("执行查询", query.lastError());
        return false;
    }
}

QSqlQuery HistoryManager::prepareQuery(const QString &queryString)
{
    QSqlQuery query(database);
    query.prepare(queryString);
    return query;
}

QString HistoryManager::formatDateTime(const QDateTime &dateTime) const
{
    return dateTime.toString("yyyy-MM-dd hh:mm:ss");
}

QDateTime HistoryManager::parseDateTime(const QString &dateTimeString) const
{
    return QDateTime::fromString(dateTimeString, "yyyy-MM-dd hh:mm:ss");
}

bool HistoryManager::validateDatabaseConnection() const
{
    if (!database.isOpen() || !database.isValid()) {
        return false;
    }
    return true;
}

void HistoryManager::logError(const QString &operation, const QSqlError &error)
{
    lastError = QString("%1失败: %2").arg(operation, error.text());
    qWarning() << lastError;
    emit errorOccurred(lastError);
}

HistoryRecord HistoryManager::recordFromQuery(const QSqlQuery &query) const
{
    HistoryRecord record;
    record.timestamp = parseDateTime(query.value("timestamp").toString());
    record.driveLetter = query.value("drive_letter").toString();
    record.displayName = query.value("display_name").toString();
    record.totalSpace = query.value("total_space").toLongLong();
    record.usedSpace = query.value("used_space").toLongLong();
    record.freeSpace = query.value("free_space").toLongLong();
    record.usagePercentage = query.value("usage_percentage").toDouble();
    record.processCount = query.value("process_count").toInt();
    record.criticalProcessCount = query.value("critical_process_count").toInt();
    record.totalDiskReadBytes = query.value("total_read_bytes").toLongLong();
    record.totalDiskWriteBytes = query.value("total_write_bytes").toLongLong();
    return record;
}

QVariantList HistoryManager::recordToVariantList(const HistoryRecord &record) const
{
    return {
        formatDateTime(record.timestamp),
        record.driveLetter,
        record.displayName,
        record.totalSpace,
        record.usedSpace,
        record.freeSpace,
        record.usagePercentage,
        record.processCount,
        record.criticalProcessCount,
        record.totalDiskReadBytes,
        record.totalDiskWriteBytes
    };
}

QString HistoryManager::getDefaultDatabasePath() const
{
    // 获取可执行程序所在目录
    QString appDirPath = QCoreApplication::applicationDirPath();
    // 在可执行程序目录下创建database文件夹
    QString databaseDir = QDir(appDirPath).absoluteFilePath("database");
    return QDir(databaseDir).absoluteFilePath("diskmonitor_history.db");
}

bool HistoryManager::ensureDirectoryExists(const QString &path)
{
    QDir dir;
    return dir.mkpath(path);
}