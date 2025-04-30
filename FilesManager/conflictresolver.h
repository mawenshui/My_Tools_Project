#ifndef CONFLICTRESOLVER_H
#define CONFLICTRESOLVER_H

#include <QObject>
#include <QFileInfo>

/**
 * @brief 文件冲突解决器 - 处理目标文件已存在的情况
 *
 * 功能特点：
 * 1. 提供覆盖/跳过/重命名三种解决策略
 * 2. 智能生成不重复的重命名方案
 * 3. 支持自定义重命名模板
 * 4. 线程安全设计
 */
class ConflictResolver : public QObject
{
    Q_OBJECT
public:
    enum Resolution
    {
        Overwrite,  // 覆盖现有文件
        Skip,       // 跳过该文件
        Rename,     // 自动重命名
        AskUser     // 询问用户(需要GUI支持)
    };
    Q_ENUM(Resolution)

    explicit ConflictResolver(QObject *parent = nullptr);

    /**
     * @brief 解决文件冲突
     * @param destPath 目标文件路径
     * @param resolution 解决策略
     * @return 实际应该使用的目标路径
     */
    QString resolveConflict(const QString &destPath,
                            Resolution resolution = Rename);

    /**
     * @brief 设置默认解决策略
     * @param resolution 默认策略
     */
    void setDefaultResolution(Resolution resolution);

    /**
     * @brief 设置重命名模板
     * @param templateStr 模板字符串(支持%1=序号, %2=时间戳, %3=原文件名)
     */
    void setRenameTemplate(const QString &templateStr);

signals:
    /**
     * @brief 冲突解决结果通知
     * @param originalPath 原始目标路径
     * @param resolvedPath 解决后的路径
     * @param action 采取的动作(覆盖/跳过/重命名)
     */
    void conflictResolved(const QString &originalPath,
                          const QString &resolvedPath,
                          Resolution action);

private:
    Resolution m_defaultResolution;//默认解决策略
    QString m_renameTemplate;

    //生成新文件名避免冲突
    QString generateNewName(const QString &originalPath) const;
    QString getTimestampSuffix() const;
};

#endif // CONFLICTRESOLVER_H
