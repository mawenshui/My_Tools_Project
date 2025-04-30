#ifndef FILEOPERATION_H
#define FILEOPERATION_H

#include <QObject>
#include <QAtomicInteger>
#include <QFile>
#include "resumabletransfer.h"
#include "conflictresolver.h"
#include "directorywalker.h"

/**
 * @brief 文件操作核心类 - 实现多线程文件复制/移动
 *
 * 功能特点：
 * 1. 支持复制/移动两种操作模式
 * 2. 多线程分块处理大文件
 * 3. 集成断点续传和冲突解决
 * 4. 实时进度反馈和错误处理
 */
class FileOperation : public QObject
{
    Q_OBJECT
public:
    enum OperationType { Copy, Move };
    Q_ENUM(OperationType)

    explicit FileOperation(QObject *parent = nullptr);
    ~FileOperation();

public slots:
    /**
     * @brief 开始文件操作
     * @param srcDir 源目录
     * @param destDir 目标目录
     * @param type 操作类型(复制/移动)
     * @param recursive 是否递归子目录
     * @param conflictRes 冲突解决策略
     * @param filter 文件过滤规则
     */
    void startOperation(const QString &srcDir,
                        const QString &destDir,
                        OperationType type,
                        bool recursive,
                        ConflictResolver::Resolution conflictRes,
                        const QString &filter);

    void pause();
    void resume();
    void cancel();

signals:
    void progressChanged(int percent);  // 进度百分比(0-100)
    void currentFileChanged(const QString &file); // 当前处理文件变化
    void statusMessage(const QString &message); // 状态消息
    void operationFinished();          // 操作完成
    void errorOccurred(const QString &error); // 错误发生

private:
    QAtomicInt m_paused;               // 暂停状态原子标志
    QAtomicInt m_canceled;             // 取消状态原子标志
    ResumableTransfer m_transfer;      // 断点续传处理器
    DirectoryWalker m_walker;          // 目录遍历器

    bool processFile(const QString &srcPath,
                     const QString &destPath,
                     OperationType type,
                     ConflictResolver::Resolution conflictRes);

    QString errorToString(QFile::FileError error) const;
};

#endif // FILEOPERATION_H
