#include "file_statistics.h"
#include <QtCore/QFileInfo>

FileStatistics::FileStatistics()
    : totalLines(0)
    , codeLines(0)
    , commentLines(0)
    , blankLines(0)
    , fileSize(0)
{
}

FileStatistics::FileStatistics(const QString &path)
    : filePath(path)
    , totalLines(0)
    , codeLines(0)
    , commentLines(0)
    , blankLines(0)
    , fileSize(0)
{
    QFileInfo fileInfo(path);
    if (fileInfo.exists()) {
        fileName = fileInfo.fileName();
        extension = fileInfo.suffix().toLower();
        fileSize = fileInfo.size();
        lastModified = fileInfo.lastModified();
        
        // 根据扩展名确定语言类型
        if (extension == "cpp" || extension == "cc" || extension == "cxx") {
            language = "C++";
        } else if (extension == "h" || extension == "hpp" || extension == "hxx") {
            language = "C++ Header";
        } else if (extension == "c") {
            language = "C";
        } else if (extension == "py") {
            language = "Python";
        } else if (extension == "java") {
            language = "Java";
        } else if (extension == "js") {
            language = "JavaScript";
        } else if (extension == "ts") {
            language = "TypeScript";
        } else if (extension == "cs") {
            language = "C#";
        } else if (extension == "php") {
            language = "PHP";
        } else if (extension == "rb") {
            language = "Ruby";
        } else if (extension == "go") {
            language = "Go";
        } else if (extension == "rs") {
            language = "Rust";
        } else if (extension == "swift") {
            language = "Swift";
        } else if (extension == "kt") {
            language = "Kotlin";
        } else {
            language = "Unknown";
        }
    }
}

void FileStatistics::reset()
{
    totalLines = 0;
    codeLines = 0;
    commentLines = 0;
    blankLines = 0;
}

bool FileStatistics::isValid() const
{
    return !filePath.isEmpty() && totalLines >= 0 && 
           (codeLines + commentLines + blankLines) <= totalLines;
}

double FileStatistics::getCodeDensity() const
{
    if (totalLines == 0) {
        return 0.0;
    }
    return (static_cast<double>(codeLines) / totalLines) * 100.0;
}

double FileStatistics::getCommentDensity() const
{
    if (totalLines == 0) {
        return 0.0;
    }
    return (static_cast<double>(commentLines) / totalLines) * 100.0;
}