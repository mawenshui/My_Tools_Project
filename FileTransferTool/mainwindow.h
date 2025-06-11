#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QThread>
#include <QMutex>
#include <QApplication>
#include <QRadioButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QComboBox>
#include <QSpinBox>
#include <QDateTimeEdit>
#include <QTextEdit>
#include <QTabWidget>
#include <QGridLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QClipboard>
#include <QMimeData>
#include <QStandardPaths>
#include <QProcess>
#include "filetransferworker.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// 前向声明
class FileTransferWorker;
enum class TransferMode;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    /**
     * @brief 添加源文件夹
     */
    void addSourceFolders();
    
    /**
     * @brief 移除选中的源文件夹
     */
    void removeSelectedFolders();
    
    /**
     * @brief 选择目标目录
     */
    void selectTargetDirectory();
    
    /**
     * @brief 开始文件转移
     */
    void startTransfer();
    
    /**
     * @brief 更新转移进度
     * @param current 当前进度
     * @param total 总数
     * @param fileName 当前文件名
     */
    void updateProgress(int current, int total, const QString &fileName);
    
    /**
     * @brief 转移完成处理
     * @param success 是否成功
     * @param message 结果消息
     */
    void onTransferFinished(bool success, const QString &message);

private:
    Ui::MainWindow *ui;
    
    // UI组件
    QListWidget *m_sourceListWidget;     // 源文件夹列表
    QPushButton *m_addFolderBtn;         // 添加文件夹按钮
    QPushButton *m_removeFolderBtn;      // 移除文件夹按钮
    QLineEdit *m_targetPathEdit;         // 目标路径输入框
    QPushButton *m_selectTargetBtn;      // 选择目标目录按钮
    QPushButton *m_startTransferBtn;     // 开始转移按钮
    QProgressBar *m_progressBar;         // 进度条
    QLabel *m_statusLabel;               // 状态标签
    
    // 转移模式选择
    QRadioButton *m_keepStructureRadio;
    QRadioButton *m_flattenFilesRadio;
    QCheckBox *m_overwriteCheckBox;
    
    // 筛选功能UI控件
    QTabWidget *m_filterTabWidget;       // 筛选选项卡
    
    // 文件类型筛选
    QCheckBox *m_enableFileTypeFilter;
    QComboBox *m_fileTypeCombo;
    QLabel *m_fileTypeExtensionsLabel;   // 显示文件类型包含的后缀名
    QLineEdit *m_customExtensions;
    
    // 文件大小筛选
    QCheckBox *m_enableSizeFilter;
    QSpinBox *m_minSizeSpinBox;
    QSpinBox *m_maxSizeSpinBox;
    QComboBox *m_sizeUnitCombo;
    
    // 修改时间筛选
    QCheckBox *m_enableTimeFilter;
    QDateTimeEdit *m_startDateTime;
    QDateTimeEdit *m_endDateTime;
    QComboBox *m_timeRangeCombo;
    
    // 文件名模式匹配
    QCheckBox *m_enableNameFilter;
    QLineEdit *m_namePatternEdit;
    QComboBox *m_patternTypeCombo;
    
    // 排除列表
    QCheckBox *m_enableExcludeFilter;
    QTextEdit *m_excludeListEdit;
    QPushButton *m_addExcludeBtn;
    QPushButton *m_clearExcludeBtn;
    
    // 工作线程
    QThread *m_workerThread;
    FileTransferWorker *m_worker;
    
    /**
     * @brief 初始化UI界面
     */
    void initUI();
    
    /**
     * @brief 设置UI状态
     * @param transferring 是否正在转移
     */
    void setUIEnabled(bool enabled);
    
    /**
     * @brief 获取预定义文件类型的扩展名列表
     * @param fileType 文件类型名称
     * @return 扩展名列表
     */
    QStringList getFileTypeExtensions(const QString &fileType);
    
    /**
     * @brief 将大小单位转换为字节
     * @param size 大小值
     * @param unit 单位名称
     * @return 字节数
     */
    qint64 convertSizeToBytes(int size, const QString &unit);
    
    /**
     * @brief 更新文件类型后缀显示
     * @param fileType 文件类型名称
     */
    void updateFileTypeExtensionsDisplay(const QString &fileType);
    
    // 文件管理功能
    /**
     * @brief 复制选中的文件/文件夹
     */
    void copySelectedItems();
    
    /**
     * @brief 剪切选中的文件/文件夹
     */
    void cutSelectedItems();
    
    /**
     * @brief 粘贴文件/文件夹
     */
    void pasteItems();
    
    /**
     * @brief 删除选中的文件/文件夹
     */
    void deleteSelectedItems();
    
    /**
     * @brief 重命名选中的文件/文件夹
     */
    void renameSelectedItem();
    
    /**
     * @brief 查找文件/文件夹
     */
    void findFiles();
    
    /**
     * @brief 查看文件/文件夹属性
     */
    void showProperties();
    
    /**
     * @brief 刷新文件列表
     */
    void refreshFileList();
    
    /**
     * @brief 双击文件列表项处理
     * @param item 被双击的项
     */
    void onFileListDoubleClicked(QListWidgetItem *item);
    
    /**
     * @brief 文件列表右键菜单
     * @param pos 右键位置
     */
    void showFileListContextMenu(const QPoint &pos);
    
    /**
     * @brief 更新文件管理按钮状态
     */
    void updateFileManagementButtons();
    
private:
    // 文件管理相关成员变量
    QStringList m_clipboardPaths;  // 剪贴板路径列表
    bool m_isCutOperation;          // 是否为剪切操作
    QString m_currentDirectory;     // 当前浏览目录
    
    // 文件管理UI组件
    QListWidget *m_fileListWidget;      // 文件浏览列表
    QPushButton *m_copyBtn;             // 复制按钮
    QPushButton *m_cutBtn;              // 剪切按钮
    QPushButton *m_pasteBtn;            // 粘贴按钮
    QPushButton *m_deleteBtn;           // 删除按钮
    QPushButton *m_renameBtn;           // 重命名按钮
    QPushButton *m_findBtn;             // 查找按钮
    QPushButton *m_propertiesBtn;       // 属性按钮
    QPushButton *m_refreshBtn;          // 刷新按钮
    QPushButton *m_upDirBtn;            // 上级目录按钮
    QLineEdit *m_currentPathEdit;       // 当前路径显示
    QComboBox *m_driveComboBox;         // 盘符选择下拉框
    
    /**
     * @brief 初始化文件管理UI
     */
    void initFileManagementUI();
    
    /**
     * @brief 加载目录内容
     * @param dirPath 目录路径
     */
    void loadDirectoryContent(const QString &dirPath);
    
    /**
     * @brief 执行文件复制操作
     * @param sourcePaths 源路径列表
     * @param targetDir 目标目录
     * @param isCut 是否为剪切操作
     * @return 是否成功
     */
    bool performFileCopy(const QStringList &sourcePaths, const QString &targetDir, bool isCut = false);
    
    /**
     * @brief 获取选中的文件路径列表
     * @return 文件路径列表
     */
    QStringList getSelectedFilePaths();
    
    /**
     * @brief 格式化文件大小显示
     * @param bytes 字节数
     * @return 格式化后的大小字符串
     */
    QString formatFileSize(qint64 bytes);
    
    /**
     * @brief 获取文件图标类型
     * @param filePath 文件路径
     * @return 图标类型字符串
     */
    QString getFileIconType(const QString &filePath);
    
    /**
     * @brief 递归复制目录
     * @param sourceDir 源目录
     * @param targetDir 目标目录
     * @return 是否成功
     */
    bool copyDirectoryRecursively(const QString &sourceDir, const QString &targetDir);
};
#endif // MAINWINDOW_H
