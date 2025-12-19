// multiplierhelpdialog.h
#ifndef MULTIPLIERHELPDIALOG_H
#define MULTIPLIERHELPDIALOG_H

#include <QDialog>

#include "../configmanager.h"
#include "../alldefine.h"

/**
 * @brief 倍率帮助对话框
 * UI 完全在 .ui 文件中声明与布局；此类仅负责信号连接与配置操作。
 */
namespace Ui { class MultiplierHelpDialog; }
class MultiplierHelpDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * @brief 构造帮助对话框
     * @param cfg 配置管理器实例（用于恢复默认）
     */
    explicit MultiplierHelpDialog(ConfigManager* cfg, QWidget* parent = nullptr);
    ~MultiplierHelpDialog();

private slots:
    /**
     * @brief 恢复默认倍率
     * 将三个倍率重置为 alldefine.h 中的默认常量，并立即写入配置。
     */
    void onRestoreDefaults();

private:
    ConfigManager* m_cfg;
    Ui::MultiplierHelpDialog* ui; // 指向由 uic 生成的 UI 类实例
};

#endif // MULTIPLIERHELPDIALOG_H
