#ifndef CODE_ANALYZER_H
#define CODE_ANALYZER_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QTimer>
#include "../models/analysis_result.h"
#include "../models/file_statistics.h"

/**
 * @brief 代码分析器类
 * 
 * 负责分析项目中的代码文件，统计各种指标
 * 支持多种编程语言的识别和分析
 * 使用多线程进行文件扫描和分析
 * 
 * @author CodeVisualization Team
 * @date 2024-01-01
 * @version 1.0.0
 */
class CodeAnalyzer : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 分析状态枚举
     */
    enum AnalysisStatus {
        Idle,           ///< 空闲状态
        Scanning,       ///< 扫描文件中
        Analyzing,      ///< 分析文件中
        Completed,      ///< 分析完成
        Cancelled,      ///< 分析取消
        Error           ///< 分析错误
    };
    
    /**
     * @brief 构造函数
     * @param parent 父对象指针
     */
    explicit CodeAnalyzer(QObject *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~CodeAnalyzer();
    
    /**
     * @brief 分析指定项目路径
     * @param projectPath 项目路径
     * @return 分析是否成功启动
     */
    bool analyzeProject(const QString &projectPath);
    
    /**
     * @brief 取消当前分析
     */
    void cancelAnalysis();
    
    /**
     * @brief 获取当前分析状态
     * @return 分析状态
     */
    AnalysisStatus getStatus() const;
    
    /**
     * @brief 获取分析结果
     * @return 分析结果
     */
    const AnalysisResult& getResult() const;
    
    /**
     * @brief 设置排除规则
     * @param excludePatterns 排除模式列表
     */
    void setExcludePatterns(const QStringList &excludePatterns);
    
    /**
     * @brief 获取排除规则
     * @return 排除模式列表
     */
    const QStringList& getExcludePatterns() const;
    
    /**
     * @brief 设置是否包含注释行统计
     * @param include 是否包含
     */
    void setIncludeComments(bool include);
    
    /**
     * @brief 设置是否包含空白行统计
     * @param include 是否包含
     */
    void setIncludeBlankLines(bool include);
    
    /**
     * @brief 设置文件类型过滤模式
     * @param excludeMode true为排除模式，false为包含模式
     */
    void setFileTypeFilterMode(bool excludeMode);
    
    /**
     * @brief 设置文件类型过滤列表
     * @param fileTypes 文件类型列表（如 *.cpp, *.h）
     */
    void setFileTypeFilters(const QStringList &fileTypes);
    
    /**
     * @brief 获取文件类型过滤模式
     * @return true为排除模式，false为包含模式
     */
    bool getFileTypeFilterMode() const;
    
    /**
     * @brief 获取文件类型过滤列表
     * @return 文件类型列表
     */
    const QStringList& getFileTypeFilters() const;
    
    /**
     * @brief 获取支持的文件扩展名
     * @return 支持的扩展名列表
     */
    static QStringList getSupportedExtensions();
    
    /**
     * @brief 设置支持的语言和扩展名
     * @param languages 语言扩展名映射
     */
    void setSupportedLanguages(const QMap<QString, QStringList> &languages);
    
    /**
     * @brief 获取支持的语言和扩展名
     * @return 语言扩展名映射
     */
    const QMap<QString, QStringList>& getSupportedLanguages() const;
    
    /**
     * @brief 设置最大文件大小限制（MB）
     * @param maxSize 最大文件大小
     */
    void setMaxFileSize(int maxSize);
    
    /**
     * @brief 设置最大目录深度
     * @param maxDepth 最大深度
     */
    void setMaxDepth(int maxDepth);
    
    /**
     * @brief 设置是否递归分析
     * @param recursive 是否递归
     */
    void setRecursiveAnalysis(bool recursive);
    
    /**
     * @brief 设置是否跟随符号链接
     * @param follow 是否跟随
     */
    void setFollowSymlinks(bool follow);
    
    /**
     * @brief 设置文件编码
     * @param encoding 编码名称
     */
    void setFileEncoding(const QString &encoding);
    
    /**
     * @brief 应用分析选项
     * @param options 分析选项映射
     */
    void setAnalysisOptions(const QMap<QString, QVariant> &options);

signals:
    /**
     * @brief 分析状态变化信号
     * @param status 新状态
     */
    void statusChanged(AnalysisStatus status);
    
    /**
     * @brief 分析进度更新信号
     * @param current 当前进度
     * @param total 总进度
     * @param message 进度消息
     */
    void progressUpdated(int current, int total, const QString &message);
    
    /**
     * @brief 文件分析完成信号
     * @param filePath 文件路径
     * @param stats 文件统计信息
     */
    void fileAnalyzed(const QString &filePath, const FileStatistics &stats);
    
    /**
     * @brief 分析完成信号
     * @param result 分析结果
     */
    void analysisCompleted(const AnalysisResult &result);
    
    /**
     * @brief 分析错误信号
     * @param error 错误信息
     */
    void analysisError(const QString &error);

private slots:
    /**
     * @brief 开始文件扫描
     */
    void startScanning();
    
    /**
     * @brief 开始文件分析
     */
    void startAnalyzing();
    
    /**
     * @brief 处理单个文件分析
     */
    void processNextFile();

private:
    /**
     * @brief 初始化分析器
     */
    void initialize();
    
    /**
     * @brief 扫描目录中的文件
     * @param dirPath 目录路径
     * @return 文件列表
     */
    QStringList scanDirectory(const QString &dirPath);
    
    /**
     * @brief 检查文件是否应该被排除
     * @param filePath 文件路径
     * @return 是否排除
     */
    bool shouldExcludeFile(const QString &filePath) const;
    
    /**
     * @brief 检查文件扩展名是否支持
     * @param extension 文件扩展名
     * @return 是否支持
     */
    bool isSupportedExtension(const QString &extension) const;
    
    /**
     * @brief 检查文件是否匹配类型过滤规则
     * @param filePath 文件路径
     * @return 是否匹配过滤规则
     */
    bool matchesFileTypeFilter(const QString &filePath) const;
    
    /**
     * @brief 分析单个文件
     * @param filePath 文件路径
     * @return 文件统计信息
     */
    FileStatistics analyzeFile(const QString &filePath);
    
    /**
     * @brief 分析文件内容
     * @param content 文件内容
     * @param language 编程语言
     * @return 行数统计结果 (代码行, 注释行, 空白行)
     */
    struct LineCount {
        int codeLines;
        int commentLines;
        int blankLines;
    };
    LineCount analyzeContent(const QString &content, const QString &language);
    
    /**
     * @brief 检查是否为注释行
     * @param line 代码行
     * @param language 编程语言
     * @param inBlockComment 是否在块注释中
     * @return 是否为注释行
     */
    bool isCommentLine(const QString &line, const QString &language, bool &inBlockComment);
    
    /**
     * @brief 检查是否为空白行
     * @param line 代码行
     * @return 是否为空白行
     */
    bool isBlankLine(const QString &line);
    
    /**
     * @brief 设置分析状态
     * @param status 新状态
     */
    void setStatus(AnalysisStatus status);
    
    // 成员变量
    QString m_projectPath;                  ///< 项目路径
    QStringList m_fileList;                 ///< 待分析文件列表
    QStringList m_excludePatterns;          ///< 排除模式列表
    AnalysisResult m_result;                ///< 分析结果
    AnalysisStatus m_status;                ///< 当前状态
    
    bool m_includeComments;                 ///< 是否包含注释行
    bool m_includeBlankLines;               ///< 是否包含空白行
    bool m_cancelled;                       ///< 是否已取消
    
    // 文件类型过滤相关
    bool m_fileTypeFilterExcludeMode;       ///< 文件类型过滤模式：true为排除，false为包含
    QStringList m_fileTypeFilters;          ///< 文件类型过滤列表
    
    int m_currentFileIndex;                 ///< 当前处理文件索引
    QTimer *m_processTimer;                 ///< 处理定时器
    
    mutable QMutex m_mutex;                 ///< 线程安全互斥锁
    
    // 高级配置选项
    QMap<QString, QStringList> m_supportedLanguages;  ///< 支持的语言和扩展名
    int m_maxFileSize;                      ///< 最大文件大小（MB）
    int m_maxDepth;                         ///< 最大目录深度
    bool m_recursiveAnalysis;               ///< 是否递归分析
    bool m_followSymlinks;                  ///< 是否跟随符号链接
    QString m_fileEncoding;                 ///< 文件编码
    
    static QStringList s_supportedExtensions;  ///< 支持的文件扩展名
};

#endif // CODE_ANALYZER_H