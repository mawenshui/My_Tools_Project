#ifndef ALLDEFINE_H
#define ALLDEFINE_H

#include <QString>
#include <QDateTime>
#include <QColor>

/**
 * @brief 最大日志条目
 */
const int MAX_LOG_ENTRIES = 100000;

/**
 * @brief 日期时间格式
 */
const QString dt_format = "yyyy-MM-dd hh:mm:ss.zzz";

/**
 * @brief 日志条目结构
 */
struct LogEntry
{
    QString level;   //日志级别
    QString message; //日志消息
    QDateTime time;  //日志时间
};

/**
 * @brief 日志颜色主题
 */
struct ThemeColors
{
    QColor info = QColor(100, 255, 100);    //信息级别颜色（柔和的绿色）
    QColor warn = QColor(255, 200, 100);    //警告级别颜色（柔和的橙色）
    QColor error = QColor(255, 100, 100);   //错误级别颜色（柔和的红色）
    QColor debug = QColor(100, 200, 255);   //调试级别颜色（柔和的蓝色）
    QColor defualt = QColor(150, 150, 150); //默认颜色（浅灰色）
};

/**
 * @brief 配置错误枚举
 */
enum class ConfigError
{
    NoError,               //无错误
    FileNotFound,          //配置文件未找到
    PermissionDenied,      //文件权限不足
    InvalidFormat,         //配置文件格式无效
    WriteFailed,           //配置文件写入失败
    VersionMismatch,       //配置文件版本不匹配
    InvalidKey,            //无效的配置键
    InvalidValue,          //无效的配置值
    DirectoryCreationFailed //目录创建失败
};

#endif // ALLDEFINE_H
