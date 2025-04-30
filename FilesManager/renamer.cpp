#include "renamer.h"
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QDebug>

Renamer::Renamer(QObject *parent)
    : QObject(parent),
      m_pattern("%f_%n"),
      m_startNumber(1),
      m_dateFormat("yyyy-MM-dd")
{
}

void Renamer::setPattern(const QString &pattern)
{
    m_pattern = pattern;
}

void Renamer::setStartNumber(int start)
{
    m_startNumber = start > 0 ? start : 1;
}

void Renamer::setDateFormat(const QString &format)
{
    m_dateFormat = format.isEmpty() ? "yyyy-MM-dd" : format;
}

QString Renamer::generateNewName(const QString &originalName, int index) const
{
    QFileInfo fi(originalName);
    QString baseName = fi.completeBaseName();
    QString suffix = fi.suffix();
    QString result = replaceVariables(baseName, index + m_startNumber);
    if(!suffix.isEmpty())
    {
        result += "." + suffix;
    }
    return result;
}

QStringList Renamer::batchRename(const QStringList &filePaths)
{
    QStringList results;
    int success = 0, fail = 0;
    for(int i = 0; i < filePaths.size(); ++i)
    {
        emit progressChanged(i + 1, filePaths.size());
        QFileInfo fi(filePaths[i]);
        QString newName = generateNewName(fi.fileName(), i);
        QString newPath = fi.dir().filePath(newName);
        // 处理文件名冲突
        newPath = ensureUniqueName(fi.path(), newName);
        if(QFile::rename(filePaths[i], newPath))
        {
            results << newPath;
            success++;
        }
        else
        {
            results << filePaths[i]; // 保留原路径表示失败
            fail++;
            qWarning() << "重命名失败:" << filePaths[i] << "->" << newPath;
        }
    }
    emit renameFinished(success, fail);
    return results;
}

QList<QPair<QString, QString >> Renamer::preview(const QStringList &filePaths) const
{
    QList<QPair<QString, QString >> previewList;

    for(int i = 0; i < filePaths.size(); ++i)
    {
        QFileInfo fi(filePaths[i]);
        QString newName = generateNewName(fi.fileName(), i);
        previewList << qMakePair(fi.fileName(), newName);
    }

    return previewList;
}

QString Renamer::replaceVariables(const QString &originalName, int index) const
{
    QString result = m_pattern;
    QString dateStr = QDateTime::currentDateTime().toString(m_dateFormat);
    // 替换模板变量
    result.replace("%n", QString::number(index))
          .replace("%d", dateStr)
          .replace("%f", originalName)
          .replace("%e", QFileInfo(originalName).suffix());
    // 支持大小写转换
    result.replace("%U", originalName.toUpper())
          .replace("%L", originalName.toLower())
          .replace("%C", originalName[0].toUpper() + originalName.mid(1).toLower());
    return result;
}

QString Renamer::ensureUniqueName(const QString &dirPath, const QString &newName) const
{
    QFileInfo fi(dirPath, newName);
    if(!fi.exists())
    {
        return fi.filePath();
    }
    // 处理文件名冲突
    QString baseName = fi.completeBaseName();
    QString suffix = fi.suffix();
    QString newBaseName;
    int counter = 1;
    do
    {
        newBaseName = baseName + "_" + QString::number(counter++);
        fi.setFile(dirPath, newBaseName + (suffix.isEmpty() ? "" : "." + suffix));
    }
    while(fi.exists());
    return fi.filePath();
}
