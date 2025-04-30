#ifndef RESUMABLETRANSFER_H
#define RESUMABLETRANSFER_H

#include <QObject>
#include <QFile>
#include <QCryptographicHash>

/**
 * @brief 断点续传处理器 - 支持大文件分块传输和断点续传
 *
 * 实现思路：
 * 1. 使用临时文件(.part)保存传输进度
 * 2. 支持自定义分块大小传输
 * 3. 自动校验文件完整性(MD5)
 * 4. 线程安全设计，可在后台线程中使用
 */
class ResumableTransfer : public QObject
{
    Q_OBJECT
public:
    explicit ResumableTransfer(QObject *parent = nullptr);

    /**
     * @brief 执行带断点续传的文件传输
     * @param src 源文件路径
     * @param dest 目标文件路径
     * @param chunkSize 分块大小(字节)，默认1MB
     * @return 传输是否成功
     */
    bool transferFile(const QString &src,
                      const QString &dest,
                      qint64 chunkSize = 1024 * 1024);

    /**
     * @brief 校验文件完整性
     * @param src 原始文件路径
     * @param dest 已传输文件路径
     * @return 文件是否完整一致
     */
    bool verifyFile(const QString &src,
                    const QString &dest) const;

signals:
    /**
     * @brief 传输进度信号
     * @param bytesTransferred 已传输字节数
     * @param totalBytes 总字节数
     */
    void progressChanged(qint64 bytesTransferred, qint64 totalBytes);

    /**
     * @brief 状态消息信号
     * @param message 状态信息
     */
    void statusMessage(const QString &message);

    /**
     * @brief 错误发生信号
     * @param error 错误描述
     */
    void errorOccurred(const QString &error);

private:
    QCryptographicHash m_hash; // 哈希计算器

    /**
     * @brief 获取临时文件路径
     * @param originalPath 原始路径
     * @return 临时文件路径
     */
    QString tempFilePath(const QString &originalPath) const;

    /**
     * @brief 传输单个文件块
     * @param src 源文件对象
     * @param dest 目标文件对象
     * @param chunkSize 分块大小
     * @return 传输是否成功
     */
    bool transferChunk(QFile &src, QFile &dest, qint64 chunkSize);
};

#endif // RESUMABLETRANSFER_H
