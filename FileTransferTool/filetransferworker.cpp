#include "filetransferworker.h"

/**
 * @brief FileTransferWorker构造函数
 * @param sourcePaths 源路径列表
 * @param targetPath 目标路径
 * @param mode 转移模式
 * @param overwrite 是否覆盖已存在文件
 * @param filterOptions 筛选选项
 * @param parent 父对象
 */
FileTransferWorker::FileTransferWorker(const QStringList &sourcePaths, const QString &targetPath, 
                                     TransferMode mode, bool overwrite, 
                                     const FilterOptions &filterOptions, QObject *parent)
    : QObject(parent), m_sourcePaths(sourcePaths), m_targetPath(targetPath), 
      m_transferMode(mode), m_overwrite(overwrite), m_filterOptions(filterOptions)
{
}

/**
 * @brief 开始文件转移操作
 */
void FileTransferWorker::startTransfer()
{
    int totalFiles = calculateTotalFiles(m_sourcePaths);
    int currentFile = 0;
    
    if (totalFiles == 0) {
        emit transferFinished(false, "没有找到要转移的文件");
        return;
    }
    
    // 确保目标目录存在
    QDir targetDir(m_targetPath);
    if (!targetDir.exists()) {
        if (!targetDir.mkpath(m_targetPath)) {
            emit transferFinished(false, "无法创建目标目录: " + m_targetPath);
            return;
        }
    }
    
    // 开始转移文件
    for (const QString &sourcePath : m_sourcePaths) {
        QFileInfo sourceInfo(sourcePath);
        
        if (sourceInfo.isFile()) {
            // 处理单个文件
            QString fileName = sourceInfo.fileName();
            QString targetFilePath = m_targetPath + "/" + fileName;
            
            emit progressUpdated(currentFile, totalFiles, fileName);
            
            if (!QFile::copy(sourcePath, targetFilePath)) {
                emit transferFinished(false, "复制文件失败: " + sourcePath);
                return;
            }
            currentFile++;
        } else if (sourceInfo.isDir()) {
            // 处理目录
            if (m_transferMode == TransferMode::KeepStructure) {
                // 保持文件夹结构
                QString dirName = sourceInfo.fileName();
                QString targetDirPath = m_targetPath + "/" + dirName;
                
                if (!copyDirectoryRecursively(sourcePath, targetDirPath, currentFile, totalFiles)) {
                    emit transferFinished(false, "复制目录失败: " + sourcePath);
                    return;
                }
            } else {
                // 扁平化复制，只复制文件
                if (!copyFilesFlattened(sourcePath, m_targetPath, currentFile, totalFiles)) {
                    emit transferFinished(false, "扁平化复制失败: " + sourcePath);
                    return;
                }
            }
        }
    }
    
    emit transferFinished(true, QString("成功转移 %1 个文件").arg(totalFiles));
}

/**
 * @brief 递归复制目录
 * @param sourceDir 源目录
 * @param targetDir 目标目录
 * @param current 当前文件计数
 * @param total 总文件数
 * @return 是否成功
 */
bool FileTransferWorker::copyDirectoryRecursively(const QString &sourceDir, const QString &targetDir, int &current, int total)
{
    QDir source(sourceDir);
    QDir target(targetDir);
    
    // 创建目标目录
    if (!target.exists()) {
        if (!target.mkpath(targetDir)) {
            return false;
        }
    }
    
    // 获取源目录中的所有条目
    QFileInfoList entries = source.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    
    for (const QFileInfo &entry : entries) {
        QString sourcePath = entry.absoluteFilePath();
        QString targetPath = targetDir + "/" + entry.fileName();
        
        if (entry.isFile()) {
            // 检查文件是否通过筛选条件
            if (!passesFilter(sourcePath)) {
                current++; // 跳过的文件也要计入进度
                continue;
            }
            
            emit progressUpdated(current, total, entry.fileName());
            
            // 处理文件覆盖
            if (QFile::exists(targetPath)) {
                if (m_overwrite) {
                    QFile::remove(targetPath);
                } else {
                    targetPath = generateUniqueFileName(targetPath);
                }
            }
            
            if (!QFile::copy(sourcePath, targetPath)) {
                return false;
            }
            current++;
        } else if (entry.isDir()) {
            // 检查目录是否被排除
            if (m_filterOptions.enableExcludeFilter && isExcluded(entry.fileName())) {
                continue;
            }
            
            if (!copyDirectoryRecursively(sourcePath, targetPath, current, total)) {
                return false;
            }
        }
    }
    
    return true;
}

/**
 * @brief 计算总文件数
 * @param paths 路径列表
 * @return 总文件数
 */
int FileTransferWorker::calculateTotalFiles(const QStringList &paths)
{
    int total = 0;
    
    for (const QString &path : paths) {
        QFileInfo info(path);
        if (info.isFile()) {
            total++;
        } else if (info.isDir()) {
            total += countFilesInDirectory(path);
        }
    }
    
    return total;
}

/**
 * @brief 递归计算目录中的文件数
 * @param dirPath 目录路径
 * @return 文件数
 */
int FileTransferWorker::countFilesInDirectory(const QString &dirPath)
{
    int count = 0;
    QDir dir(dirPath);
    
    QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    
    for (const QFileInfo &entry : entries) {
        if (entry.isFile()) {
            // 只计算通过筛选条件的文件
            if (passesFilter(entry.absoluteFilePath())) {
                count++;
            }
        } else if (entry.isDir()) {
            // 检查目录是否被排除
            if (!(m_filterOptions.enableExcludeFilter && isExcluded(entry.fileName()))) {
                count += countFilesInDirectory(entry.absoluteFilePath());
            }
        }
    }
    
    return count;
}

/**
 * @brief 扁平化复制文件(只复制文件，不保持目录结构)
 * @param sourceDir 源目录
 * @param targetDir 目标目录
 * @param current 当前文件计数
 * @param total 总文件数
 * @return 是否成功
 */
bool FileTransferWorker::copyFilesFlattened(const QString &sourceDir, const QString &targetDir, int &current, int total)
{
    QDir source(sourceDir);
    
    // 获取源目录中的所有条目
    QFileInfoList entries = source.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    
    for (const QFileInfo &entry : entries) {
        QString sourcePath = entry.absoluteFilePath();
        
        if (entry.isFile()) {
            // 检查文件是否通过筛选条件
            if (!passesFilter(sourcePath)) {
                current++; // 跳过的文件也要计入进度
                continue;
            }
            
            // 直接复制文件到目标目录根目录
            QString targetPath = targetDir + "/" + entry.fileName();
            
            emit progressUpdated(current, total, entry.fileName());
            
            // 处理文件覆盖
            if (QFile::exists(targetPath)) {
                if (m_overwrite) {
                    QFile::remove(targetPath);
                } else {
                    targetPath = generateUniqueFileName(targetPath);
                }
            }
            
            if (!QFile::copy(sourcePath, targetPath)) {
                return false;
            }
            current++;
        } else if (entry.isDir()) {
            // 检查目录是否被排除
            if (m_filterOptions.enableExcludeFilter && isExcluded(entry.fileName())) {
                continue;
            }
            
            // 递归处理子目录
            if (!copyFilesFlattened(sourcePath, targetDir, current, total)) {
                return false;
            }
        }
    }
    
    return true;
}

/**
 * @brief 生成唯一文件名(处理重名文件)
 * @param targetPath 目标文件路径
 * @return 唯一的文件路径
 */
QString FileTransferWorker::generateUniqueFileName(const QString &targetPath)
{
    QFileInfo fileInfo(targetPath);
    QString baseName = fileInfo.completeBaseName();
    QString suffix = fileInfo.suffix();
    QString dir = fileInfo.absolutePath();
    
    int counter = 1;
    QString newPath;
    
    do {
        if (suffix.isEmpty()) {
            newPath = QString("%1/%2(%3)").arg(dir, baseName, QString::number(counter));
        } else {
            newPath = QString("%1/%2(%3).%4").arg(dir, baseName, QString::number(counter), suffix);
        }
        counter++;
    } while (QFile::exists(newPath));
    
    return newPath;
}

/**
 * @brief 检查文件是否通过所有筛选条件
 * @param filePath 文件路径
 * @return 是否通过筛选
 */
bool FileTransferWorker::passesFilter(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    
    // 检查文件类型筛选
    if (m_filterOptions.enableFileTypeFilter && !matchesFileType(fileInfo)) {
        return false;
    }
    
    // 检查文件大小筛选
    if (m_filterOptions.enableSizeFilter && !matchesFileSize(fileInfo)) {
        return false;
    }
    
    // 检查修改时间筛选
    if (m_filterOptions.enableTimeFilter && !matchesModificationTime(fileInfo)) {
        return false;
    }
    
    // 检查文件名模式筛选
    if (m_filterOptions.enableNameFilter && !matchesNamePattern(fileInfo)) {
        return false;
    }
    
    // 检查排除列表
    if (m_filterOptions.enableExcludeFilter && isExcluded(fileInfo.fileName())) {
        return false;
    }
    
    return true;
}

/**
 * @brief 检查文件类型是否匹配
 * @param fileInfo 文件信息
 * @return 是否匹配
 */
bool FileTransferWorker::matchesFileType(const QFileInfo &fileInfo)
{
    QString suffix = fileInfo.suffix().toLower();
    // 检查不带点号的后缀
    if (m_filterOptions.allowedExtensions.contains(suffix)) {
        return true;
    }
    // 检查带点号的后缀
    QString suffixWithDot = "." + suffix;
    return m_filterOptions.allowedExtensions.contains(suffixWithDot);
}

/**
 * @brief 检查文件大小是否匹配
 * @param fileInfo 文件信息
 * @return 是否匹配
 */
bool FileTransferWorker::matchesFileSize(const QFileInfo &fileInfo)
{
    qint64 fileSize = fileInfo.size();
    
    return fileSize >= m_filterOptions.minSize && fileSize <= m_filterOptions.maxSize;
}

/**
 * @brief 检查修改时间是否匹配
 * @param fileInfo 文件信息
 * @return 是否匹配
 */
bool FileTransferWorker::matchesModificationTime(const QFileInfo &fileInfo)
{
    QDateTime fileTime = fileInfo.lastModified();
    
    return fileTime >= m_filterOptions.startTime && fileTime <= m_filterOptions.endTime;
}

/**
 * @brief 检查文件名模式是否匹配
 * @param fileInfo 文件信息
 * @return 是否匹配
 */
bool FileTransferWorker::matchesNamePattern(const QFileInfo &fileInfo)
{
    QString fileName = fileInfo.fileName();
    
    if (m_filterOptions.useRegex) {
        QRegularExpression regex(m_filterOptions.namePattern);
        return regex.match(fileName).hasMatch();
    } else {
        QRegularExpression regex(QRegularExpression::wildcardToRegularExpression(m_filterOptions.namePattern));
        return regex.match(fileName).hasMatch();
    }
}

/**
 * @brief 检查文件/目录是否在排除列表中
 * @param name 文件或目录名
 * @return 是否被排除
 */
bool FileTransferWorker::isExcluded(const QString &name)
{
    for (const QString &pattern : m_filterOptions.excludeList) {
        QString trimmedPattern = pattern.trimmed();
        if (trimmedPattern.isEmpty()) continue;
        
        // 支持通配符匹配
        QRegularExpression regex(QRegularExpression::wildcardToRegularExpression(trimmedPattern));
        if (regex.match(name).hasMatch()) {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 根据文件类型获取扩展名列表
 * @param fileType 文件类型
 * @return 扩展名列表
 */
QStringList FileTransferWorker::getFileTypeExtensions(const QString &fileType)
{
    if (fileType == "图片") {
        return {"jpg", "jpeg", "png", "gif", "bmp", "tiff", "webp", "svg", "ico"};
    } else if (fileType == "文档") {
        return {"txt", "doc", "docx", "pdf", "xls", "xlsx", "ppt", "pptx", "rtf", "odt", "ods", "odp"};
    } else if (fileType == "视频") {
        return {"mp4", "avi", "mkv", "mov", "wmv", "flv", "webm", "m4v", "3gp", "mpg", "mpeg"};
    } else if (fileType == "音频") {
        return {"mp3", "wav", "flac", "aac", "ogg", "wma", "m4a", "opus"};
    } else if (fileType == "压缩包") {
        return {"zip", "rar", "7z", "tar", "gz", "bz2", "xz", "z"};
    } else if (fileType == "可执行文件") {
        return {"exe", "msi", "app", "deb", "rpm", "dmg"};
    }
    
    return {};
}

/**
 * @brief 将大小值和单位转换为字节数
 * @param size 大小值
 * @param unit 单位
 * @return 字节数
 */
qint64 FileTransferWorker::convertSizeToBytes(double size, const QString &unit)
{
    if (unit == "KB") {
        return static_cast<qint64>(size * 1024);
    } else if (unit == "MB") {
        return static_cast<qint64>(size * 1024 * 1024);
    } else if (unit == "GB") {
        return static_cast<qint64>(size * 1024 * 1024 * 1024);
    }
    
    return static_cast<qint64>(size); // 默认为字节
}