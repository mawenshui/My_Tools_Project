#include "csv_exporter.h"
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
#include <QtCore/QDebug>

CSVExporter::CSVExporter(QObject *parent)
    : QObject(parent)
    , m_encoding("UTF-8")  // 默认使用UTF-8编码
{
}

CSVExporter::~CSVExporter()
{
}

void CSVExporter::setAnalysisResult(const AnalysisResult &result)
{
    m_analysisResult = result;
}

void CSVExporter::setEncoding(const QString &encoding)
{
    m_encoding = encoding.isEmpty() ? "UTF-8" : encoding;
}

bool CSVExporter::exportToSingleFile(const QString &filePath, ExportContents contents)
{
    m_lastError.clear();
    if(filePath.isEmpty())
    {
        setError(tr("文件路径不能为空"));
        return false;
    }
    // 确保目录存在
    QFileInfo fileInfo(filePath);
    QDir dir = fileInfo.absoluteDir();
    if(!dir.exists() && !dir.mkpath("."))
    {
        setError(tr("无法创建目录: %1").arg(dir.absolutePath()));
        return false;
    }
    QString csvContent;
    QTextStream stream(&csvContent);
    // 添加文件头信息
    stream << "# CodeVisualization " << tr("分析结果导出") << "\n";
    stream << "# " << tr("导出时间") << ": " << formatDateTime(QDateTime::currentDateTime()) << "\n";
    stream << "# " << tr("项目分析结果") << " - " << escapeCSVField(m_analysisResult.getProjectPath()) << "\n";
    stream << "\n";
    int totalSections = 0;
    int currentSection = 0;
    // 计算总的导出部分数
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
    // 导出概览信息
    if(contents & Overview)
    {
        emit progressUpdated(++currentSection, totalSections, tr("导出概览信息..."));
        stream << "\n";
        stream << "# " << tr("概览信息") << "\n";
        stream << generateOverviewCSV();
        stream << "\n";
    }
    // 导出详细表格
    if(contents & DetailedTable)
    {
        emit progressUpdated(++currentSection, totalSections, tr("导出详细表格..."));
        stream << "\n";
        stream << "# " << tr("详细统计表格") << "\n";
        stream << generateDetailedTableCSV();
        stream << "\n";
    }
    // 导出文件列表
    if(contents & FileList)
    {
        emit progressUpdated(++currentSection, totalSections, tr("导出文件列表..."));
        stream << "\n";
        stream << "# " << tr("文件列表") << "\n";
        stream << generateFileListCSV();
        stream << "\n";
    }
    // 导出语言统计
    if(contents & LanguageStats)
    {
        emit progressUpdated(++currentSection, totalSections, tr("导出语言统计..."));
        stream << "\n";
        stream << "# " << tr("语言统计") << "\n";
        stream << generateLanguageStatsCSV();
        stream << "\n";
    }
    bool success = writeCSVFile(filePath, csvContent);
    emit exportCompleted(success, success ? tr("导出完成") : m_lastError);
    return success;
}

bool CSVExporter::exportToMultipleFiles(const QString &dirPath, const QString &filePrefix, ExportContents contents)
{
    m_lastError.clear();
    if(dirPath.isEmpty())
    {
        setError(tr("目录路径不能为空"));
        return false;
    }
    // 确保目录存在
    QDir dir(dirPath);
    if(!dir.exists() && !dir.mkpath("."))
    {
        setError(tr("无法创建目录: %1").arg(dirPath));
        return false;
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
        QString fileName = QString("%1_overview_%2.csv").arg(prefix, timestamp);
        QString filePath = dir.absoluteFilePath(fileName);
        QString content = generateOverviewCSV();
        if(!writeCSVFile(filePath, content))
        {
            allSuccess = false;
        }
    }
    // 导出详细表格
    if(contents & DetailedTable)
    {
        emit progressUpdated(++currentFile, totalFiles, tr("导出详细表格..."));
        QString fileName = QString("%1_detailed_%2.csv").arg(prefix, timestamp);
        QString filePath = dir.absoluteFilePath(fileName);
        QString content = generateDetailedTableCSV();
        if(!writeCSVFile(filePath, content))
        {
            allSuccess = false;
        }
    }
    // 导出文件列表
    if(contents & FileList)
    {
        emit progressUpdated(++currentFile, totalFiles, tr("导出文件列表..."));
        QString fileName = QString("%1_filelist_%2.csv").arg(prefix, timestamp);
        QString filePath = dir.absoluteFilePath(fileName);
        QString content = generateFileListCSV();
        if(!writeCSVFile(filePath, content))
        {
            allSuccess = false;
        }
    }
    // 导出语言统计
    if(contents & LanguageStats)
    {
        emit progressUpdated(++currentFile, totalFiles, tr("导出语言统计..."));
        QString fileName = QString("%1_languages_%2.csv").arg(prefix, timestamp);
        QString filePath = dir.absoluteFilePath(fileName);
        QString content = generateLanguageStatsCSV();
        if(!writeCSVFile(filePath, content))
        {
            allSuccess = false;
        }
    }
    emit exportCompleted(allSuccess, allSuccess ? tr("导出完成") : m_lastError);
    return allSuccess;
}

QString CSVExporter::getLastError() const
{
    return m_lastError;
}

QString CSVExporter::generateOverviewCSV() const
{
    QString content;
    QTextStream stream(&content);
    // CSV头部
    stream << tr("项目") << "," << tr("值") << "\n";
    // 基本信息
    int totalFiles = m_analysisResult.getTotalFiles();
    int totalLines = m_analysisResult.getTotalLines();
    int codeLines = m_analysisResult.getTotalCodeLines();
    int commentLines = m_analysisResult.getTotalCommentLines();
    int blankLines = m_analysisResult.getTotalBlankLines();
    // 计算总文件大小
    qint64 totalSize = 0;
    for(const FileStatistics &fileStats : m_analysisResult.getFileStatistics())
    {
        totalSize += fileStats.fileSize;
    }
    stream << escapeCSVField(tr("项目路径")) << "," << escapeCSVField(m_analysisResult.getProjectPath()) << "\n";
    stream << escapeCSVField(tr("分析时间")) << "," << escapeCSVField(formatDateTime(m_analysisResult.getAnalysisStartTime())) << "\n";
    stream << escapeCSVField(tr("总文件数")) << "," << formatNumber(totalFiles) << "\n";
    stream << escapeCSVField(tr("总行数")) << "," << formatNumber(totalLines) << "\n";
    stream << escapeCSVField(tr("代码行数")) << "," << formatNumber(codeLines) << "\n";
    stream << escapeCSVField(tr("注释行数")) << "," << formatNumber(commentLines) << "\n";
    stream << escapeCSVField(tr("空行数")) << "," << formatNumber(blankLines) << "\n";
    stream << escapeCSVField(tr("总文件大小")) << "," << escapeCSVField(formatFileSize(totalSize)) << "\n";
    // 计算百分比
    if(totalLines > 0)
    {
        double codePercent = (double)codeLines / totalLines * 100;
        double commentPercent = (double)commentLines / totalLines * 100;
        double blankPercent = (double)blankLines / totalLines * 100;
        stream << escapeCSVField(tr("代码行占比")) << "," << formatPercentage(codePercent) << "\n";
        stream << escapeCSVField(tr("注释行占比")) << "," << formatPercentage(commentPercent) << "\n";
        stream << escapeCSVField(tr("空行占比")) << "," << formatPercentage(blankPercent) << "\n";
    }
    // 语言数量
    stream << escapeCSVField(tr("编程语言数量")) << "," << formatNumber(m_analysisResult.getLanguageCount()) << "\n";
    return content;
}

QString CSVExporter::generateDetailedTableCSV() const
{
    QString content;
    QTextStream stream(&content);
    // CSV头部
    stream << tr("语言") << "," << tr("文件数") << "," << tr("总行数") << "," << tr("代码行数") << "," << tr("注释行数") << "," << tr("空行数") << "," << tr("文件大小") << "," << tr("平均行数") << "," << tr("代码密度") << "\n";
    // 计算详细的语言统计
    QMap<QString, int> languageFileCount;
    QMap<QString, int> languageTotalLines;
    QMap<QString, int> languageCodeLines;
    QMap<QString, int> languageCommentLines;
    QMap<QString, int> languageBlankLines;
    QMap<QString, qint64> languageTotalSize;
    // 遍历文件统计，按语言汇总
    for(const FileStatistics &fileStats : m_analysisResult.getFileStatistics())
    {
        const QString &language = fileStats.language;
        if(!language.isEmpty())
        {
            languageFileCount[language]++;
            languageTotalLines[language] += fileStats.totalLines;
            languageCodeLines[language] += fileStats.codeLines;
            languageCommentLines[language] += fileStats.commentLines;
            languageBlankLines[language] += fileStats.blankLines;
            languageTotalSize[language] += fileStats.fileSize;
        }
    }
    // 输出每种语言的统计
    for(auto it = languageFileCount.begin(); it != languageFileCount.end(); ++it)
    {
        const QString &language = it.key();
        int fileCount = it.value();
        int totalLines = languageTotalLines[language];
        int codeLines = languageCodeLines[language];
        int commentLines = languageCommentLines[language];
        int blankLines = languageBlankLines[language];
        qint64 totalSize = languageTotalSize[language];
        double avgLines = fileCount > 0 ? (double)totalLines / fileCount : 0;
        double codeDensity = totalLines > 0 ? (double)codeLines / totalLines * 100 : 0;
        stream << escapeCSVField(language) << ",";
        stream << formatNumber(fileCount) << ",";
        stream << formatNumber(totalLines) << ",";
        stream << formatNumber(codeLines) << ",";
        stream << formatNumber(commentLines) << ",";
        stream << formatNumber(blankLines) << ",";
        stream << escapeCSVField(formatFileSize(totalSize)) << ",";
        stream << QString::number(avgLines, 'f', 1) << ",";
        stream << formatPercentage(codeDensity) << "\n";
    }
    return content;
}

QString CSVExporter::generateFileListCSV() const
{
    QString content;
    QTextStream stream(&content);
    // CSV头部
    stream << tr("文件路径") << "," << tr("语言") << "," << tr("总行数") << "," << tr("代码行数") << "," << tr("注释行数") << "," << tr("空行数") << "," << tr("文件大小") << "," << tr("代码密度") << "\n";
    // 遍历文件统计
    for(const FileStatistics &fileStats : m_analysisResult.getFileStatistics())
    {
        double codeDensity = fileStats.totalLines > 0 ? (double)fileStats.codeLines / fileStats.totalLines * 100 : 0;
        stream << escapeCSVField(fileStats.filePath) << ",";
        stream << escapeCSVField(fileStats.language) << ",";
        stream << formatNumber(fileStats.totalLines) << ",";
        stream << formatNumber(fileStats.codeLines) << ",";
        stream << formatNumber(fileStats.commentLines) << ",";
        stream << formatNumber(fileStats.blankLines) << ",";
        stream << escapeCSVField(formatFileSize(fileStats.fileSize)) << ",";
        stream << formatPercentage(codeDensity) << "\n";
    }
    return content;
}

QString CSVExporter::generateLanguageStatsCSV() const
{
    QString content;
    QTextStream stream(&content);
    // CSV头部
    stream << tr("语言") << "," << tr("文件数") << "," << tr("文件占比") << "," << tr("总行数") << "," << tr("行数占比") << "," << tr("代码行数") << "," << tr("注释行数") << "," << tr("空行数") << "," << tr("文件大小") << "," << tr("大小占比") << "\n";
    // 计算总计
    int totalFiles = m_analysisResult.getTotalFiles();
    int totalLines = m_analysisResult.getTotalLines();
    qint64 totalSize = 0;
    // 计算总文件大小
    for(const FileStatistics &fileStats : m_analysisResult.getFileStatistics())
    {
        totalSize += fileStats.fileSize;
    }
    // 计算详细的语言统计
    QMap<QString, int> languageFileCount;
    QMap<QString, int> languageTotalLines;
    QMap<QString, int> languageCodeLines;
    QMap<QString, int> languageCommentLines;
    QMap<QString, int> languageBlankLines;
    QMap<QString, qint64> languageTotalSize;
    // 遍历文件统计，按语言汇总
    for(const FileStatistics &fileStats : m_analysisResult.getFileStatistics())
    {
        const QString &language = fileStats.language;
        if(!language.isEmpty())
        {
            languageFileCount[language]++;
            languageTotalLines[language] += fileStats.totalLines;
            languageCodeLines[language] += fileStats.codeLines;
            languageCommentLines[language] += fileStats.commentLines;
            languageBlankLines[language] += fileStats.blankLines;
            languageTotalSize[language] += fileStats.fileSize;
        }
    }
    // 输出每种语言的统计
    for(auto it = languageFileCount.begin(); it != languageFileCount.end(); ++it)
    {
        const QString &language = it.key();
        int fileCount = it.value();
        int langTotalLines = languageTotalLines[language];
        int codeLines = languageCodeLines[language];
        int commentLines = languageCommentLines[language];
        int blankLines = languageBlankLines[language];
        qint64 langTotalSize = languageTotalSize[language];
        double filePercent = totalFiles > 0 ? (double)fileCount / totalFiles * 100 : 0;
        double linePercent = totalLines > 0 ? (double)langTotalLines / totalLines * 100 : 0;
        double sizePercent = totalSize > 0 ? (double)langTotalSize / totalSize * 100 : 0;
        stream << escapeCSVField(language) << ",";
        stream << formatNumber(fileCount) << ",";
        stream << formatPercentage(filePercent) << ",";
        stream << formatNumber(langTotalLines) << ",";
        stream << formatPercentage(linePercent) << ",";
        stream << formatNumber(codeLines) << ",";
        stream << formatNumber(commentLines) << ",";
        stream << formatNumber(blankLines) << ",";
        stream << escapeCSVField(formatFileSize(langTotalSize)) << ",";
        stream << formatPercentage(sizePercent) << "\n";
    }
    return content;
}

bool CSVExporter::writeCSVFile(const QString &filePath, const QString &content)
{
    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        setError(tr("无法打开文件进行写入: %1").arg(filePath));
        return false;
    }
    
    QTextStream stream(&file);
    
    // 设置编码
    QTextCodec *codec = getTextCodec();
    if(codec)
    {
        stream.setCodec(codec);
    }
    else
    {
        // 如果编码不支持，回退到UTF-8
        stream.setCodec("UTF-8");
        qWarning() << "不支持的编码格式:" << m_encoding << "，使用UTF-8编码";
    }
    
    // 对于UTF-8和UTF-16系列编码，添加BOM
    if(m_encoding.compare("UTF-8", Qt::CaseInsensitive) == 0 || 
       m_encoding.startsWith("UTF-16", Qt::CaseInsensitive))
    {
        stream.setGenerateByteOrderMark(true);
    }
    
    stream << content;
    file.close();
    
    if(file.error() != QFile::NoError)
    {
        setError(tr("写入文件时发生错误: %1").arg(file.errorString()));
        return false;
    }
    
    return true;
}

QTextCodec* CSVExporter::getTextCodec() const
{
    QTextCodec *codec = QTextCodec::codecForName(m_encoding.toUtf8());
    return codec;
}

QString CSVExporter::escapeCSVField(const QString &field) const
{
    QString escaped = field;
    // 如果字段包含逗号、引号或换行符，需要用引号包围
    if(escaped.contains(',') || escaped.contains('"') || escaped.contains('\n') || escaped.contains('\r'))
    {
        // 转义引号
        escaped.replace('"', "\"\"");
        // 用引号包围
        escaped = '"' + escaped + '"';
    }
    return escaped;
}

QString CSVExporter::formatFileSize(qint64 bytes) const
{
    const QStringList units = {"B", "KB", "MB", "GB", "TB"};
    double size = bytes;
    int unitIndex = 0;
    while(size >= 1024.0 && unitIndex < units.size() - 1)
    {
        size /= 1024.0;
        unitIndex++;
    }
    return QString("%1 %2").arg(QString::number(size, 'f', 2), units[unitIndex]);
}

QString CSVExporter::formatNumber(int number) const
{
    // 返回不带千位分隔符的数字字符串，避免CSV格式错误
    return QString::number(number);
}

QString CSVExporter::formatPercentage(double value) const
{
    return QString("%1%").arg(QString::number(value, 'f', 2));
}

QString CSVExporter::formatDateTime(const QDateTime &dateTime) const
{
    return dateTime.toString("yyyy-MM-dd hh:mm:ss");
}

void CSVExporter::setError(const QString &error)
{
    m_lastError = error;
    qWarning() << "CSVExporter Error:" << error;
}
