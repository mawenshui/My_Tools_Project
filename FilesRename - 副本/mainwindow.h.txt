#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileSystemModel>
#include <QAction>
#include <QMenu>
#include <QStatusBar>
#include <QTreeView>
#include <QTableView>
#include <QTextEdit>
#include <QComboBox>
#include <QSplitter>
#include <QTabWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QInputDialog>
#include <QDir>
#include <QDateTime>
#include <QRegularExpression>
#include <QHeaderView>
#include <QDesktopServices>
#include <QProcess>
#include <QClipboard>
#include <QDebug>

#include "alldefine.h"
#include "previewmodel.h"
#include "filedelegate.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    // 自动连接的槽函数
    void onOpenFolder();
    void onGeneratePreview();
    void onExecuteRename();
    void onPathComboChanged(const QString &path);

    // 手动连接的槽函数
    void showContextMenu(const QPoint &pos);
    void updateRecentFoldersMenu();
//    void openRecentFolder();

private:
    Ui::MainWindow* ui;
    QFileSystemModel* fsModel;
    PreviewModel* previewModel;

    // 动态菜单成员
    QMenu* recentFoldersMenu;
    QStringList recentFolders;

private:
    // 处理文件后缀修改
    QString processExtension(const QString& original, const RenameRules& rules) const;
    // 验证后缀修改规则
    bool validateExtensionRules(const RenameRules& rules, QString& error) const;

    // 初始化方法
    void initFileSystemModel();
    void initRecentFoldersMenu();
    void setCurrentFolder(const QString &path);
    void addToRecentFolders(const QString &path);

    // 文件操作核心方法
    QFileInfoList getSelectedFiles() const;
    void processRenameRules();
    bool validateNames(const QList<PreviewItem>& items) const;
    void logOperation(const QString &message);

    // 规则数据结构
    RenameRules collectRules() const;
    QString applyRenameRules(const QString &original, int& counter) const;

private:
    // 文件树相关方法
    void setupFileTree();
    void saveFileTreeSettings();
    void loadFileTreeSettings();
    void updateFileTreeColumns();

    // 上下文菜单
    void createFileTreeContextMenu();
    void showFileTreeHeaderMenu(const QPoint& pos);

    // 文件树设置
    FileTreeSettings m_fileTreeSettings;

private slots:
    void onFileTreeHeaderClicked(int logicalIndex);
    void onFileTreeCustomContextMenuRequested(const QPoint &pos);
    void onFileTreeHeaderCustomContextMenuRequested(const QPoint &pos);
};

#endif // MAINWINDOW_H
