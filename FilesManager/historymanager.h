#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include <QObject>
#include <QSettings>
#include <QDateTime>
#include <QVariantMap>
#include <QList>

/**
 * @brief 操作历史管理器 - 记录文件操作历史
 *
 * 功能特点：
 * 1. 单例模式设计，全局唯一实例
 * 2. 使用QSettings持久化存储
 * 3. 自动限制历史记录数量
 * 4. 支持按时间范围查询
 */
class HistoryManager : public QObject
{
    Q_OBJECT
public:
    // 删除拷贝构造函数和赋值运算符
    HistoryManager(const HistoryManager&) = delete;
    HistoryManager& operator=(const HistoryManager&) = delete;

    /**
     * @brief 获取单例实例
     * @return 全局唯一的HistoryManager实例
     */
    static HistoryManager& instance();

    /**
     * @brief 保存操作记录
     * @param operationType 操作类型(复制/移动/删除等)
     * @param source 源路径
     * @param destination 目标路径(删除操作可为空)
     * @param timestamp 操作时间(默认当前时间)
     */
    void saveOperation(const QString &operationType,
                       const QString &source,
                       const QString &destination = QString(),
                       const QDateTime &timestamp = QDateTime::currentDateTime());

    /**
     * @brief 获取所有历史记录
     * @return 按时间倒序排列的历史记录列表
     */
    QList<QVariantMap> operationHistory() const;

    /**
     * @brief 获取指定时间范围内的历史记录
     * @param start 开始时间
     * @param end 结束时间
     * @return 符合条件的记录列表
     */
    QList<QVariantMap> operationHistory(const QDateTime &start,
                                        const QDateTime &end) const;

    /**
     * @brief 清空历史记录
     */
    void clearHistory();

    /**
     * @brief 设置最大历史记录数量
     * @param max 最大数量
     */
    void setMaxHistoryItems(int max);

signals:
    /**
     * @brief 历史记录发生变化时触发
     */
    void historyChanged();

private:
    explicit HistoryManager(QObject *parent = nullptr);
    ~HistoryManager();

    QSettings m_settings;//配置存储对象
    int m_maxHistoryItems;

    static const QString HISTORY_GROUP;
    static const QString OPERATION_KEY;
    static const QString SOURCE_KEY;
    static const QString DEST_KEY;
    static const QString TIME_KEY;
};

#endif // HISTORYMANAGER_H
