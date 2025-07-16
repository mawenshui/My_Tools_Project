#ifndef FILE_STATISTICS_H
#define FILE_STATISTICS_H

#include <QtCore/QString>
#include <QtCore/QDateTime>

/**
 * @brief 文件统计信息结构
 * 
 * 存储单个文件的代码统计信息，包括各种行数统计
 * 和文件基本信息
 * 
 * @author CodeVisualization Team
 * @date 2024-01-01
 * @version 1.0.0
 */
struct FileStatistics
{
    QString filePath;           ///< 文件路径
    QString fileName;           ///< 文件名
    QString language;           ///< 编程语言
    QString extension;          ///< 文件扩展名
    
    int totalLines;             ///< 总行数
    int codeLines;              ///< 代码行数
    int commentLines;           ///< 注释行数
    int blankLines;             ///< 空白行数
    
    qint64 fileSize;            ///< 文件大小（字节）
    QDateTime lastModified;     ///< 最后修改时间
    
    /**
     * @brief 默认构造函数
     */
    FileStatistics();
    
    /**
     * @brief 构造函数
     * @param path 文件路径
     */
    explicit FileStatistics(const QString &path);
    
    /**
     * @brief 重置统计信息
     */
    void reset();
    
    /**
     * @brief 检查统计信息是否有效
     * @return 是否有效
     */
    bool isValid() const;
    
    /**
     * @brief 计算代码密度（代码行数/总行数）
     * @return 代码密度百分比
     */
    double getCodeDensity() const;
    
    /**
     * @brief 计算注释密度（注释行数/总行数）
     * @return 注释密度百分比
     */
    double getCommentDensity() const;
};

#endif // FILE_STATISTICS_H