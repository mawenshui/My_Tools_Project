#ifndef MARKDOWN_EXPORTER_H
#define MARKDOWN_EXPORTER_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include "../models/analysis_result.h"
#include "../models/file_statistics.h"

/**
 * @brief Markdown导出器类
 * 
 * 负责将分析结果导出为Markdown格式
 * 支持多种内容类型的导出
 * 支持单文件和多文件导出模式
 * 
 * @author CodeVisualization Team
 * @date 2025-01-31
 * @version 1.0.0
 */
class MarkdownExporter : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 导出内容类型枚举
     */
    enum ExportContent {
        Overview = 0x01,        ///< 概览信息
        DetailedTable = 0x02,   ///< 详细表格
        FileList = 0x04,        ///< 文件列表
        LanguageStats = 0x08    ///< 语言统计
    };
    Q_DECLARE_FLAGS(ExportContents, ExportContent)
    
    /**
     * @brief 导出模式枚举
     */
    enum ExportMode {
        SingleFile,    ///< 单文件模式
        MultipleFiles  ///< 多文件模式
    };
    
    /**
     * @brief 构造函数
     * @param parent 父对象指针
     */
    explicit MarkdownExporter(QObject *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~MarkdownExporter();
    
    /**
     * @brief 设置分析结果
     * @param result 分析结果
     */
    void setAnalysisResult(const AnalysisResult &result);
    
    /**
     * @brief 导出到单个Markdown文件
     * @param filePath 文件路径
     * @param contents 要导出的内容类型
     * @return 导出是否成功
     */
    bool exportToSingleFile(const QString &filePath, ExportContents contents);
    
    /**
     * @brief 导出到多个Markdown文件
     * @param dirPath 目录路径
     * @param filePrefix 文件前缀
     * @param contents 要导出的内容类型
     * @return 导出是否成功
     */
    bool exportToMultipleFiles(const QString &dirPath, const QString &filePrefix, ExportContents contents);
    
    /**
     * @brief 获取最后的错误信息
     * @return 错误信息
     */
    QString getLastError() const;

signals:
    /**
     * @brief 导出进度更新信号
     * @param current 当前进度
     * @param total 总进度
     * @param message 进度消息
     */
    void progressUpdated(int current, int total, const QString &message);
    
    /**
     * @brief 导出完成信号
     * @param success 是否成功
     * @param message 完成消息
     */
    void exportCompleted(bool success, const QString &message);

private:
    /**
     * @brief 生成概览信息的Markdown内容
     * @return Markdown内容字符串
     */
    QString generateOverviewMarkdown() const;
    
    /**
     * @brief 生成详细表格的Markdown内容
     * @return Markdown内容字符串
     */
    QString generateDetailedTableMarkdown() const;
    
    /**
     * @brief 生成文件列表的Markdown内容
     * @return Markdown内容字符串
     */
    QString generateFileListMarkdown() const;
    
    /**
     * @brief 生成语言统计的Markdown内容
     * @return Markdown内容字符串
     */
    QString generateLanguageStatsMarkdown() const;
    
    /**
     * @brief 写入Markdown文件
     * @param filePath 文件路径
     * @param content 文件内容
     * @return 写入是否成功
     */
    bool writeMarkdownFile(const QString &filePath, const QString &content);
    
    /**
     * @brief 转义Markdown特殊字符
     * @param text 原始文本
     * @return 转义后的文本
     */
    QString escapeMarkdownText(const QString &text) const;
    
    /**
     * @brief 格式化文件大小
     * @param bytes 字节数
     * @return 格式化后的文件大小字符串
     */
    QString formatFileSize(qint64 bytes) const;
    
    /**
     * @brief 格式化数字
     * @param number 数字
     * @return 格式化后的数字字符串
     */
    QString formatNumber(int number) const;
    
    /**
     * @brief 格式化百分比
     * @param value 百分比值
     * @return 格式化后的百分比字符串
     */
    QString formatPercentage(double value) const;
    
    /**
     * @brief 格式化日期时间
     * @param dateTime 日期时间
     * @return 格式化后的日期时间字符串
     */
    QString formatDateTime(const QDateTime &dateTime) const;
    
    /**
     * @brief 设置错误信息
     * @param error 错误信息
     */
    void setError(const QString &error);

private:
    AnalysisResult m_analysisResult;        ///< 分析结果
    QString m_lastError;                    ///< 最后的错误信息
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MarkdownExporter::ExportContents)

#endif // MARKDOWN_EXPORTER_H