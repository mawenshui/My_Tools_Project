#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileSystemModel>
#include <QStringList>
#include <QAction>
#include <QMenu>
#include <QStatusBar>
#include <QTreeView>
#include <QTableView>
#include <QTextEdit>
#include <QComboBox>
#include <QSplitter>
#include <QTabWidget>

#include "previewmodel.h"

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
};

#endif // MAINWINDOW_H
