#include "markdown_exporter.h"
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
#include <QtCore/QDebug>

/**
 * @brief 构造函数
 * @param parent 父对象指针
 */
MarkdownExporter::MarkdownExporter(QObject *parent)
    : QObject(parent)
{
}

/**
 * @brief 析构函数
 */
MarkdownExporter::~MarkdownExporter()
{
}

/**
 * @brief 设置分析结果
 * @param result 分析结果
 */
void MarkdownExporter::setAnalysisResult(const AnalysisResult &result)
{
    m_analysisResult = result;
}

/**
 * @brief 导出到单个Markdown文件
 * @param filePath 文件路径
 * @param contents 要导出的内容类型
 * @return 导出是否成功
 */
bool MarkdownExporter::exportToSingleFile(const QString &filePath, ExportContents contents)
{
    m_lastError.clear();
    if(filePath.isEmpty())
    {
        setError(tr("文件路径不能为空"));
        return false;
    }
    
    // 计算总的部分数
    int totalSections = 0;
    int currentSection = 0;
    
    if(contents & Overview)
    {
        totalSections++;
    }
    if(contents & DetailedTable)
    {
        totalSections++;
    }
    if(contents & FileList)
    {
        totalSections++;
    }
    if(contents & LanguageStats)
    {
        totalSections++;
    }
    
    QString markdownContent;
    QTextStream stream(&markdownContent);
    
    // 写入文件头部信息
    stream << "# CodeVisualization " << tr("分析结果导出") << "\n\n";
    stream << "**" << tr("导出时间") << ":** " << formatDateTime(QDateTime::currentDateTime()) << "\n\n";
    stream << "**" << tr("项目分析结果") << ":** " << m_analysisResult.getProjectPath() << "\n\n";
    stream << "---\n\n";
    
    // 导出概览信息
    if(contents & Overview)
    {
        emit progressUpdated(++currentSection, totalSections, tr("导出概览信息..."));
        stream << generateOverviewMarkdown();
        stream << "\n";
    }
    
    // 导出详细表格
    if(contents & DetailedTable)
    {
        emit progressUpdated(++currentSection, totalSections, tr("导出详细表格..."));
        stream << generateDetailedTableMarkdown();
        stream << "\n";
    }
    
    // 导出文件列表
    if(contents & FileList)
    {
        emit progressUpdated(++currentSection, totalSections, tr("导出文件列表..."));
        stream << generateFileListMarkdown();
        stream << "\n";
    }
    
    // 导出语言统计
    if(contents & LanguageStats)
    {
        emit progressUpdated(++currentSection, totalSections, tr("导出语言统计..."));
        stream << generateLanguageStatsMarkdown();
        stream << "\n";
    }
    
    bool success = writeMarkdownFile(filePath, markdownContent);
    emit exportCompleted(success, success ? tr("导出完成") : m_lastError);
    return success;
}

/**
 * @brief 导出到多个Markdown文件
 * @param dirPath 目录路径
 * @param filePrefix 文件前缀
 * @param contents 要导出的内容类型
 * @return 导出是否成功
 */
bool MarkdownExporter::exportToMultipleFiles(const QString &dirPath, const QString &filePrefix, ExportContents contents)
{
    m_lastError.clear();
    if(dirPath.isEmpty())
    {
        setError(tr("目录路径不能为空"));
        return false;
    }
    
    // 确保目录存在
    QDir dir(dirPath);
    if(!dir.exists())
    {
        if(!dir.mkpath(dirPath))
        {
            setError(tr("无法创建目录: %1").arg(dirPath));
            return false;
        }
    }
    
    QString prefix = filePrefix.isEmpty() ? "analysis" : filePrefix;
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    
    int totalFiles = 0;
    int currentFile = 0;
    
    // 计算总的文件数
    if(contents & Overview)
    {
        totalFiles++;
    }
    if(contents & DetailedTable)
    {
        totalFiles++;
    }
    if(contents & FileList)
    {
        totalFiles++;
    }
    if(contents & LanguageStats)
    {
        totalFiles++;
    }
    
    bool allSuccess = true;
    
    // 导出概览信息
    if(contents & Overview)
    {
        emit progressUpdated(++currentFile, totalFiles, tr("导出概览信息..."));
        QString fileName = QString("%1_overview_%2.md").arg(prefix, timestamp);
        QString filePath = dir.absoluteFilePath(fileName);
        QString content = generateOverviewMarkdown();
        if(!writeMarkdownFile(filePath, content))
        {
            allSuccess = false;
        }
    }
    
    // 导出详细表格
    if(contents & DetailedTable)
    {
        emit progressUpdated(++currentFile, totalFiles, tr("导出详细表格..."));
        QString fileName = QString("%1_detailed_%2.md").arg(prefix, timestamp);
        QString filePath = dir.absoluteFilePath(fileName);
        QString content = generateDetailedTableMarkdown();
        if(!writeMarkdownFile(filePath, content))
        {
            allSuccess = false;
        }
    }
    
    // 导出文件列表
    if(contents & FileList)
    {
        emit progressUpdated(++currentFile, totalFiles, tr("导出文件列表..."));
        QString fileName = QString("%1_filelist_%2.md").arg(prefix, timestamp);
        QString filePath = dir.absoluteFilePath(fileName);
        QString content = generateFileListMarkdown();
        if(!writeMarkdownFile(filePath, content))
        {
            allSuccess = false;
        }
    }
    
    // 导出语言统计
    if(contents & LanguageStats)
    {
        emit progressUpdated(++currentFile, totalFiles, tr("导出语言统计..."));
        QString fileName = QString("%1_language_stats_%2.md").arg(prefix, timestamp);
        QString filePath = dir.absoluteFilePath(fileName);
        QString content = generateLanguageStatsMarkdown();
        if(!writeMarkdownFile(filePath, content))
        {
            allSuccess = false;
        }
    }
    
    emit exportCompleted(allSuccess, allSuccess ? tr("导出完成") : tr("部分文件导出失败"));
    return allSuccess;
}

/**
 * @brief 获取最后的错误信息
 * @return 错误信息
 */
QString MarkdownExporter::getLastError() const
{
    return m_lastError;
}

/**
 * @brief 生成概览信息的Markdown内容
 * @return Markdown内容字符串
 */
QString MarkdownExporter::generateOverviewMarkdown() const
{
    QString content;
    QTextStream stream(&content);
    
    stream << "## " << tr("概览信息") << "\n\n";
    
    // 基本信息表格
    stream << "| " << tr("项目") << " | " << tr("值") << " |\n";
    stream << "|---|---|\n";
    
    int totalFiles = m_analysisResult.getTotalFiles();
    int totalLines = m_analysisResult.getTotalLines();
    int codeLines = m_analysisResult.getTotalCodeLines();
    int commentLines = m_analysisResult.getTotalCommentLines();
    int blankLines = m_analysisResult.getTotalBlankLines();
    
    stream << "| " << tr("项目路径") << " | " << escapeMarkdownText(m_analysisResult.getProjectPath()) << " |\n";
    stream << "| " << tr("分析时间") << " | " << formatDateTime(m_analysisResult.getAnalysisStartTime()) << " |\n";
    stream << "| " << tr("总文件数") << " | " << formatNumber(totalFiles) << " |\n";
    stream << "| " << tr("总行数") << " | " << formatNumber(totalLines) << " |\n";
    stream << "| " << tr("代码行数") << " | " << formatNumber(codeLines) << " |\n";
    stream << "| " << tr("注释行数") << " | " << formatNumber(commentLines) << " |\n";
    stream << "| " << tr("空行数") << " | " << formatNumber(blankLines) << " |\n";
    
    // 计算百分比
    if(totalLines > 0)
    {
        double codePercent = (double)codeLines / totalLines * 100.0;
        double commentPercent = (double)commentLines / totalLines * 100.0;
        double blankPercent = (double)blankLines / totalLines * 100.0;
        
        stream << "| " << tr("代码行占比") << " | " << formatPercentage(codePercent) << " |\n";
        stream << "| " << tr("注释行占比") << " | " << formatPercentage(commentPercent) << " |\n";
        stream << "| " << tr("空行占比") << " | " << formatPercentage(blankPercent) << " |\n";
    }
    
    stream << "| " << tr("编程语言数量") << " | " << formatNumber(m_analysisResult.getLanguageCount()) << " |\n";
    
    return content;
}

/**
 * @brief 生成详细表格的Markdown内容
 * @return Markdown内容字符串
 */
QString MarkdownExporter::generateDetailedTableMarkdown() const
{
    QString content;
    QTextStream stream(&content);
    
    stream << "## " << tr("详细统计表格") << "\n\n";
    
    // 表格头部
    stream << "| " << tr("语言") << " | " << tr("文件数") << " | " << tr("总行数") << " | "
           << tr("代码行数") << " | " << tr("注释行数") << " | " << tr("空行数") << " |\n";
    stream << "|---|---|---|---|---|---|\n";
    
    // 获取语言文件数量和详细统计数据
    QMap<QString, int> languageFileCount = m_analysisResult.getLanguageFileCount();
    const QVector<FileStatistics>& fileStats = m_analysisResult.getFileStatistics();
    
    // 按语言汇总统计信息
    QMap<QString, QVector<int>> languageDetails; // [totalLines, codeLines, commentLines, blankLines]
    
    for(const FileStatistics &fileStat : fileStats)
    {
        if(!languageDetails.contains(fileStat.language))
        {
            languageDetails[fileStat.language] = QVector<int>(4, 0);
        }
        languageDetails[fileStat.language][0] += fileStat.totalLines;
        languageDetails[fileStat.language][1] += fileStat.codeLines;
        languageDetails[fileStat.language][2] += fileStat.commentLines;
        languageDetails[fileStat.language][3] += fileStat.blankLines;
    }
    
    for(auto it = languageFileCount.begin(); it != languageFileCount.end(); ++it)
    {
        const QString &language = it.key();
        int fileCount = it.value();
        
        stream << "| " << escapeMarkdownText(language) << " | ";
        stream << formatNumber(fileCount) << " | ";
        
        if(languageDetails.contains(language))
        {
            const QVector<int> &details = languageDetails[language];
            stream << formatNumber(details[0]) << " | "; // 总行数
            stream << formatNumber(details[1]) << " | "; // 代码行数
            stream << formatNumber(details[2]) << " | "; // 注释行数
            stream << formatNumber(details[3]) << " |\n"; // 空行数
        }
        else
        {
            stream << "0 | 0 | 0 | 0 |\n";
        }
    }
    
    return content;
}

/**
 * @brief 生成文件列表的Markdown内容
 * @return Markdown内容字符串
 */
QString MarkdownExporter::generateFileListMarkdown() const
{
    QString content;
    QTextStream stream(&content);
    
    stream << "## " << tr("文件列表") << "\n\n";
    
    // 表格头部
    stream << "| " << tr("文件路径") << " | " << tr("语言") << " | " << tr("总行数") << " | "
           << tr("代码行数") << " | " << tr("注释行数") << " | " << tr("空行数") << " | "
           << tr("文件大小") << " | " << tr("代码密度") << " |\n";
    stream << "|---|---|---|---|---|---|---|---|\n";
    
    // 获取文件统计数据
    const QVector<FileStatistics>& fileStats = m_analysisResult.getFileStatistics();

    for(const FileStatistics &fileStat : fileStats)
    {
        double codeDensity = fileStat.totalLines > 0 ? (double)fileStat.codeLines / fileStat.totalLines * 100.0 : 0.0;

        stream << "| " << escapeMarkdownText(fileStat.filePath) << " | ";
        stream << escapeMarkdownText(fileStat.language) << " | ";
        stream << formatNumber(fileStat.totalLines) << " | ";
        stream << formatNumber(fileStat.codeLines) << " | ";
        stream << formatNumber(fileStat.commentLines) << " | ";
        stream << formatNumber(fileStat.blankLines) << " | ";
        stream << formatFileSize(fileStat.fileSize) << " | ";
        stream << formatPercentage(codeDensity) << " |\n";
    }
    
    return content;
}

/**
 * @brief 生成语言统计的Markdown内容
 * @return Markdown内容字符串
 */
QString MarkdownExporter::generateLanguageStatsMarkdown() const
{
    QString content;
    QTextStream stream(&content);
    
    stream << "## " << tr("语言统计") << "\n\n";
    
    // 表格头部
    stream << "| " << tr("语言") << " | " << tr("文件数") << " |\n";
    stream << "|---|---|\n";
    
    // 获取语言文件数量统计数据
    QMap<QString, int> languageFileCount = m_analysisResult.getLanguageFileCount();
    
    for(auto it = languageFileCount.begin(); it != languageFileCount.end(); ++it)
    {
        const QString &language = it.key();
        int fileCount = it.value();
        
        stream << "| " << escapeMarkdownText(language) << " | ";
        stream << formatNumber(fileCount) << " |\n";
    }
    
    return content;
}

/**
 * @brief 写入Markdown文件
 * @param filePath 文件路径
 * @param content 文件内容
 * @return 写入是否成功
 */
bool MarkdownExporter::writeMarkdownFile(const QString &filePath, const QString &content)
{
    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        setError(tr("无法打开文件进行写入: %1").arg(filePath));
        return false;
    }
    
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    stream << content;
    
    file.close();
    
    if(file.error() != QFile::NoError)
    {
        setError(tr("写入文件时发生错误: %1").arg(file.errorString()));
        return false;
    }
    
    return true;
}

/**
 * @brief 转义Markdown特殊字符
 * @param text 原始文本
 * @return 转义后的文本
 */
QString MarkdownExporter::escapeMarkdownText(const QString &text) const
{
    QString escaped = text;
    
    // 转义Markdown特殊字符
    escaped.replace("\\", "\\\\");  // 反斜杠
    escaped.replace("`", "\\`");      // 反引号
    escaped.replace("*", "\\*");      // 星号
    escaped.replace("_", "\\_");      // 下划线
    escaped.replace("{", "\\{");      // 左大括号
    escaped.replace("}", "\\}");      // 右大括号
    escaped.replace("[", "\\[");      // 左方括号
    escaped.replace("]", "\\]");      // 右方括号
    escaped.replace("(", "\\(");      // 左圆括号
    escaped.replace(")", "\\)");      // 右圆括号
    escaped.replace("#", "\\#");      // 井号
    escaped.replace("+", "\\+");      // 加号
    escaped.replace("-", "\\-");      // 减号
    escaped.replace(".", "\\.");      // 点号
    escaped.replace("!", "\\!");      // 感叹号
    escaped.replace("|", "\\|");      // 管道符
    
    return escaped;
}

/**
 * @brief 格式化文件大小
 * @param bytes 字节数
 * @return 格式化后的文件大小字符串
 */
QString MarkdownExporter::formatFileSize(qint64 bytes) const
{
    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;
    const qint64 GB = MB * 1024;
    
    if(bytes >= GB)
    {
        return QString("%1 GB").arg(QString::number((double)bytes / GB, 'f', 2));
    }
    else if(bytes >= MB)
    {
        return QString("%1 MB").arg(QString::number((double)bytes / MB, 'f', 2));
    }
    else if(bytes >= KB)
    {
        return QString("%1 KB").arg(QString::number((double)bytes / KB, 'f', 2));
    }
    else
    {
        return QString("%1 B").arg(bytes);
    }
}

/**
 * @brief 格式化数字
 * @param number 数字
 * @return 格式化后的数字字符串
 */
QString MarkdownExporter::formatNumber(int number) const
{
    // 返回不带千位分隔符的数字字符串
    return QString::number(number);
}

/**
 * @brief 格式化百分比
 * @param value 百分比值
 * @return 格式化后的百分比字符串
 */
QString MarkdownExporter::formatPercentage(double value) const
{
    return QString("%1%").arg(QString::number(value, 'f', 2));
}

/**
 * @brief 格式化日期时间
 * @param dateTime 日期时间
 * @return 格式化后的日期时间字符串
 */
QString MarkdownExporter::formatDateTime(const QDateTime &dateTime) const
{
    return dateTime.toString("yyyy-MM-dd hh:mm:ss");
}

/**
 * @brief 设置错误信息
 * @param error 错误信息
 */
void MarkdownExporter::setError(const QString &error)
{
    m_lastError = error;
    qWarning() << "MarkdownExporter Error:" << error;
}