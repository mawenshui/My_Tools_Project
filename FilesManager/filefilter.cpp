#include "filefilter.h"
#include <QFileInfo>
#include <QDebug>

bool FileFilter::s_caseSensitive = false;

FileFilter::FileFilter(QObject *parent) : QObject(parent) {}

void FileFilter::setCaseSensitive(bool sensitive)
{
    s_caseSensitive = sensitive;
}

bool FileFilter::match(const QString &fileName, const QString &pattern)
{
    if(pattern.isEmpty())
    {
        return true;
    }
    for(const QString &singlePattern : pattern.split(';', QString::SkipEmptyParts))
    {
        QRegularExpression regex = createRegExp(singlePattern.trimmed());
        if(regex.match(fileName).hasMatch())
        {
            return true;
        }
    }
    return false;
}

QStringList FileFilter::filterFiles(const QStringList &files, const QString &pattern)
{
    QStringList result;
    for(const QString &file : files)
    {
        if(match(QFileInfo(file).fileName(), pattern))
        {
            result << file;
        }
    }
    return result;
}

QRegularExpression FileFilter::createRegExp(const QString &pattern)
{
    QRegularExpression::PatternOptions options = s_caseSensitive
        ? QRegularExpression::NoPatternOption
        : QRegularExpression::CaseInsensitiveOption;
    // 如果是简单通配符模式（如 *.txt）
    if(pattern.startsWith("*.") && !pattern.contains('?') && !pattern.contains('['))
    {
        QString escaped = QRegularExpression::escape(pattern.mid(1));
        return QRegularExpression("^" + escaped + "$", options);
    }
    // 判断是否已经是正则表达式
    if(pattern.startsWith('^') || pattern.contains('[') || pattern.contains('\\'))
    {
        return QRegularExpression(pattern, options);
    }
    // 将通配符模式转换为正则表达式
    QString regexPattern;
    for(const QChar &ch : pattern)
    {
        switch(ch.unicode())
        {
            case '*':
                regexPattern += ".*";
                break;
            case '?':
                regexPattern += ".";
                break;
            default:
                regexPattern += QRegularExpression::escape(ch);
                break;
        }
    }
    return QRegularExpression("^" + regexPattern + "$", options);
}
