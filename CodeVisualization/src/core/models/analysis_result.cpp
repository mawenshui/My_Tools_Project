#include "analysis_result.h"

AnalysisResult::AnalysisResult()
    : m_totalLines(0)
    , m_totalCodeLines(0)
    , m_totalCommentLines(0)
    , m_totalBlankLines(0)
    , m_cacheValid(false)
{
}

AnalysisResult::~AnalysisResult()
{
}

void AnalysisResult::addFileStatistics(const FileStatistics &fileStats)
{
    if (fileStats.isValid()) {
        m_fileStatistics.append(fileStats);
        m_cacheValid = false; // 标记缓存无效
    }
}

const QVector<FileStatistics>& AnalysisResult::getFileStatistics() const
{
    return m_fileStatistics;
}

int AnalysisResult::getTotalLines() const
{
    if (!m_cacheValid) {
        updateCache();
    }
    return m_totalLines;
}

int AnalysisResult::getTotalCodeLines() const
{
    if (!m_cacheValid) {
        updateCache();
    }
    return m_totalCodeLines;
}

int AnalysisResult::getTotalCommentLines() const
{
    if (!m_cacheValid) {
        updateCache();
    }
    return m_totalCommentLines;
}

int AnalysisResult::getTotalBlankLines() const
{
    if (!m_cacheValid) {
        updateCache();
    }
    return m_totalBlankLines;
}

int AnalysisResult::getTotalFiles() const
{
    return m_fileStatistics.size();
}

QMap<QString, int> AnalysisResult::getLanguageStatistics() const
{
    QMap<QString, int> languageStats;
    
    for (const FileStatistics &fileStats : m_fileStatistics) {
        if (!fileStats.language.isEmpty()) {
            languageStats[fileStats.language] += fileStats.codeLines;
        }
    }
    
    return languageStats;
}

int AnalysisResult::getLanguageCount() const
{
    QMap<QString, int> languageStats = getLanguageStatistics();
    return languageStats.size();
}

const QString& AnalysisResult::getProjectPath() const
{
    return m_projectPath;
}

void AnalysisResult::setProjectPath(const QString &path)
{
    m_projectPath = path;
}

const QDateTime& AnalysisResult::getAnalysisStartTime() const
{
    return m_analysisStartTime;
}

void AnalysisResult::setAnalysisStartTime(const QDateTime &startTime)
{
    m_analysisStartTime = startTime;
}

const QDateTime& AnalysisResult::getAnalysisEndTime() const
{
    return m_analysisEndTime;
}

void AnalysisResult::setAnalysisEndTime(const QDateTime &endTime)
{
    m_analysisEndTime = endTime;
}

qint64 AnalysisResult::getAnalysisDuration() const
{
    if (m_analysisStartTime.isValid() && m_analysisEndTime.isValid()) {
        return m_analysisStartTime.msecsTo(m_analysisEndTime);
    }
    return 0;
}

void AnalysisResult::clear()
{
    m_fileStatistics.clear();
    m_projectPath.clear();
    m_analysisStartTime = QDateTime();
    m_analysisEndTime = QDateTime();
    m_cacheValid = false;
}

bool AnalysisResult::isEmpty() const
{
    return m_fileStatistics.isEmpty();
}

double AnalysisResult::getAverageCodeDensity() const
{
    if (m_fileStatistics.isEmpty()) {
        return 0.0;
    }
    
    double totalDensity = 0.0;
    int validFiles = 0;
    
    for (const FileStatistics &fileStats : m_fileStatistics) {
        if (fileStats.totalLines > 0) {
            totalDensity += fileStats.getCodeDensity();
            validFiles++;
        }
    }
    
    return validFiles > 0 ? totalDensity / validFiles : 0.0;
}

double AnalysisResult::getAverageCommentDensity() const
{
    if (m_fileStatistics.isEmpty()) {
        return 0.0;
    }
    
    double totalDensity = 0.0;
    int validFiles = 0;
    
    for (const FileStatistics &fileStats : m_fileStatistics) {
        if (fileStats.totalLines > 0) {
            totalDensity += fileStats.getCommentDensity();
            validFiles++;
        }
    }
    
    return validFiles > 0 ? totalDensity / validFiles : 0.0;
}

void AnalysisResult::updateCache() const
{
    m_totalLines = 0;
    m_totalCodeLines = 0;
    m_totalCommentLines = 0;
    m_totalBlankLines = 0;
    
    for (const FileStatistics &fileStats : m_fileStatistics) {
        m_totalLines += fileStats.totalLines;
        m_totalCodeLines += fileStats.codeLines;
        m_totalCommentLines += fileStats.commentLines;
        m_totalBlankLines += fileStats.blankLines;
    }
    
    m_cacheValid = true;
}