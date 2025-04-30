#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "previewmodel.h"
#include "alldefine.h"

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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , fsModel(new QFileSystemModel(this))
    , previewModel(new PreviewModel(this))
{
    ui->setupUi(this);
    // 初始化文件系统模型
    fsModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    fsModel->setRootPath("");
    ui->treeView->setModel(fsModel);
    ui->treeView->setHeaderHidden(true);
    // 初始化预览表格
    ui->previewTable->setModel(previewModel);
    ui->previewTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // 初始化最近文件夹菜单
    recentFoldersMenu = new QMenu(tr("最近打开的文件夹"), this);
    ui->menu_File->insertMenu(ui->action_Exit, recentFoldersMenu);
    // 加载历史记录
    QSettings settings;
    recentFolders = settings.value("recentFolders").toStringList();
    updateRecentFoldersMenu();
    // 启用拖放
    setAcceptDrops(true);
    ui->treeView->setDragEnabled(true);
    ui->treeView->setAcceptDrops(true);
    // 连接信号槽
    connect(ui->action_OpenFolder, &QAction::triggered, this, &MainWindow::onOpenFolder);
    connect(ui->pushButton_Preview, &QPushButton::clicked, this, &MainWindow::onGeneratePreview);
    connect(ui->pushButton_Execute, &QPushButton::clicked, this, &MainWindow::onExecuteRename);
    connect(ui->treeView, &QTreeView::customContextMenuRequested, this, &MainWindow::showContextMenu);
    connect(ui->comboBox_Path, &QComboBox::currentTextChanged, this, &MainWindow::onPathComboChanged);
    // 默认打开用户目录
    setCurrentFolder(QDir::homePath());
}

MainWindow::~MainWindow()
{
    delete ui;
}

// 文件系统操作
void MainWindow::onOpenFolder()
{
    QString folderPath = QFileDialog::getExistingDirectory(
                             this,
                             tr("选择文件夹"),
                             QDir::homePath(),
                             QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
                         );
    if(!folderPath.isEmpty())
    {
        setCurrentFolder(folderPath);
        addToRecentFolders(folderPath);
    }
}

void MainWindow::setCurrentFolder(const QString &path)
{
    if(!QFileInfo(path).isDir())
    {
        return;
    }
    fsModel->setRootPath(path);
    QModelIndex index = fsModel->index(path);
    ui->treeView->setRootIndex(index);
    ui->treeView->expand(index);
    // 更新路径组合框
    ui->comboBox_Path->blockSignals(true);
    ui->comboBox_Path->clear();
    ui->comboBox_Path->addItem(QIcon(":/icons/icons/folder.png"), path);
    ui->comboBox_Path->blockSignals(false);
    statusBar()->showMessage(tr("当前文件夹: %1").arg(path), 3000);
}

// 最近文件夹管理
void MainWindow::addToRecentFolders(const QString &path)
{
    if(path.isEmpty())
    {
        return;
    }
    recentFolders.removeAll(path);
    recentFolders.prepend(path);
    // 限制最大数量
    while(recentFolders.size() > 5)
    {
        recentFolders.removeLast();
    }
    // 保存到设置
    QSettings settings;
    settings.setValue("recentFolders", recentFolders);
    updateRecentFoldersMenu();
}

void MainWindow::updateRecentFoldersMenu()
{
    recentFoldersMenu->clear();
    if(recentFolders.isEmpty())
    {
        recentFoldersMenu->addAction(tr("无最近记录"))->setEnabled(false);
        return;
    }
    for(const QString &folder : recentFolders)
    {
        QAction *action = recentFoldersMenu->addAction(
                              QIcon(":/icons/icons/folder.png"),
                              folder
                          );
        connect(action, &QAction::triggered, this, [this, folder]()
        {
            setCurrentFolder(folder);
        });
    }
}

// 拖放支持
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    if(!mimeData->hasUrls())
    {
        return;
    }
    QUrl url = mimeData->urls().first();
    if(url.isLocalFile())
    {
        QString path = url.toLocalFile();
        if(QFileInfo(path).isDir())
        {
            setCurrentFolder(path);
            addToRecentFolders(path);
        }
    }
}

// 上下文菜单
void MainWindow::showContextMenu(const QPoint &pos)
{
    QMenu menu;
    menu.addAction(ui->action_OpenFolder);
    if(!recentFolders.isEmpty())
    {
        menu.addMenu(recentFoldersMenu);
    }
    menu.addSeparator();
    menu.addAction(QIcon(":/icons/icons/refresh.png"), tr("刷新"), [this]()
    {
        fsModel->setRootPath(fsModel->rootPath()); // 替代refresh()
    });
    menu.addAction(tr("新建文件夹"), [this]()
    {
        QString folderName = QInputDialog::getText(
                                 this,
                                 tr("新建文件夹"),
                                 tr("请输入文件夹名称:"),
                                 QLineEdit::Normal,
                                 "",
                                 nullptr,
                                 Qt::MSWindowsFixedSizeDialogHint
                             );
        if(!folderName.isEmpty())
        {
            QDir currentDir(fsModel->filePath(ui->treeView->rootIndex()));
            if(currentDir.mkdir(folderName))
            {
                fsModel->setRootPath(fsModel->rootPath()); // 刷新视图
            }
            else
            {
                QMessageBox::warning(this, tr("错误"), tr("创建文件夹失败"));
            }
        }
    });
    menu.exec(ui->treeView->viewport()->mapToGlobal(pos));
}

// 路径组合框处理
void MainWindow::onPathComboChanged(const QString &path)
{
    if(QFileInfo(path).isDir())
    {
        setCurrentFolder(path);
        addToRecentFolders(path);
    }
}

// 文件重命名核心功能
QFileInfoList MainWindow::getSelectedFiles() const
{
    QFileInfoList files;
    QModelIndexList indexes = ui->treeView->selectionModel()->selectedRows();
    for(const QModelIndex &index : indexes)
    {
        QString path = fsModel->filePath(index);
        QFileInfo fileInfo(path);
        if(fileInfo.isDir())
        {
            QDir dir(path);
            files += dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
        }
        else
        {
            files << fileInfo;
        }
    }
    return files;
}

RenameRules MainWindow::collectRules() const
{
    RenameRules rules;
    // 替换规则
    rules.replaceEnabled = ui->checkBox_Replace->isChecked();
    rules.replaceFrom = ui->lineEdit_Find->text();
    rules.replaceTo = ui->lineEdit_Replace->text();
    rules.useRegex = ui->checkBox_Regex->isChecked();
    // 序号规则
    rules.sequenceEnabled = ui->checkBox_Sequence->isChecked();
    rules.sequenceStart = ui->spinBox_Start->value();
    rules.sequenceStep = ui->spinBox_Step->value();
    rules.minDigits = ui->spinBox_Digits->value();
    rules.sequencePos = static_cast<RenameRules::SequencePosition>(ui->comboBox_Position->currentIndex());
    // 大小写规则
    rules.caseEnabled = ui->checkBox_Case->isChecked();
    rules.caseType = static_cast<RenameRules::CaseType>(ui->comboBox_Case->currentIndex());
    // 扩展名规则
    rules.keepExtension = ui->checkBox_Extension->isChecked();
    return rules;
}

QString MainWindow::applyRenameRules(const QString &original, int& counter) const
{
    RenameRules rules = collectRules(); // 使用局部变量替代currentRules
    QFileInfo fi(original);
    QString baseName = fi.completeBaseName();
    QString extension = fi.suffix();
    // 应用替换规则
    if(rules.replaceEnabled && !rules.replaceFrom.isEmpty())
    {
        if(rules.useRegex)
        {
            QRegularExpression regex(rules.replaceFrom);
            baseName.replace(regex, rules.replaceTo);
        }
        else
        {
            baseName.replace(rules.replaceFrom, rules.replaceTo, Qt::CaseInsensitive);
        }
    }
    // 应用序号规则
    if(rules.sequenceEnabled)
    {
        QString seq = QString::number(counter).rightJustified(rules.minDigits, '0');
        switch(rules.sequencePos)
        {
            case RenameRules::Prefix:
                baseName.prepend(seq + "_");
                break;
            case RenameRules::Suffix:
                baseName.append("_" + seq);
                break;
            case RenameRules::Replace:
                baseName = seq;
                break;
        }
        counter += rules.sequenceStep;
    }
    // 应用大小写规则
    if(rules.caseEnabled)
    {
        switch(rules.caseType)
        {
            case RenameRules::Upper:
                baseName = baseName.toUpper();
                break;
            case RenameRules::Lower:
                baseName = baseName.toLower();
                break;
            case RenameRules::Title:
                bool prevSpace = true;
                for(QChar &c : baseName)
                {
                    if(prevSpace && c.isLetter())
                    {
                        c = c.toUpper();
                        prevSpace = false;
                    }
                    else
                    {
                        c = c.toLower();
                        prevSpace = c.isSpace();
                    }
                }
                break;
        }
    }
    // 处理扩展名
    QString newName = baseName;
    if(rules.keepExtension && !extension.isEmpty())
    {
        newName += "." + extension;
    }
    // 清理非法字符
    static QRegularExpression illegalChars(R"([\/:*?"<>|])");
    newName.replace(illegalChars, "");
    // 处理保留文件名
    static QStringList reservedNames = {"CON", "PRN", "AUX", "NUL", "COM1", "LPT1"};
    QString namePart = newName.split('.').first().toUpper();
    if(reservedNames.contains(namePart))
    {
        newName = "_" + newName;
    }
    return newName.isEmpty() ? "unnamed" : newName;
}

void MainWindow::onGeneratePreview()
{
    RenameRules rules = collectRules(); // 使用局部变量
    QFileInfoList files = getSelectedFiles();
    if(files.isEmpty())
    {
        QMessageBox::warning(this, tr("警告"), tr("请先选择文件或文件夹"));
        return;
    }
    QList<PreviewItem> previewItems;
    int counter = rules.sequenceStart;
    QSet<QString> usedNames;
    for(const QFileInfo &fi : files)
    {
        PreviewItem item;
        item.original = fi.fileName();
        try
        {
            item.newName = applyRenameRules(fi.fileName(), counter);
            item.valid = !item.newName.isEmpty() && (item.newName != item.original);
            // 检查重复名称
            if(usedNames.contains(item.newName))
            {
                item.valid = false;
                item.error = tr("文件名重复");
            }
            else
            {
                usedNames.insert(item.newName);
            }
            // 检查目标文件是否存在
            QString newPath = fi.absolutePath() + QDir::separator() + item.newName;
            if(QFile::exists(newPath) && newPath != fi.absoluteFilePath())
            {
                item.valid = false;
                item.error = tr("文件已存在");
            }
        }
        catch(const std::exception &e)
        {
            item.valid = false;
            item.error = tr("处理错误: %1").arg(e.what());
        }
        previewItems.append(item);
    }
    previewModel->updateData(previewItems);
    statusBar()->showMessage(tr("生成预览完成，共%1项").arg(previewItems.count()), 3000);
}

void MainWindow::onExecuteRename()
{
    const QList<PreviewItem>& items = previewModel->getItems();
    if(items.isEmpty())
    {
        QMessageBox::warning(this, tr("警告"), tr("没有可执行的重命名项"));
        return;
    }
    // 验证所有名称
    for(const auto &item : items)
    {
        if(!item.valid)
        {
            QMessageBox::critical(this, tr("错误"),
                                  tr("存在无效的重命名项: %1 → %2\n错误: %3")
                                  .arg(item.original).arg(item.newName).arg(item.error));
            return;
        }
    }
    // 执行重命名
    int successCount = 0;
    QModelIndex rootIndex = ui->treeView->rootIndex();
    QString currentPath = fsModel->filePath(rootIndex);
    for(const auto &item : items)
    {
        QString oldPath = currentPath + QDir::separator() + item.original;
        QString newPath = currentPath + QDir::separator() + item.newName;
        if(QFile::rename(oldPath, newPath))
        {
            successCount++;
            logOperation(tr("成功: %1 → %2").arg(item.original).arg(item.newName));
        }
        else
        {
            logOperation(tr("失败: %1 → %2").arg(item.original).arg(item.newName));
        }
    }
    // 刷新视图
    fsModel->setRootPath(fsModel->rootPath());
    statusBar()->showMessage(tr("操作完成，成功%1项").arg(successCount), 5000);
}

void MainWindow::logOperation(const QString &message)
{
    QDateTime now = QDateTime::currentDateTime();
    ui->textEdit_Log->append(tr("[%1] %2").arg(now.toString("hh:mm:ss")).arg(message));
}
