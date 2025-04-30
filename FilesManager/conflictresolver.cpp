#include "conflictresolver.h"
#include <QFileInfo>
#include <QDateTime>
#include <QDir>
#include <QDebug>

ConflictResolver::ConflictResolver(QObject *parent)
    : QObject(parent),
      m_defaultResolution(Rename),
      m_renameTemplate("%3_(%1)%2")  // 默认模板: 原文件名_(序号).扩展名
{
}

QString ConflictResolver::resolveConflict(const QString &destPath,
        Resolution resolution)
{
    QFileInfo destInfo(destPath);
    QString resolvedPath = destPath;
    Resolution actualAction = resolution == AskUser ? m_defaultResolution : resolution;
    // 只有文件存在时才需要处理冲突
    if(destInfo.exists())
    {
        switch(actualAction)
        {
            case Overwrite:
                if(!QFile::remove(destPath))
                {
                    qWarning() << "无法删除文件:" << destPath;
                    return QString();
                }
                break;
            case Skip:
                resolvedPath = QString(); // 空路径表示跳过
                break;
            case Rename:
                resolvedPath = generateNewName(destPath);
                break;
            case AskUser: // 理论上不会执行到这里
                break;
        }
        emit conflictResolved(destPath, resolvedPath, actualAction);
    }
    return resolvedPath;
}

void ConflictResolver::setDefaultResolution(Resolution resolution)
{
    if(resolution != AskUser)
    {
        m_defaultResolution = resolution;
    }
}

void ConflictResolver::setRenameTemplate(const QString &templateStr)
{
    if(!templateStr.isEmpty())
    {
        m_renameTemplate = templateStr;
    }
}

QString ConflictResolver::generateNewName(const QString &originalPath) const
{
    QFileInfo fi(originalPath);
    QString baseName = fi.completeBaseName();
    QString suffix = fi.suffix();
    QString dir = fi.path();
    // 尝试不同的序号直到找到不冲突的文件名
    for(int i = 1; i < 1000; ++i)
    {
        QString newName = m_renameTemplate
                          .arg(i)                                      // %1 = 序号
                          .arg(suffix.isEmpty() ? "" : "." + suffix)   // %2 = 扩展名(带点)
                          .arg(baseName);                              // %3 = 原文件名
        QString newPath = QDir(dir).filePath(newName);
        if(!QFileInfo::exists(newPath))
        {
            return newPath;
        }
    }
    // 如果序号用尽，添加时间戳
    return QDir(dir).filePath(
               QString("%1_%2.%3")
               .arg(baseName)
               .arg(getTimestampSuffix())
               .arg(suffix)
           );
}

QString ConflictResolver::getTimestampSuffix() const
{
    return QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
}
