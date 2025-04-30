// alldefine.h
#ifndef ALLDEFINE_H
#define ALLDEFINE_H

#include <QString>
#include <QList>
#include <QDateTime>
#include <QStringList>
#include <QColor>

// 文件树列定义
enum FileTreeColumn
{
    COLUMN_NAME = 0,
    COLUMN_TYPE,
    COLUMN_SIZE,
    COLUMN_DATE,
    COLUMN_COUNT  // 列总数
};

// 文件树列设置
struct FileTreeSettings
{
    bool columnsVisible[COLUMN_COUNT] = {true, true, true, true};
    int columnWidths[COLUMN_COUNT] = {200, 80, 80, 120};
    int sortColumn = COLUMN_NAME;
    Qt::SortOrder sortOrder = Qt::AscendingOrder;
};

// 预览项数据结构
struct PreviewItem
{
    QString original;    // 原始文件名
    QString newName;    // 新文件名
    bool valid = true;  // 是否有效
    QString error;      // 错误信息
};

// 重命名规则结构体
struct RenameRules
{
    bool replaceEnabled = false;      // 是否启用替换
    QString replaceFrom;              // 查找内容
    QString replaceTo;                // 替换为
    bool useRegex = false;            // 是否使用正则表达式

    bool sequenceEnabled = false;     // 是否启用序号
    int sequenceStart = 1;            // 起始值
    int sequenceStep = 1;             // 步长
    int minDigits = 3;                // 最小位数

    enum SequencePosition { SequencePosition_Prefix, SequencePosition_Suffix, SequencePosition_Replace };
    SequencePosition sequencePos = SequencePosition_Prefix;

    bool caseEnabled = false;         // 是否修改大小写
    enum CaseType { CaseType_Upper, CaseType_Lower, CaseType_Title };
    CaseType caseType = CaseType_Upper;

    bool keepExtension = true;        // 是否保留扩展名

    // 新增后缀修改相关选项
    bool extensionEnabled = false;     // 是否启用后缀修改
    enum ExtensionAction { ExtensionAction_Add, ExtensionAction_Replace, ExtensionAction_Remove };
    ExtensionAction extensionAction = ExtensionAction_Replace; // 默认替换后缀
    QString extensionValue;           // 新的后缀值(不带点)
};

#endif // ALLDEFINE_H
