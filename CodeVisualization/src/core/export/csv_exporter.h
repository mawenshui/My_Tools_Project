#ifndef CSV_EXPORTER_H
#define CSV_EXPORTER_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>
#include <QtCore/QTextCodec>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include <QtCore/QLocale>
#include "../models/analysis_result.h"
#include "../models/file_statistics.h"

/**
 * @brief CSV导出器类
 * 
 * 负责将分析结果导出为CSV格式
 * 支持多种内容类型的导出
 * 支持单文件和多文件导出模式
 * 
 * @author CodeVisualization Team
 * @date 2024-12-01
 * @version 1.0.0
 */
class CSVExporter : public QObject
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
    explicit CSVExporter(QObject *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~CSVExporter();
    
    /**
     * @brief 设置分析结果
     * @param result 分析结果
     */
    void setAnalysisResult(const AnalysisResult &result);
    
    /**
     * @brief 设置文件编码
     * @param encoding 编码格式（如UTF-8、GBK等）
     */
    void setEncoding(const QString &encoding);
    
    /**
     * @brief 导出到单个CSV文件
     * @param filePath 文件路径
     * @param contents 要导出的内容类型
     * @return 导出是否成功
     */
    bool exportToSingleFile(const QString &filePath, ExportContents contents);
    
    /**
     * @brief 导出到多个CSV文件
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
     * @brief 导出进度信号
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
     * @brief 生成概览信息CSV内容
     * @return CSV内容字符串
     */
    QString generateOverviewCSV() const;
    
    /**
     * @brief 生成详细表格CSV内容
     * @return CSV内容字符串
     */
    QString generateDetailedTableCSV() const;
    
    /**
     * @brief 生成文件列表CSV内容
     * @return CSV内容字符串
     */
    QString generateFileListCSV() const;
    
    /**
     * @brief 生成语言统计CSV内容
     * @return CSV内容字符串
     */
    QString generateLanguageStatsCSV() const;
    
    /**
     * @brief 写入CSV文件
     * @param filePath 文件路径
     * @param content 文件内容
     * @return 写入是否成功
     */
    bool writeCSVFile(const QString &filePath, const QString &content);
    
    /**
     * @brief 获取文本编码器
     * @return 文本编码器指针
     */
    QTextCodec* getTextCodec() const;
    
    /**
     * @brief 转义CSV字段
     * @param field 字段内容
     * @return 转义后的字段
     */
    QString escapeCSVField(const QString &field) const;
    
    /**
     * @brief 格式化文件大小
     * @param bytes 字节数
     * @return 格式化后的大小字符串
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
     * @brief 格式化时间
     * @param dateTime 时间对象
     * @return 格式化后的时间字符串
     */
    QString formatDateTime(const QDateTime &dateTime) const;
    
    /**
     * @brief 设置错误信息
     * @param error 错误信息
     */
    void setError(const QString &error);

private:
    AnalysisResult m_analysisResult;    ///< 分析结果
    QString m_lastError;                ///< 最后的错误信息
    QString m_encoding;                 ///< 文件编码格式
};

Q_DECLARE_OPERATORS_FOR_FLAGS(CSVExporter::ExportContents)

#endif // CSV_EXPORTER_H