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
};
#endif // MAINWINDOW_H
