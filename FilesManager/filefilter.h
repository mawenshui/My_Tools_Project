#ifndef FILEFILTER_H
#define FILEFILTER_H

#include <QObject>
#include <QStringList>
#include <QRegularExpression>


/**
 * @brief 文件过滤器 - 提供灵活的文件名匹配功能
 *
 * 功能特点：
 * 1. 支持通配符(* ?)和正则表达式
 * 2. 多模式组合过滤(用分号分隔)
 * 3. 大小写敏感选项
 * 4. 线程安全设计
 */
class FileFilter : public QObject
{
    Q_OBJECT
public:
    explicit FileFilter(QObject *parent = nullptr);

    /**
     * @brief 设置是否区分大小写
     * @param sensitive 是否区分大小写
     */
    static void setCaseSensitive(bool sensitive);

    /**
     * @brief 检查单个文件名是否匹配过滤规则
     * @param fileName 待检查的文件名
     * @param pattern 过滤规则(支持通配符和正则)
     * @return 是否匹配
     */
    static bool match(const QString &fileName,
                      const QString &pattern);

    /**
     * @brief 批量过滤文件列表
     * @param files 待过滤的文件列表
     * @param pattern 过滤规则
     * @return 匹配的文件列表
     */
    static QStringList filterFiles(const QStringList &files,
                                   const QString &pattern);

private:
    //创建正则表达式对象
    static QRegularExpression createRegExp(const QString &pattern);
    static bool s_caseSensitive;
};

#endif // FILEFILTER_H
