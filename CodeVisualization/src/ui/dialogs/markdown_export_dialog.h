#ifndef MARKDOWN_EXPORT_DIALOG_H
#define MARKDOWN_EXPORT_DIALOG_H

#include <QtWidgets/QDialog>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>

/**
 * @brief Markdown导出配置对话框
 * 
 * 允许用户选择要导出的内容类型和导出方式
 * 支持导出概览、详细表格、文件列表、语言统计
 * 支持单文件或多文件导出模式
 * 
 * @author CodeVisualization Team
 * @date 2024-12-01
 * @version 1.0.0
 */
class MarkdownExportDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 导出内容类型枚举
     */
    enum ExportContent {
        Overview = 0x01,        ///< 概览信息
        DetailedTable = 0x02,   ///< 详细表格
        FileList = 0x04,        ///< 文件列表
        LanguageStats = 0x08    ///< 语言统计
    };
    Q_DECLARE_FLAGS(ExportContents, ExportContent)
    
    /**
     * @brief 导出模式枚举
     */
    enum ExportMode {
        SingleFile,    ///< 单文件模式
        MultipleFiles  ///< 多文件模式
    };
    
    /**
     * @brief 构造函数
     * @param parent 父组件指针
     */
    explicit MarkdownExportDialog(QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~MarkdownExportDialog();
    
    /**
     * @brief 获取选择的导出内容
     * @return 导出内容标志
     */
    ExportContents getSelectedContents() const;
    
    /**
     * @brief 获取导出模式
     * @return 导出模式
     */
    ExportMode getExportMode() const;
    
    /**
     * @brief 获取导出路径
     * @return 导出路径
     */
    QString getExportPath() const;
    
    /**
     * @brief 获取文件名前缀（多文件模式时使用）
     * @return 文件名前缀
     */
    QString getFilePrefix() const;
    
    /**
     * @brief 设置默认导出路径
     * @param path 默认路径
     */
    void setDefaultPath(const QString &path);

private slots:
    /**
     * @brief 浏览按钮点击槽函数
     */
    void onBrowseClicked();
    
    /**
     * @brief 导出模式改变槽函数
     */
    void onExportModeChanged();
    
    /**
     * @brief 确定按钮点击槽函数
     */
    void onOkClicked();
    
    /**
     * @brief 取消按钮点击槽函数
     */
    void onCancelClicked();
    
    /**
     * @brief 全选按钮点击槽函数
     */
    void onSelectAllClicked();
    
    /**
     * @brief 全不选按钮点击槽函数
     */
    void onSelectNoneClicked();

private:
    /**
     * @brief 初始化用户界面
     */
    void initializeUI();
    
    /**
     * @brief 创建内容选择组
     * @return 内容选择组指针
     */
    QGroupBox* createContentSelectionGroup();
    
    /**
     * @brief 创建导出模式组
     * @return 导出模式组指针
     */
    QGroupBox* createExportModeGroup();
    
    /**
     * @brief 创建路径选择组
     * @return 路径选择组指针
     */
    QGroupBox* createPathSelectionGroup();
    
    /**
     * @brief 创建按钮区域
     * @return 按钮区域指针
     */
    QWidget* createButtonArea();
    
    /**
     * @brief 验证输入
     * @return 输入是否有效
     */
    bool validateInput();
    
    /**
     * @brief 更新UI状态
     */
    void updateUIState();

private:
    // 主布局
    QVBoxLayout *m_mainLayout;          ///< 主布局
    
    // 内容选择组
    QGroupBox *m_contentGroup;          ///< 内容选择组
    QCheckBox *m_overviewCheckBox;      ///< 概览复选框
    QCheckBox *m_detailedTableCheckBox; ///< 详细表格复选框
    QCheckBox *m_fileListCheckBox;      ///< 文件列表复选框
    QCheckBox *m_languageStatsCheckBox; ///< 语言统计复选框
    QPushButton *m_selectAllButton;     ///< 全选按钮
    QPushButton *m_selectNoneButton;    ///< 全不选按钮
    
    // 导出模式组
    QGroupBox *m_modeGroup;             ///< 导出模式组
    QRadioButton *m_singleFileRadio;    ///< 单文件单选按钮
    QRadioButton *m_multipleFilesRadio; ///< 多文件单选按钮
    QButtonGroup *m_modeButtonGroup;    ///< 模式按钮组
    
    // 路径选择组
    QGroupBox *m_pathGroup;             ///< 路径选择组
    QLineEdit *m_pathEdit;              ///< 路径编辑框
    QPushButton *m_browseButton;        ///< 浏览按钮
    QLabel *m_pathLabel;                ///< 路径标签
    QLineEdit *m_prefixEdit;            ///< 文件前缀编辑框
    QLabel *m_prefixLabel;              ///< 前缀标签
    
    // 按钮区域
    QPushButton *m_okButton;            ///< 确定按钮
    QPushButton *m_cancelButton;        ///< 取消按钮
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MarkdownExportDialog::ExportContents)

#endif // MARKDOWN_EXPORT_DIALOG_H