// alldefine.h
#ifndef ALLDEFINE_H
#define ALLDEFINE_H

#include <QString>
#include <QList>

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

    enum SequencePosition { Prefix, Suffix, Replace };
    SequencePosition sequencePos = Prefix;

    bool caseEnabled = false;         // 是否修改大小写
    enum CaseType { Upper, Lower, Title };
    CaseType caseType = Upper;

    bool keepExtension = true;        // 是否保留扩展名
};

#endif // ALLDEFINE_H
