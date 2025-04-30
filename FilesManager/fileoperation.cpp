#include "fileoperation.h"
#include <QDir>
#include <QFileInfo>
#include <QThread>
#include <QDebug>

FileOperation::FileOperation(QObject *parent)
    : QObject(parent),
      m_paused(false),
      m_canceled(false)
{
    // 连接内部信号
    connect(&m_transfer, &ResumableTransfer::progressChanged, this, [this](qint64 bytes, qint64 total)
    {
        emit progressChanged(static_cast<int>(bytes * 100 / total));
    });
    connect(&m_transfer, &ResumableTransfer::statusMessage, this, &FileOperation::statusMessage);
    connect(&m_walker, &DirectoryWalker::fileFound, this, [this](const QString & file)
    {
        emit currentFileChanged(QFileInfo(file).fileName());
    });
}

FileOperation::~FileOperation()
{
    cancel();
}

void FileOperation::startOperation(const QString &srcDir,
                                   const QString &destDir,
                                   OperationType type,
                                   bool recursive,
                                   ConflictResolver::Resolution conflictRes,
                                   const QString &filter)
{
    // 重置状态
    m_canceled = false;
    m_paused = false;
    emit statusMessage("正在扫描文件...");
    // 查找所有匹配文件
    QStringList files = m_walker.findFiles(srcDir, recursive, filter);
    if(files.isEmpty())
    {
        emit errorOccurred("未找到匹配的文件");
        return;
    }
    emit statusMessage(QString("找到 %1 个文件").arg(files.size()));
    int processed = 0;
    const int total = files.size();
    // 处理每个文件
    foreach(const QString &srcPath, files)
    {
        // 检查取消状态
        if(m_canceled)
        {
            emit statusMessage("操作已被取消");
            return;
        }
        // 处理暂停状态
        while(m_paused && !m_canceled)
        {
            QThread::msleep(100);
        }
        QFileInfo fi(srcPath);
        QString destPath = QDir(destDir).filePath(fi.fileName());
        // 处理文件
        if(!processFile(srcPath, destPath, type, conflictRes))
        {
            return; // 发生错误时中止
        }
        // 更新进度
        emit progressChanged(++processed * 100 / total);
    }
    emit operationFinished();
}

bool FileOperation::processFile(const QString &srcPath,
                                const QString &destPath,
                                OperationType type,
                                ConflictResolver::Resolution conflictRes)
{
    QFileInfo destInfo(destPath);
    QString finalDest = destPath;
    // 处理文件冲突
    if(destInfo.exists())
    {
        switch(conflictRes)
        {
            case ConflictResolver::Skip:
                emit statusMessage(QString("跳过已存在文件: %1").arg(destPath));
                return true;
            case ConflictResolver::Rename:
                {
                    int counter = 1;
                    QString newName;
                    do
                    {
                        newName = QString("%1_%2.%3")
                                  .arg(destInfo.baseName())
                                  .arg(counter++)
                                  .arg(destInfo.completeSuffix());
                        finalDest = QDir(destInfo.path()).filePath(newName);
                    }
                    while(QFileInfo::exists(finalDest));
                    emit statusMessage(QString("重命名文件为: %1").arg(newName));
                    break;
                }
            case ConflictResolver::Overwrite:
                if(!QFile::remove(destPath))
                {
                    emit errorOccurred(QString("无法覆盖文件: %1").arg(destPath));
                    return false;
                }
                break;
        }
    }
    // 执行文件操作
    bool success = false;
    if(type == Copy)
    {
        emit statusMessage(QString("复制: %1 → %2").arg(srcPath, finalDest));
        success = m_transfer.transferFile(srcPath, finalDest);
    }
    else
    {
        emit statusMessage(QString("移动: %1 → %2").arg(srcPath, finalDest));
        if(QFile::rename(srcPath, finalDest))
        {
            success = true;
        }
        else
        {
            // 回退到复制+删除方案
            if(m_transfer.transferFile(srcPath, finalDest))
            {
                success = QFile::remove(srcPath);
                if(!success)
                {
                    emit errorOccurred(QString("无法删除源文件: %1").arg(srcPath));
                }
            }
        }
    }
    if(!success)
    {
        emit errorOccurred(QString("操作失败: %1").arg(srcPath));
    }
    return success;
}

void FileOperation::pause()
{
    m_paused = true;
    emit statusMessage("操作已暂停");
}

void FileOperation::resume()
{
    m_paused = false;
    emit statusMessage("操作已恢复");
}

void FileOperation::cancel()
{
    m_canceled = true;
    m_paused = false; // 确保能退出暂停状态
    emit statusMessage("正在取消操作...");
}

QString FileOperation::errorToString(QFile::FileError error) const
{
    switch(error)
    {
        case QFile::ReadError:
            return "读取错误";
        case QFile::WriteError:
            return "写入错误";
        case QFile::PermissionsError:
            return "权限不足";
        case QFile::CopyError:
            return "复制失败";
        case QFile::RenameError:
            return "重命名失败";
        case QFile::RemoveError:
            return "删除失败";
        default:
            return "未知错误";
    }
}
