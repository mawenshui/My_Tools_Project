#include "checksumvalidator.h"
#include <QFileInfo>
#include <QElapsedTimer>
#include <QDebug>

ChecksumValidator::ChecksumValidator(QObject *parent)
    : QObject(parent)
{
}

QString ChecksumValidator::calculateHash(const QString &filePath,
        Algorithm algo,
        qint64 chunkSize)
{
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "无法打开文件:" << file.errorString();
        return QString();
    }
    QCryptographicHash hash(toQtAlgorithm(algo));
    qint64 totalSize = file.size();
    qint64 bytesRead = 0;
    QElapsedTimer timer;
    timer.start();
    // 分块计算哈希
    while(!file.atEnd())
    {
        QByteArray chunk = file.read(chunkSize);
        hash.addData(chunk);
        bytesRead += chunk.size();
        // 每处理10MB或进度变化≥1%时可以考虑发射信号
        // 注意：静态方法不能直接发射信号，需要外部实例化后使用
    }
    file.close();
    qDebug() << QString("计算%1哈希完成, 耗时: %2ms")
             .arg(algo == MD5 ? "MD5" :
                  algo == SHA1 ? "SHA1" :
                  algo == SHA256 ? "SHA256" : "SHA512")
             .arg(timer.elapsed());
    return QString(hash.result().toHex());
}

bool ChecksumValidator::verifyFiles(const QString &file1,
                                    const QString &file2,
                                    Algorithm algo)
{
    // 快速检查文件大小
    if(QFileInfo(file1).size() != QFileInfo(file2).size())
    {
        return false;
    }
    // 计算两个文件的哈希值
    QString hash1 = calculateHash(file1, algo);
    QString hash2 = calculateHash(file2, algo);
    return (!hash1.isEmpty() && !hash2.isEmpty() && hash1 == hash2);
}

bool ChecksumValidator::verifyHash(const QString &filePath,
                                   const QString &expectedHash,
                                   Algorithm algo)
{
    QString actualHash = calculateHash(filePath, algo);
    return (!actualHash.isEmpty() && actualHash == expectedHash);
}

QCryptographicHash::Algorithm ChecksumValidator::toQtAlgorithm(Algorithm algo)
{
    switch(algo)
    {
        case SHA1:
            return QCryptographicHash::Sha1;
        case SHA256:
            return QCryptographicHash::Sha256;
        case SHA512:
            return QCryptographicHash::Sha512;
        default:
            return QCryptographicHash::Md5;
    }
}
