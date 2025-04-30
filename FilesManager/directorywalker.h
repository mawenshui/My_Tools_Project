#ifndef DIRECTORYWALKER_H
#define DIRECTORYWALKER_H

#include <QObject>
#include <QStringList>
#include <QRegExp>

/**
 * @brief 目录遍历器 - 递归搜索目录中的文件
 *
 * 功能特点：
 * 1. 支持递归/非递归两种搜索模式
 * 2. 支持通配符和正则表达式过滤
 * 3. 实时反馈搜索进度
 * 4. 线程安全设计
 */
class DirectoryWalker : public QObject
{
    Q_OBJECT
public:
    explicit DirectoryWalker(QObject *parent = nullptr);

    /**
     * @brief 查找匹配的文件
     * @param path 搜索根路径
     * @param recursive 是否递归子目录
     * @param filter 过滤规则(支持*.txt形式或正则表达式)
     * @return 匹配的文件路径列表
     */
    QStringList findFiles(const QString &path,
                          bool recursive,
                          const QString &filter = "") const;

signals:
    /**
     * @brief 发现匹配文件时触发
     * @param filePath 文件完整路径
     */
    void fileFound(const QString &filePath) const;

    /**
     * @brief 进度变化时触发
     * @param percent 进度百分比(0-100)
     */
    void progressChanged(int percent) const;

private:
    /**
     * @brief 检查文件名是否匹配过滤规则
     * @param fileName 待检查的文件名
     * @param filter 过滤规则
     * @return 是否匹配
     */
    bool matchesFilter(const QString &fileName,
                       const QString &filter) const;
};

#endif // DIRECTORYWALKER_H
