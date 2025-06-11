#ifndef FILETRANSFERWORKER_H
#define FILETRANSFERWORKER_H

#include <QObject>
#include <QStringList>
#include <QString>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QDateTime>
#include <QRegularExpression>

// 转移模式枚举
enum class TransferMode {
    KeepStructure,    // 保持文件夹结构
    FlattenFiles      // 只提取文件，不保持结构
};

// 文件筛选选项结构体
struct FilterOptions {
    // 文件类型筛选
    bool enableFileTypeFilter = false;
    QStringList allowedExtensions;
    
    // 文件大小筛选
    bool enableSizeFilter = false;
    qint64 minSize = 0;  // 字节
    qint64 maxSize = 0;  // 字节
    
    // 修改时间筛选
    bool enableTimeFilter = false;
    QDateTime startTime;
    QDateTime endTime;
    
    // 文件名模式匹配
    bool enableNameFilter = false;
    QString namePattern;
    bool useRegex = false;  // true: 正则表达式, false: 通配符
    
    // 排除列表
    bool enableExcludeFilter = false;
    QStringList excludeList;
};

/**
 * @brief 文件转移工作线程类
 */
class FileTransferWorker : public QObject
{
    Q_OBJECT

public:
    explicit FileTransferWorker(const QStringList &sourcePaths, const QString &targetPath, 
                               TransferMode mode = TransferMode::KeepStructure, 
                               bool overwrite = false, 
                               const FilterOptions &filterOptions = FilterOptions(),
                               QObject *parent = nullptr);
    
public slots:
    /**
     * @brief 开始文件转移操作
     */
    void startTransfer();
    
signals:
    /**
     * @brief 转移进度更新信号
     * @param current 当前已处理的文件数
     * @param total 总文件数
     * @param fileName 当前正在处理的文件名
     */
    void progressUpdated(int current, int total, const QString &fileName);
    
    /**
     * @brief 转移完成信号
     * @param success 是否成功
     * @param message 结果消息
     */
    void transferFinished(bool success, const QString &message);
    
private:
    QStringList m_sourcePaths;  // 源路径列表
    QString m_targetPath;       // 目标路径
    TransferMode m_transferMode; // 转移模式
    bool m_overwrite;           // 是否覆盖已存在文件
    FilterOptions m_filterOptions; // 筛选选项
    
    /**
     * @brief 递归复制目录
     * @param sourceDir 源目录
     * @param targetDir 目标目录
     * @param current 当前文件计数
     * @param total 总文件数
     * @return 是否成功
     */
    bool copyDirectoryRecursively(const QString &sourceDir, const QString &targetDir, int &current, int total);
    
    /**
     * @brief 计算总文件数
     * @param paths 路径列表
     * @return 总文件数
     */
    int calculateTotalFiles(const QStringList &paths);
    
    /**
     * @brief 递归计算目录中的文件数
     * @param dirPath 目录路径
     * @return 文件数
     */
    int countFilesInDirectory(const QString &dirPath);
    
    /**
     * @brief 扁平化复制文件(只复制文件，不保持目录结构)
     * @param sourceDir 源目录
     * @param targetDir 目标目录
     * @param current 当前文件计数
     * @param total 总文件数
     * @return 是否成功
     */
    bool copyFilesFlattened(const QString &sourceDir, const QString &targetDir, int &current, int total);
    
    /**
     * @brief 生成唯一文件名(处理重名文件)
     * @param targetPath 目标文件路径
     * @return 唯一的文件路径
     */
    QString generateUniqueFileName(const QString &targetPath);
    
    /**
     * @brief 检查文件是否通过筛选条件
     * @param filePath 文件路径
     * @return 是否通过筛选
     */
    bool passesFilter(const QString &filePath);
    
    /**
     * @brief 检查文件类型是否匹配
     * @param fileName 文件名
     * @return 是否匹配
     */
    bool matchesFileType(const QFileInfo &fileInfo);
    
    /**
     * @brief 检查文件大小是否在范围内
     * @param fileSize 文件大小(字节)
     * @return 是否在范围内
     */
    bool matchesFileSize(const QFileInfo &fileInfo);
    
    /**
     * @brief 检查文件修改时间是否在范围内
     * @param lastModified 文件最后修改时间
     * @return 是否在范围内
     */
    bool matchesModificationTime(const QFileInfo &fileInfo);
    
    /**
     * @brief 检查文件名是否匹配模式
     * @param fileName 文件名
     * @return 是否匹配
     */
    bool matchesNamePattern(const QFileInfo &fileInfo);
    
    /**
     * @brief 检查文件/文件夹是否在排除列表中
     * @param path 文件或文件夹路径
     * @return 是否被排除
     */
    bool isExcluded(const QString &path);
    
    /**
     * @brief 获取预定义文件类型的扩展名列表
     * @param fileType 文件类型名称
     * @return 扩展名列表
     */
    QStringList getFileTypeExtensions(const QString &fileType);
    
    /**
      * @brief 将大小单位转换为字节
      * @param size 大小值
      * @param unit 单位(字节/KB/MB/GB)
      * @return 字节数
      */
     qint64 convertSizeToBytes(double size, const QString &unit);
};

#endif // FILETRANSFERWORKER_H