#include "code_analyzer.h"
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QRegularExpression>

// 静态成员初始化
QStringList CodeAnalyzer::s_supportedExtensions = {
    // C/C++
    "c", "cpp", "cxx", "cc", "h", "hpp", "hxx",
    // Java
    "java",
    // Python
    "py", "pyw",
    // JavaScript/TypeScript
    "js", "jsx", "ts", "tsx",
    // C#
    "cs",
    // Go
    "go",
    // Rust
    "rs",
    // PHP
    "php",
    // Ruby
    "rb",
    // Swift
    "swift",
    // Kotlin
    "kt", "kts",
    // Scala
    "scala",
    // HTML/CSS
    "html", "htm", "css", "scss", "sass",
    // XML/JSON
    "xml", "json", "yaml", "yml",
    // Shell
    "sh", "bash", "zsh", "fish",
    // SQL
    "sql",
    // Markdown
    "md", "markdown"
};

CodeAnalyzer::CodeAnalyzer(QObject *parent)
    : QObject(parent)
    , m_status(Idle)
    , m_includeComments(true)
    , m_includeBlankLines(true)
    , m_cancelled(false)
    , m_currentFileIndex(0)
    , m_processTimer(new QTimer(this))
    , m_maxFileSize(10)
    , m_maxDepth(10)
    , m_recursiveAnalysis(true)
    , m_followSymlinks(false)
    , m_fileEncoding("UTF-8")
{
    initialize();
}

CodeAnalyzer::~CodeAnalyzer()
{
    cancelAnalysis();
}

void CodeAnalyzer::initialize()
{
    // 设置默认排除模式
    m_excludePatterns << "*.git*" << "*.svn*" << "*.hg*"
                      << "*.vs*" << "*.vscode*" << "*.idea*"
                      << "*build*" << "*debug*" << "*release*"
                      << "*bin*" << "*obj*" << "*target*"
                      << "*node_modules*" << "*vendor*"
                      << "*.tmp" << "*.temp" << "*.log";
    
    // 初始化默认支持的语言
    m_supportedLanguages["C++"] = QStringList() << "cpp" << "cxx" << "cc" << "c" << "h" << "hpp" << "hxx";
    m_supportedLanguages["C"] = QStringList() << "c" << "h";
    m_supportedLanguages["Python"] = QStringList() << "py" << "pyw";
    m_supportedLanguages["Java"] = QStringList() << "java";
    m_supportedLanguages["JavaScript"] = QStringList() << "js" << "jsx";
    m_supportedLanguages["TypeScript"] = QStringList() << "ts" << "tsx";
    m_supportedLanguages["C#"] = QStringList() << "cs";
    m_supportedLanguages["Go"] = QStringList() << "go";
    m_supportedLanguages["Rust"] = QStringList() << "rs";
    m_supportedLanguages["PHP"] = QStringList() << "php";
    m_supportedLanguages["Ruby"] = QStringList() << "rb";
    m_supportedLanguages["Swift"] = QStringList() << "swift";
    m_supportedLanguages["Kotlin"] = QStringList() << "kt" << "kts";
    m_supportedLanguages["Scala"] = QStringList() << "scala";
    m_supportedLanguages["HTML"] = QStringList() << "html" << "htm";
    m_supportedLanguages["CSS"] = QStringList() << "css" << "scss" << "sass";
    m_supportedLanguages["XML"] = QStringList() << "xml";
    m_supportedLanguages["JSON"] = QStringList() << "json";
    m_supportedLanguages["YAML"] = QStringList() << "yml" << "yaml";
    m_supportedLanguages["Markdown"] = QStringList() << "md" << "markdown";
    m_supportedLanguages["Shell"] = QStringList() << "sh" << "bash" << "zsh" << "fish";
    m_supportedLanguages["SQL"] = QStringList() << "sql";
    
    // 连接定时器信号
    connect(m_processTimer, &QTimer::timeout, this, &CodeAnalyzer::processNextFile);
    m_processTimer->setSingleShot(true);
}

bool CodeAnalyzer::analyzeProject(const QString &projectPath)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_status == Scanning || m_status == Analyzing) {
        qWarning() << "Analysis already in progress";
        return false;
    }
    
    QFileInfo projectInfo(projectPath);
    if (!projectInfo.exists() || !projectInfo.isDir()) {
        emit analysisError(tr("无效的项目路径: %1").arg(projectPath));
        return false;
    }
    
    // 重置状态
    m_projectPath = projectPath;
    m_result.clear();
    m_result.setProjectPath(projectPath);
    m_fileList.clear();
    m_cancelled = false;
    m_currentFileIndex = 0;
    
    // 设置分析开始时间
    m_result.setAnalysisStartTime(QDateTime::currentDateTime());
    
    // 开始扫描
    setStatus(Scanning);
    QTimer::singleShot(0, this, &CodeAnalyzer::startScanning);
    
    return true;
}

void CodeAnalyzer::cancelAnalysis()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_status == Idle || m_status == Completed || m_status == Cancelled) {
        return;
    }
    
    m_cancelled = true;
    m_processTimer->stop();
    setStatus(Cancelled);
}

CodeAnalyzer::AnalysisStatus CodeAnalyzer::getStatus() const
{
    QMutexLocker locker(&m_mutex);
    return m_status;
}

const AnalysisResult& CodeAnalyzer::getResult() const
{
    QMutexLocker locker(&m_mutex);
    return m_result;
}

void CodeAnalyzer::setExcludePatterns(const QStringList &excludePatterns)
{
    QMutexLocker locker(&m_mutex);
    m_excludePatterns = excludePatterns;
}

const QStringList& CodeAnalyzer::getExcludePatterns() const
{
    QMutexLocker locker(&m_mutex);
    return m_excludePatterns;
}

void CodeAnalyzer::setIncludeComments(bool include)
{
    QMutexLocker locker(&m_mutex);
    m_includeComments = include;
}

void CodeAnalyzer::setIncludeBlankLines(bool include)
{
    QMutexLocker locker(&m_mutex);
    m_includeBlankLines = include;
}

QStringList CodeAnalyzer::getSupportedExtensions()
{
    return s_supportedExtensions;
}

void CodeAnalyzer::startScanning()
{
    if (m_cancelled) {
        return;
    }
    
    emit progressUpdated(0, 100, tr("正在扫描文件..."));
    
    // 扫描项目目录
    m_fileList = scanDirectory(m_projectPath);
    
    if (m_cancelled) {
        return;
    }
    
    if (m_fileList.isEmpty()) {
        emit analysisError(tr("项目中未找到支持的文件"));
        setStatus(Error);
        return;
    }
    
    emit progressUpdated(100, 100, tr("找到 %1 个文件").arg(m_fileList.size()));
    
    // 开始分析
    QTimer::singleShot(100, this, &CodeAnalyzer::startAnalyzing);
}

void CodeAnalyzer::startAnalyzing()
{
    if (m_cancelled) {
        return;
    }
    
    setStatus(Analyzing);
    m_currentFileIndex = 0;
    
    emit progressUpdated(0, m_fileList.size(), tr("开始分析..."));
    
    // 开始处理第一个文件
    QTimer::singleShot(0, this, &CodeAnalyzer::processNextFile);
}

void CodeAnalyzer::processNextFile()
{
    if (m_cancelled) {
        return;
    }
    
    if (m_currentFileIndex >= m_fileList.size()) {
        // 分析完成
        m_result.setAnalysisEndTime(QDateTime::currentDateTime());
        setStatus(Completed);
        emit progressUpdated(m_fileList.size(), m_fileList.size(), tr("分析完成"));
        emit analysisCompleted(m_result);
        return;
    }
    
    QString filePath = m_fileList[m_currentFileIndex];
    
    // 更新进度
    emit progressUpdated(m_currentFileIndex, m_fileList.size(), 
                        tr("正在分析: %1").arg(QFileInfo(filePath).fileName()));
    
    // 分析文件
    FileStatistics stats = analyzeFile(filePath);
    
    if (stats.isValid()) {
        m_result.addFileStatistics(stats);
        emit fileAnalyzed(filePath, stats);
    }
    
    m_currentFileIndex++;
    
    // 继续处理下一个文件（使用定时器避免阻塞UI）
    if (!m_cancelled) {
        m_processTimer->start(1); // 1ms延迟
    }
}

QStringList CodeAnalyzer::scanDirectory(const QString &dirPath)
{
    QStringList fileList;
    QDir dir(dirPath);
    
    if (!dir.exists()) {
        return fileList;
    }
    
    // 获取所有文件和子目录
    QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    
    for (const QFileInfo &entry : entries) {
        if (m_cancelled) {
            break;
        }
        
        QString entryPath = entry.absoluteFilePath();
        
        // 检查是否应该排除
        if (shouldExcludeFile(entryPath)) {
            continue;
        }
        
        if (entry.isDir()) {
            // 递归扫描子目录
            fileList.append(scanDirectory(entryPath));
        } else if (entry.isFile()) {
            // 检查文件扩展名
            QString extension = entry.suffix().toLower();
            if (isSupportedExtension(extension)) {
                fileList.append(entryPath);
            }
        }
    }
    
    return fileList;
}

bool CodeAnalyzer::shouldExcludeFile(const QString &filePath) const
{
    QString fileName = QFileInfo(filePath).fileName();
    QString relativePath = QDir(m_projectPath).relativeFilePath(filePath);
    
    for (const QString &pattern : m_excludePatterns) {
        QRegularExpression regex(QRegularExpression::wildcardToRegularExpression(pattern));
        if (regex.match(fileName).hasMatch() || regex.match(relativePath).hasMatch()) {
            return true;
        }
    }
    
    return false;
}

bool CodeAnalyzer::isSupportedExtension(const QString &extension) const
{
    return s_supportedExtensions.contains(extension.toLower());
}

FileStatistics CodeAnalyzer::analyzeFile(const QString &filePath)
{
    FileStatistics stats;
    
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        return stats;
    }
    
    // 设置基本信息
    stats.filePath = filePath;
    stats.fileName = fileInfo.fileName();
    stats.extension = fileInfo.suffix().toLower();
    stats.fileSize = fileInfo.size();
    stats.lastModified = fileInfo.lastModified();
    
    // 确定编程语言
    if (stats.extension == "cpp" || stats.extension == "cxx" || stats.extension == "cc") {
        stats.language = "C++";
    } else if (stats.extension == "c") {
        stats.language = "C";
    } else if (stats.extension == "h" || stats.extension == "hpp" || stats.extension == "hxx") {
        stats.language = "C/C++ Header";
    } else if (stats.extension == "java") {
        stats.language = "Java";
    } else if (stats.extension == "py" || stats.extension == "pyw") {
        stats.language = "Python";
    } else if (stats.extension == "js" || stats.extension == "jsx") {
        stats.language = "JavaScript";
    } else if (stats.extension == "ts" || stats.extension == "tsx") {
        stats.language = "TypeScript";
    } else if (stats.extension == "cs") {
        stats.language = "C#";
    } else if (stats.extension == "go") {
        stats.language = "Go";
    } else if (stats.extension == "rs") {
        stats.language = "Rust";
    } else if (stats.extension == "php") {
        stats.language = "PHP";
    } else if (stats.extension == "rb") {
        stats.language = "Ruby";
    } else if (stats.extension == "swift") {
        stats.language = "Swift";
    } else if (stats.extension == "kt" || stats.extension == "kts") {
        stats.language = "Kotlin";
    } else if (stats.extension == "scala") {
        stats.language = "Scala";
    } else if (stats.extension == "html" || stats.extension == "htm") {
        stats.language = "HTML";
    } else if (stats.extension == "css") {
        stats.language = "CSS";
    } else if (stats.extension == "scss" || stats.extension == "sass") {
        stats.language = "SCSS/SASS";
    } else if (stats.extension == "xml") {
        stats.language = "XML";
    } else if (stats.extension == "json") {
        stats.language = "JSON";
    } else if (stats.extension == "yaml" || stats.extension == "yml") {
        stats.language = "YAML";
    } else if (stats.extension == "sh" || stats.extension == "bash" || stats.extension == "zsh" || stats.extension == "fish") {
        stats.language = "Shell";
    } else if (stats.extension == "sql") {
        stats.language = "SQL";
    } else if (stats.extension == "md" || stats.extension == "markdown") {
        stats.language = "Markdown";
    } else {
        stats.language = "Unknown";
    }
    
    // 读取文件内容
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file:" << filePath;
        return stats;
    }
    
    QTextStream stream(&file);
    QString content = stream.readAll();
    file.close();
    
    // 分析内容
    LineCount lineCount = analyzeContent(content, stats.language);
    
    stats.codeLines = lineCount.codeLines;
    stats.commentLines = m_includeComments ? lineCount.commentLines : 0;
    stats.blankLines = m_includeBlankLines ? lineCount.blankLines : 0;
    stats.totalLines = stats.codeLines + stats.commentLines + stats.blankLines;
    
    return stats;
}

CodeAnalyzer::LineCount CodeAnalyzer::analyzeContent(const QString &content, const QString &language)
{
    LineCount count = {0, 0, 0};
    
    QStringList lines = content.split('\n');
    bool inBlockComment = false;
    
    for (const QString &line : lines) {
        QString trimmedLine = line.trimmed();
        
        if (isBlankLine(trimmedLine)) {
            count.blankLines++;
        } else if (isCommentLine(trimmedLine, language, inBlockComment)) {
            count.commentLines++;
        } else {
            count.codeLines++;
        }
    }
    
    return count;
}

bool CodeAnalyzer::isCommentLine(const QString &line, const QString &language, bool &inBlockComment)
{
    QString trimmed = line.trimmed();
    
    if (trimmed.isEmpty()) {
        return false;
    }
    
    // C/C++/Java/JavaScript/TypeScript/C#/Go/Rust/Swift/Kotlin/Scala style comments
    if (language == "C++" || language == "C" || language == "C/C++ Header" ||
        language == "Java" || language == "JavaScript" || language == "TypeScript" ||
        language == "C#" || language == "Go" || language == "Rust" ||
        language == "Swift" || language == "Kotlin" || language == "Scala") {
        
        // Check for block comment end
        if (inBlockComment) {
            if (trimmed.contains("*/")) {
                inBlockComment = false;
            }
            return true;
        }
        
        // Check for single line comment
        if (trimmed.startsWith("//")) {
            return true;
        }
        
        // Check for block comment start
        if (trimmed.startsWith("/*")) {
            inBlockComment = !trimmed.contains("*/");
            return true;
        }
    }
    
    // Python/Shell style comments
    if (language == "Python" || language == "Shell") {
        if (trimmed.startsWith("#")) {
            return true;
        }
    }
    
    // HTML/XML style comments
    if (language == "HTML" || language == "XML") {
        if (inBlockComment) {
            if (trimmed.contains("-->")) {
                inBlockComment = false;
            }
            return true;
        }
        
        if (trimmed.startsWith("<!--")) {
            inBlockComment = !trimmed.contains("-->");
            return true;
        }
    }
    
    // CSS style comments
    if (language == "CSS" || language == "SCSS/SASS") {
        if (inBlockComment) {
            if (trimmed.contains("*/")) {
                inBlockComment = false;
            }
            return true;
        }
        
        if (trimmed.startsWith("/*")) {
            inBlockComment = !trimmed.contains("*/");
            return true;
        }
    }
    
    // SQL style comments
    if (language == "SQL") {
        if (trimmed.startsWith("--") || trimmed.startsWith("#")) {
            return true;
        }
    }
    
    return false;
}

bool CodeAnalyzer::isBlankLine(const QString &line)
{
    return line.trimmed().isEmpty();
}

void CodeAnalyzer::setSupportedLanguages(const QMap<QString, QStringList> &languages)
{
    QMutexLocker locker(&m_mutex);
    m_supportedLanguages = languages;
}

const QMap<QString, QStringList>& CodeAnalyzer::getSupportedLanguages() const
{
    QMutexLocker locker(&m_mutex);
    return m_supportedLanguages;
}

void CodeAnalyzer::setMaxFileSize(int maxSize)
{
    QMutexLocker locker(&m_mutex);
    m_maxFileSize = maxSize;
}

void CodeAnalyzer::setMaxDepth(int maxDepth)
{
    QMutexLocker locker(&m_mutex);
    m_maxDepth = maxDepth;
}

void CodeAnalyzer::setRecursiveAnalysis(bool recursive)
{
    QMutexLocker locker(&m_mutex);
    m_recursiveAnalysis = recursive;
}

void CodeAnalyzer::setFollowSymlinks(bool follow)
{
    QMutexLocker locker(&m_mutex);
    m_followSymlinks = follow;
}

void CodeAnalyzer::setFileEncoding(const QString &encoding)
{
    QMutexLocker locker(&m_mutex);
    m_fileEncoding = encoding;
}

void CodeAnalyzer::setAnalysisOptions(const QMap<QString, QVariant> &options)
{
    QMutexLocker locker(&m_mutex);
    
    if (options.contains("includeComments")) {
        m_includeComments = options["includeComments"].toBool();
    }
    if (options.contains("includeBlankLines")) {
        m_includeBlankLines = options["includeBlankLines"].toBool();
    }
    if (options.contains("recursiveAnalysis")) {
        m_recursiveAnalysis = options["recursiveAnalysis"].toBool();
    }
    if (options.contains("followSymlinks")) {
        m_followSymlinks = options["followSymlinks"].toBool();
    }
    if (options.contains("maxFileSize")) {
        m_maxFileSize = options["maxFileSize"].toInt();
    }
    if (options.contains("maxDepth")) {
        m_maxDepth = options["maxDepth"].toInt();
    }
    if (options.contains("encoding")) {
        m_fileEncoding = options["encoding"].toString();
    }
}

void CodeAnalyzer::setStatus(AnalysisStatus status)
{
    if (m_status != status) {
        m_status = status;
        emit statusChanged(status);
    }
}