#ifndef RENAMER_H
#define RENAMER_H

#include <QObject>
#include <QStringList>
#include <QRegularExpression>

/**
 * @brief 批量重命名工具 - 支持模板变量和自动编号
 *
 * 功能特点：
 * 1. 支持模板变量 (%n=序号, %d=日期, %f=原文件名等)
 * 2. 自动处理文件名冲突
 * 3. 支持正则表达式替换
 * 4. 提供预览功能
 */
class Renamer : public QObject
{
    Q_OBJECT
public:
    explicit Renamer(QObject *parent = nullptr);

    /**
     * @brief 设置重命名模板
     * @param pattern 模板字符串(支持变量替换)
     */
    void setPattern(const QString &pattern);

    /**
     * @brief 设置起始序号
     * @param start 起始值(默认1)
     */
    void setStartNumber(int start);

    /**
     * @brief 设置日期格式
     * @param format 日期格式(默认yyyy-MM-dd)
     */
    void setDateFormat(const QString &format);

    /**
     * @brief 生成新文件名(不实际重命名)
     * @param originalName 原文件名(不含路径)
     * @param index 文件序号
     * @return 生成的新文件名
     */
    QString generateNewName(const QString &originalName, int index = 0) const;

    /**
     * @brief 批量重命名文件
     * @param filePaths 文件路径列表
     * @return 重命名结果列表(新路径)
     */
    QStringList batchRename(const QStringList &filePaths);

    /**
     * @brief 预览重命名结果
     * @param filePaths 文件路径列表
     * @return 预览结果列表(原文件名 -> 新文件名)
     */
    QList<QPair<QString, QString >> preview(const QStringList &filePaths) const;

signals:
    /**
     * @brief 重命名进度信号
     * @param current 当前处理序号
     * @param total 总文件数
     */
    void progressChanged(int current, int total);

    /**
     * @brief 重命名完成信号
     * @param successCount 成功数量
     * @param failCount 失败数量
     */
    void renameFinished(int successCount, int failCount);

private:
    QString m_pattern;
    int m_startNumber;
    QString m_dateFormat;

    //替换模板变量
    QString replaceVariables(const QString &originalName, int index) const;
    QString ensureUniqueName(const QString &dirPath, const QString &newName) const;
};

#endif // RENAMER_H
