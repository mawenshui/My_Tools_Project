#ifndef CSV_EXPORT_DIALOG_H
#define CSV_EXPORT_DIALOG_H

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
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>

/**
 * @brief CSV导出配置对话框
 * 
 * 允许用户选择要导出的内容类型和导出方式
 * 支持导出概览、详细表格、文件列表、语言统计
 * 支持单文件或多文件导出模式
 * 
 * @author CodeVisualization Team
 * @date 2024-12-01
 * @version 1.0.0
 */
class CSVExportDialog : public QDialog
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
    explicit CSVExportDialog(QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~CSVExportDialog();
    
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
     * @brief 获取选择的编码格式
     * @return 编码格式名称
     */
    QString getSelectedEncoding() const;
    
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
     * @brief 初始化UI界面
     */
    void initializeUI();
    
    /**
     * @brief 创建内容选择区域
     * @return 内容选择组件
     */
    QGroupBox* createContentSelectionGroup();
    
    /**
     * @brief 创建导出模式选择区域
     * @return 导出模式选择组件
     */
    QGroupBox* createExportModeGroup();
    
    /**
     * @brief 创建路径选择组
     * @return 路径选择组组件
     */
    QGroupBox* createPathSelectionGroup();
    
    /**
     * @brief 创建编码选择组
     * @return 编码选择组组件
     */
    QGroupBox* createEncodingSelectionGroup();
    
    /**
     * @brief 创建按钮区域
     * @return 按钮区域组件
     */
    QWidget* createButtonArea();
    
    /**
     * @brief 验证输入
     * @return 验证是否通过
     */
    bool validateInput();
    
    /**
     * @brief 更新UI状态
     */
    void updateUIState();

private:
    // 布局
    QVBoxLayout *m_mainLayout;          ///< 主布局
    
    // 内容选择
    QGroupBox *m_contentGroup;          ///< 内容选择组
    QCheckBox *m_overviewCheckBox;      ///< 概览复选框
    QCheckBox *m_detailedTableCheckBox; ///< 详细表格复选框
    QCheckBox *m_fileListCheckBox;      ///< 文件列表复选框
    QCheckBox *m_languageStatsCheckBox; ///< 语言统计复选框
    QPushButton *m_selectAllButton;     ///< 全选按钮
    QPushButton *m_selectNoneButton;    ///< 全不选按钮
    
    // 导出模式
    QGroupBox *m_modeGroup;             ///< 导出模式组
    QRadioButton *m_singleFileRadio;    ///< 单文件单选按钮
    QRadioButton *m_multipleFilesRadio; ///< 多文件单选按钮
    QButtonGroup *m_modeButtonGroup;    ///< 模式按钮组
    
    // 路径选择组件
    QGroupBox *m_pathGroup;             ///< 路径选择组
    QLineEdit *m_pathEdit;              ///< 路径编辑框
    QPushButton *m_browseButton;        ///< 浏览按钮
    QLabel *m_pathLabel;                ///< 路径标签
    QLineEdit *m_prefixEdit;            ///< 文件前缀编辑框
    QLabel *m_prefixLabel;              ///< 前缀标签
    
    // 编码选择组件
    QGroupBox *m_encodingGroup;         ///< 编码选择组
    QComboBox *m_encodingComboBox;      ///< 编码选择下拉框
    QLabel *m_encodingLabel;            ///< 编码标签
    
    // 按钮组件
    QPushButton *m_okButton;            ///< 确定按钮
    QPushButton *m_cancelButton;        ///< 取消按钮
};

Q_DECLARE_OPERATORS_FOR_FLAGS(CSVExportDialog::ExportContents)

#endif // CSV_EXPORT_DIALOG_H