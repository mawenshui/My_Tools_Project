// multipliersettingsdialog.h
#ifndef MULTIPLIERSETTINGSDIALOG_H
#define MULTIPLIERSETTINGSDIALOG_H

#include <QDialog>

#include "../configmanager.h"
#include "../alldefine.h"

/**
 * @brief 倍率设置对话框
 * UI 完全在 .ui 文件中声明与布局；此类仅负责数据读写与信号连接。
 */
namespace Ui { class MultiplierSettingsDialog; }
class MultiplierSettingsDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * @brief 构造对话框
     * @param cfg 配置管理器实例（非空）
     */
    explicit MultiplierSettingsDialog(ConfigManager* cfg, QWidget* parent = nullptr);
    ~MultiplierSettingsDialog();

private slots:
    /**
     * @brief 保存并关闭对话框
     * 将当前三个倍率写入配置文件（范围0.001–3.0校验由控件保证）。
     */
    void onAccept();

private:
    ConfigManager* m_cfg;
    Ui::MultiplierSettingsDialog* ui; // 指向由 uic 生成的 UI 类实例
};

#endif // MULTIPLIERSETTINGSDIALOG_H
