#include "resumabletransfer.h"
#include <QFileInfo>
#include <QDateTime>

ResumableTransfer::ResumableTransfer(QObject *parent)
    : QObject(parent), m_hash(QCryptographicHash::Md5) {}

bool ResumableTransfer::transferFile(const QString &src,
                                     const QString &dest,
                                     qint64 chunkSize)
{
    // 检查源文件
    QFile srcFile(src);
    if(!srcFile.exists())
    {
        emit errorOccurred("源文件不存在: " + src);
        return false;
    }
    // 准备目标临时文件
    const QString tempPath = tempFilePath(dest);
    QFile destFile(tempPath);
    qint64 startPosition = 0;
    // 检查是否已有部分传输
    if(destFile.exists())
    {
        startPosition = destFile.size();
        emit statusMessage(QString("检测到未完成传输，从 %1 字节处继续").arg(startPosition));
    }
    // 打开文件
    if(!srcFile.open(QIODevice::ReadOnly))
    {
        emit errorOccurred("无法打开源文件: " + srcFile.errorString());
        return false;
    }
    if(!destFile.open(startPosition == 0 ? QIODevice::WriteOnly : QIODevice::Append))
    {
        srcFile.close();
        emit errorOccurred("无法打开目标文件: " + destFile.errorString());
        return false;
    }
    // 定位到续传位置
    if(startPosition > 0 && !srcFile.seek(startPosition))
    {
        srcFile.close();
        destFile.close();
        emit errorOccurred("无法定位到续传位置");
        return false;
    }
    // 获取文件总大小
    const qint64 totalSize = srcFile.size();
    qint64 bytesTransferred = startPosition;
    // 分块传输
    while(!srcFile.atEnd() && bytesTransferred < totalSize)
    {
        if(!transferChunk(srcFile, destFile, chunkSize))
        {
            srcFile.close();
            destFile.close();
            return false;
        }
        bytesTransferred = destFile.pos();
        emit progressChanged(bytesTransferred, totalSize);
    }
    // 关闭文件
    srcFile.close();
    destFile.close();
    // 重命名临时文件
    if(bytesTransferred == totalSize)
    {
        if(QFile::exists(dest))
        {
            QFile::remove(dest);
        }
        if(!QFile::rename(tempPath, dest))
        {
            emit errorOccurred("无法重命名临时文件");
            return false;
        }
        emit statusMessage("文件传输完成");
        return true;
    }
    return false;
}

bool ResumableTransfer::transferChunk(QFile &src, QFile &dest, qint64 chunkSize)
{
    const qint64 maxRetry = 3;
    qint64 retryCount = 0;
    while(retryCount < maxRetry)
    {
        const QByteArray chunk = src.read(chunkSize);
        if(chunk.isEmpty() && !src.atEnd())
        {
            retryCount++;
            emit statusMessage(QString("读取失败，重试 %1/%2").arg(retryCount).arg(maxRetry));
            continue;
        }
        if(dest.write(chunk) != chunk.size())
        {
            retryCount++;
            emit statusMessage(QString("写入失败，重试 %1/%2").arg(retryCount).arg(maxRetry));
            continue;
        }
        // 立即刷新到磁盘
        if(!dest.flush())
        {
            emit errorOccurred("无法刷新写入缓存");
            return false;
        }
        // 更新哈希
        m_hash.addData(chunk);
        return true;
    }
    emit errorOccurred(QString("传输失败，已达到最大重试次数 %1").arg(maxRetry));
    return false;
}

bool ResumableTransfer::verifyFile(const QString &src, const QString &dest) const
{
    QFile srcFile(src), destFile(dest);
    if(!srcFile.open(QIODevice::ReadOnly) || !destFile.open(QIODevice::ReadOnly))
    {
        return false;
    }
    // 比较文件大小
    if(srcFile.size() != destFile.size())
    {
        srcFile.close();
        destFile.close();
        return false;
    }
    // 比较内容
    const qint64 chunkSize = 1024 * 1024; // 1MB
    QByteArray srcChunk, destChunk;
    while(!srcFile.atEnd())
    {
        srcChunk = srcFile.read(chunkSize);
        destChunk = destFile.read(chunkSize);
        if(srcChunk != destChunk)
        {
            srcFile.close();
            destFile.close();
            return false;
        }
    }
    srcFile.close();
    destFile.close();
    return true;
}

QString ResumableTransfer::tempFilePath(const QString &originalPath) const
{
    return originalPath + ".part";
}
