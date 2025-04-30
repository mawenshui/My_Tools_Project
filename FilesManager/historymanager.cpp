#include "historymanager.h"
#include <QDebug>

const QString HistoryManager::HISTORY_GROUP = "FileOperationHistory";
const QString HistoryManager::OPERATION_KEY = "operation";
const QString HistoryManager::SOURCE_KEY = "source";
const QString HistoryManager::DEST_KEY = "destination";
const QString HistoryManager::TIME_KEY = "timestamp";

HistoryManager& HistoryManager::instance()
{
    static HistoryManager instance;
    return instance;
}

HistoryManager::HistoryManager(QObject *parent)
    : QObject(parent),
      m_settings("MyCompany", "FileOperationTool"),
      m_maxHistoryItems(50)
{
    m_settings.beginGroup(HISTORY_GROUP);
}

HistoryManager::~HistoryManager()
{
    m_settings.endGroup();
}

void HistoryManager::saveOperation(const QString &operationType,
                                   const QString &source,
                                   const QString &destination,
                                   const QDateTime &timestamp)
{
    // 获取现有记录
    int count = m_settings.value("count", 0).toInt();
    QStringList keys = m_settings.childKeys();
    // 创建新记录
    QVariantMap record;
    record[OPERATION_KEY] = operationType;
    record[SOURCE_KEY] = source;
    record[DEST_KEY] = destination;
    record[TIME_KEY] = timestamp.toString(Qt::ISODate);
    // 保存记录
    QString key = QString("item%1").arg(count + 1);
    m_settings.setValue(key, record);
    // 更新计数
    m_settings.setValue("count", count + 1);
    // 清理旧记录
    if(count >= m_maxHistoryItems)
    {
        // 找出最早的5条记录删除
        QStringList allKeys;
        for(const QString &k : keys)
        {
            if(k.startsWith("item"))
            {
                allKeys << k;
            }
        }
        // 按数字排序(确保删除最早的)
        std::sort(allKeys.begin(), allKeys.end(), [](const QString & a, const QString & b)
        {
            return a.mid(4).toInt() < b.mid(4).toInt();
        });
        // 删除超出数量的记录
        for(int i = 0; i < allKeys.size() - m_maxHistoryItems + 1; ++i)
        {
            m_settings.remove(allKeys[i]);
        }
        m_settings.setValue("count", m_maxHistoryItems);
    }
    emit historyChanged();
}

QList<QVariantMap> HistoryManager::operationHistory() const
{
    QList<QVariantMap> history;
    QStringList keys = m_settings.childKeys();
    for(const QString &key : keys)
    {
        if(key.startsWith("item"))
        {
            QVariantMap record = m_settings.value(key).toMap();
            history.prepend(record); // 新记录在前
        }
    }
    return history;
}

QList<QVariantMap> HistoryManager::operationHistory(const QDateTime &start,
        const QDateTime &end) const
{
    QList<QVariantMap> result;
    QList<QVariantMap> allHistory = operationHistory();
    for(const QVariantMap &record : allHistory)
    {
        QDateTime dt = QDateTime::fromString(record[TIME_KEY].toString(), Qt::ISODate);
        if(dt >= start && dt <= end)
        {
            result.append(record);
        }
    }
    return result;
}

void HistoryManager::clearHistory()
{
    m_settings.remove(""); // 清除当前组的所有内容
    m_settings.setValue("count", 0);
    emit historyChanged();
}

void HistoryManager::setMaxHistoryItems(int max)
{
    if(max > 0 && max != m_maxHistoryItems)
    {
        m_maxHistoryItems = max;
        // 立即应用新限制
        int count = m_settings.value("count", 0).toInt();
        if(count > max)
        {
            QStringList keys;
            for(const QString &key : m_settings.childKeys())
            {
                if(key.startsWith("item"))
                {
                    keys << key;
                }
            }
            std::sort(keys.begin(), keys.end(), [](const QString & a, const QString & b)
            {
                return a.mid(4).toInt() < b.mid(4).toInt();
            });
            for(int i = 0; i < keys.size() - max; ++i)
            {
                m_settings.remove(keys[i]);
            }
            m_settings.setValue("count", max);
        }
    }
}
