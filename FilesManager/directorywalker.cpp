#include "directorywalker.h"
#include <QDirIterator>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QDebug>

DirectoryWalker::DirectoryWalker(QObject *parent)
    : QObject(parent)
{
}

QStringList DirectoryWalker::findFiles(const QString &path,
                                       bool recursive,
                                       const QString &filter) const
{
    QStringList result;
    QElapsedTimer timer;
    timer.start();
    // 设置迭代器标志
    QDirIterator::IteratorFlags flags = recursive ?
                                        QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags;
    // 第一次遍历获取总数(用于进度计算)
    int totalFiles = 0;
    QDirIterator countIter(path, QDir::Files, flags);
    while(countIter.hasNext())
    {
        countIter.next();
        totalFiles++;
    }
    if(totalFiles == 0)
    {
        emit progressChanged(100);
        return result;
    }
    // 实际遍历文件
    int processed = 0;
    QDirIterator iter(path, QDir::Files, flags);
    while(iter.hasNext())
    {
        iter.next();
        const QString filePath = iter.filePath();
        const QString fileName = iter.fileName();
        // 应用过滤规则
        if(filter.isEmpty() || matchesFilter(fileName, filter))
        {
            result.append(filePath);
            emit fileFound(filePath);
        }
        // 更新进度(每处理10个文件或进度变化≥1%时触发)
        if(++processed % 10 == 0 || processed * 100 / totalFiles != (processed - 1) * 100 / totalFiles)
        {
            emit progressChanged(processed * 100 / totalFiles);
        }
    }
    qDebug() << "Directory scan completed in" << timer.elapsed() << "ms";
    emit progressChanged(100);
    return result;
}

bool DirectoryWalker::matchesFilter(const QString &fileName,
                                    const QString &filter) const
{
    // 处理多个过滤规则(用;分隔)
    foreach(const QString & pattern, filter.split(';', QString::SkipEmptyParts))
    {
        // 判断是否是正则表达式
        if(pattern.startsWith('^') || pattern.contains('*') || pattern.contains('?'))
        {
            QRegExp regex(pattern, Qt::CaseInsensitive, QRegExp::Wildcard);
            if(regex.exactMatch(fileName))
            {
                return true;
            }
        }
        // 简单后缀匹配
        else if(fileName.endsWith(pattern, Qt::CaseInsensitive))
        {
            return true;
        }
    }
    return false;
}
