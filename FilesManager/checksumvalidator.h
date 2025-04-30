#ifndef CHECKSUMVALIDATOR_H
#define CHECKSUMVALIDATOR_H

#include <QObject>
#include <QCryptographicHash>
#include <QFile>

/**
 * @brief 文件校验和验证器 - 提供文件完整性校验功能
 *
 * 功能特点：
 * 1. 支持MD5/SHA1/SHA256等多种算法
 * 2. 大文件分块计算，避免内存溢出
 * 3. 支持文件对比和哈希计算
 * 4. 线程安全设计
 */
class ChecksumValidator : public QObject
{
    Q_OBJECT
public:
    enum Algorithm
    {
        MD5 = 0,
        SHA1,
        SHA256,
        SHA512
    };
    Q_ENUM(Algorithm)

    explicit ChecksumValidator(QObject *parent = nullptr);

    /**
     * @brief 计算文件的哈希值
     * @param filePath 文件路径
     * @param algo 哈希算法
     * @param chunkSize 分块大小(字节)，默认1MB
     * @return 哈希值的16进制字符串，出错返回空字符串
     */
    static QString calculateHash(const QString &filePath,
                                 Algorithm algo = MD5,
                                 qint64 chunkSize = 1024 * 1024);

    /**
     * @brief 验证两个文件是否相同
     * @param file1 第一个文件路径
     * @param file2 第二个文件路径
     * @param algo 使用的哈希算法
     * @return 文件是否相同
     */
    static bool verifyFiles(const QString &file1,
                            const QString &file2,
                            Algorithm algo = MD5);

    /**
     * @brief 验证文件与给定哈希是否匹配
     * @param filePath 文件路径
     * @param expectedHash 预期的哈希值
     * @param algo 使用的哈希算法
     * @return 是否匹配
     */
    static bool verifyHash(const QString &filePath,
                           const QString &expectedHash,
                           Algorithm algo = MD5);

signals:
    /**
     * @brief 进度变化信号
     * @param bytesProcessed 已处理的字节数
     * @param totalBytes 总字节数
     */
    void progressChanged(qint64 bytesProcessed, qint64 totalBytes);

private:
    //转换算法枚举到qt实现
    static QCryptographicHash::Algorithm toQtAlgorithm(Algorithm algo);
};

#endif // CHECKSUMVALIDATOR_H
