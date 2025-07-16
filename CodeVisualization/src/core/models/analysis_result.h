#ifndef ANALYSIS_RESULT_H
#define ANALYSIS_RESULT_H

#include <QtCore/QVector>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QDateTime>
#include "file_statistics.h"

/**
 * @brief 分析结果数据结构
 * 
 * 存储整个项目的代码分析结果，包括所有文件的统计信息
 * 和汇总数据
 * 
 * @author CodeVisualization Team
 * @date 2024-01-01
 * @version 1.0.0
 */
class AnalysisResult
{
public:
    /**
     * @brief 默认构造函数
     */
    AnalysisResult();
    
    /**
     * @brief 析构函数
     */
    ~AnalysisResult();
    
    /**
     * @brief 添加文件统计信息
     * @param fileStats 文件统计信息
     */
    void addFileStatistics(const FileStatistics &fileStats);
    
    /**
     * @brief 获取所有文件统计信息
     * @return 文件统计信息列表
     */
    const QVector<FileStatistics>& getFileStatistics() const;
    
    /**
     * @brief 获取总行数
     * @return 总行数
     */
    int getTotalLines() const;
    
    /**
     * @brief 获取总代码行数
     * @return 总代码行数
     */
    int getTotalCodeLines() const;
    
    /**
     * @brief 获取总注释行数
     * @return 总注释行数
     */
    int getTotalCommentLines() const;
    
    /**
     * @brief 获取总空白行数
     * @return 总空白行数
     */
    int getTotalBlankLines() const;
    
    /**
     * @brief 获取文件总数
     * @return 文件总数
     */
    int getTotalFiles() const;
    
    /**
     * @brief 获取语言统计信息
     * @return 语言名称到统计信息的映射
     */
    QMap<QString, int> getLanguageStatistics() const;
    
    /**
     * @brief 获取支持的语言数量
     * @return 语言数量
     */
    int getLanguageCount() const;
    
    /**
     * @brief 获取项目路径
     * @return 项目路径
     */
    const QString& getProjectPath() const;
    
    /**
     * @brief 设置项目路径
     * @param path 项目路径
     */
    void setProjectPath(const QString &path);
    
    /**
     * @brief 获取分析开始时间
     * @return 分析开始时间
     */
    const QDateTime& getAnalysisStartTime() const;
    
    /**
     * @brief 设置分析开始时间
     * @param startTime 分析开始时间
     */
    void setAnalysisStartTime(const QDateTime &startTime);
    
    /**
     * @brief 获取分析结束时间
     * @return 分析结束时间
     */
    const QDateTime& getAnalysisEndTime() const;
    
    /**
     * @brief 设置分析结束时间
     * @param endTime 分析结束时间
     */
    void setAnalysisEndTime(const QDateTime &endTime);
    
    /**
     * @brief 获取分析耗时（毫秒）
     * @return 分析耗时
     */
    qint64 getAnalysisDuration() const;
    
    /**
     * @brief 清空所有数据
     */
    void clear();
    
    /**
     * @brief 检查结果是否为空
     * @return 是否为空
     */
    bool isEmpty() const;
    
    /**
     * @brief 获取平均代码密度
     * @return 平均代码密度百分比
     */
    double getAverageCodeDensity() const;
    
    /**
     * @brief 获取平均注释密度
     * @return 平均注释密度百分比
     */
    double getAverageCommentDensity() const;
    
private:
    QVector<FileStatistics> m_fileStatistics;   ///< 文件统计信息列表
    QString m_projectPath;                      ///< 项目路径
    QDateTime m_analysisStartTime;              ///< 分析开始时间
    QDateTime m_analysisEndTime;                ///< 分析结束时间
    
    // 缓存的汇总数据
    mutable int m_totalLines;                   ///< 总行数缓存
    mutable int m_totalCodeLines;               ///< 总代码行数缓存
    mutable int m_totalCommentLines;            ///< 总注释行数缓存
    mutable int m_totalBlankLines;              ///< 总空白行数缓存
    mutable bool m_cacheValid;                  ///< 缓存是否有效
    
    /**
     * @brief 更新缓存数据
     */
    void updateCache() const;
};

#endif // ANALYSIS_RESULT_H