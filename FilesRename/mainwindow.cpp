#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , fsModel(new QFileSystemModel(this))
    , previewModel(new PreviewModel(this))
    , recentFoldersMenu(new QMenu(this))
{
    ui->setupUi(this);
    // 初始化文件系统模型
    fsModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    // 设置文件代理
    ui->treeView_Files->setItemDelegate(new FileDelegate(this));  // 确保使用正确的视图名称
    // 初始化文件树
    setupFileTree();
    loadFileTreeSettings();
    initRecentFoldersMenu();
    // 连接信号槽
    connect(ui->treeView_Files->header(), &QHeaderView::customContextMenuRequested,
            this, &MainWindow::onFileTreeHeaderCustomContextMenuRequested);
    connect(ui->treeView_Files->header(), &QHeaderView::sectionClicked,
            this, &MainWindow::onFileTreeHeaderClicked);
    connect(ui->treeView_Files, &QTreeView::customContextMenuRequested,
            this, &MainWindow::onFileTreeCustomContextMenuRequested);
    connect(ui->pushButton_Preview, &QPushButton::clicked,
            this, &MainWindow::onGeneratePreview);
    // 默认打开用户目录
    setCurrentFolder(QDir::homePath());
    //初始化预览表格
    ui->previewTable->setModel(previewModel);
    // 设置表格列宽
    ui->previewTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->previewTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->previewTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
}

MainWindow::~MainWindow()
{
    // 保存最近文件夹
    QSettings settings;
    settings.setValue("recentFolders", recentFolders);
    saveFileTreeSettings();
    delete ui;
}

// 初始化文件树设置
void MainWindow::setupFileTree()
{
    // 确保模型已创建
    if(!fsModel)
    {
        fsModel = new QFileSystemModel(this);
    }
    // 设置根路径
    fsModel->setRootPath("");
    ui->treeView_Files->setModel(fsModel);
    // 设置列名
    fsModel->setHeaderData(COLUMN_NAME, Qt::Horizontal, tr("名称"));
    fsModel->setHeaderData(COLUMN_TYPE, Qt::Horizontal, tr("类型"));
    fsModel->setHeaderData(COLUMN_SIZE, Qt::Horizontal, tr("大小"));
    fsModel->setHeaderData(COLUMN_DATE, Qt::Horizontal, tr("修改日期"));
    // 设置初始列宽
    for(int i = 0; i < COLUMN_COUNT; ++i)
    {
        ui->treeView_Files->setColumnHidden(i, !m_fileTreeSettings.columnsVisible[i]);
        if(!ui->treeView_Files->isColumnHidden(i))
        {
            ui->treeView_Files->setColumnWidth(i, m_fileTreeSettings.columnWidths[i]);
        }
    }
    // 设置文件名列自动扩展
    ui->treeView_Files->header()->setSectionResizeMode(COLUMN_NAME, QHeaderView::Interactive);
    ui->treeView_Files->header()->setStretchLastSection(false);
    // 启用排序
    ui->treeView_Files->setSortingEnabled(true);
    ui->treeView_Files->sortByColumn(m_fileTreeSettings.sortColumn, m_fileTreeSettings.sortOrder);
}

// 保存文件树设置
void MainWindow::saveFileTreeSettings()
{
    QSettings settings;
    settings.beginGroup("FileTree");
    // 保存列可见性
    for(int i = 0; i < COLUMN_COUNT; ++i)
    {
        settings.setValue(QString("ColumnVisible%1").arg(i),
                          ui->treeView_Files->isColumnHidden(i));
    }
    // 保存列宽度
    for(int i = 0; i < COLUMN_COUNT; ++i)
    {
        settings.setValue(QString("ColumnWidth%1").arg(i),
                          ui->treeView_Files->columnWidth(i));
    }
    // 保存排序设置
    settings.setValue("SortColumn", ui->treeView_Files->header()->sortIndicatorSection());
    settings.setValue("SortOrder", ui->treeView_Files->header()->sortIndicatorOrder());
    settings.endGroup();
}

// 加载文件树设置
void MainWindow::loadFileTreeSettings()
{
    QSettings settings;
    settings.beginGroup("FileTree");
    // 加载列可见性
    for(int i = 0; i < COLUMN_COUNT; ++i)
    {
        m_fileTreeSettings.columnsVisible[i] = settings.value(
                QString("ColumnVisible%1").arg(i), true).toBool();
    }
    // 加载列宽度
    for(int i = 0; i < COLUMN_COUNT; ++i)
    {
        m_fileTreeSettings.columnWidths[i] = settings.value(
                QString("ColumnWidth%1").arg(i),
                i == COLUMN_NAME ? 250 : (i == COLUMN_DATE ? 120 : 80)).toInt();
    }
    // 加载排序设置
    m_fileTreeSettings.sortColumn = settings.value("SortColumn", COLUMN_NAME).toInt();
    m_fileTreeSettings.sortOrder = static_cast<Qt::SortOrder>(
                                       settings.value("SortOrder", Qt::AscendingOrder).toInt());
    settings.endGroup();
}

// 更新文件树列显示
void MainWindow::updateFileTreeColumns()
{
    for(int i = 0; i < COLUMN_COUNT; ++i)
    {
        ui->treeView_Files->setColumnHidden(i, !m_fileTreeSettings.columnsVisible[i]);
        if(!ui->treeView_Files->isColumnHidden(i))
        {
            ui->treeView_Files->setColumnWidth(i, m_fileTreeSettings.columnWidths[i]);
        }
    }
}

// 文件树表头点击事件
void MainWindow::onFileTreeHeaderClicked(int logicalIndex)
{
    // 更新排序设置
    m_fileTreeSettings.sortColumn = logicalIndex;
    if(ui->treeView_Files->header()->sortIndicatorOrder() == Qt::AscendingOrder)
    {
        m_fileTreeSettings.sortOrder = Qt::DescendingOrder;
    }
    else
    {
        m_fileTreeSettings.sortOrder = Qt::AscendingOrder;
    }
    // 应用排序
    ui->treeView_Files->sortByColumn(logicalIndex, m_fileTreeSettings.sortOrder);
    saveFileTreeSettings();
}

// 文件树表头右键菜单
void MainWindow::onFileTreeHeaderCustomContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);
    menu.setToolTipsVisible(true);
    // 添加列显示选项
    for(int i = 0; i < COLUMN_COUNT; ++i)
    {
        QString columnName = fsModel->headerData(i, Qt::Horizontal).toString();
        QAction *action = menu.addAction(columnName, [this, i]()
        {
            m_fileTreeSettings.columnsVisible[i] = !m_fileTreeSettings.columnsVisible[i];
            updateFileTreeColumns();
            saveFileTreeSettings();
        });
        action->setCheckable(true);
        action->setChecked(!ui->treeView_Files->isColumnHidden(i));
        // 设置工具提示
        switch(i)
        {
            case COLUMN_NAME:
                action->setToolTip(tr("显示/隐藏文件名列"));
                break;
            case COLUMN_TYPE:
                action->setToolTip(tr("显示/隐藏文件类型列"));
                break;
            case COLUMN_SIZE:
                action->setToolTip(tr("显示/隐藏文件大小列"));
                break;
            case COLUMN_DATE:
                action->setToolTip(tr("显示/隐藏修改日期列"));
                break;
        }
    }
    menu.addSeparator();
    // 添加重置列宽选项
    QAction *resetAction = menu.addAction(tr("重置列宽"), [this]()
    {
        for(int i = 0; i < COLUMN_COUNT; ++i)
        {
            m_fileTreeSettings.columnWidths[i] =
                (i == COLUMN_NAME ? 250 : (i == COLUMN_DATE ? 120 : 80));
        }
        updateFileTreeColumns();
        saveFileTreeSettings();
    });
    resetAction->setToolTip(tr("恢复默认列宽设置"));
    menu.exec(ui->treeView_Files->header()->mapToGlobal(pos));
}

// 文件项右键菜单
void MainWindow::onFileTreeCustomContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = ui->treeView_Files->indexAt(pos);
    if(!index.isValid())
    {
        return;
    }
    QString filePath = fsModel->filePath(index);
    QFileInfo fileInfo(filePath);
    QMenu menu(this);
    // 添加常用操作
    menu.addAction(tr("打开"), [filePath]()
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
    });
    menu.addAction(tr("在资源管理器中显示"), [filePath]()
    {
        QProcess::startDetached("explorer", {"/select,", QDir::toNativeSeparators(filePath)});
    });
    menu.addAction(tr("复制完整路径"), [filePath]()
    {
        QApplication::clipboard()->setText(QDir::toNativeSeparators(filePath));
    });
    menu.addSeparator();
    // 添加重命名操作
    menu.addAction(tr("重命名"), [this, index]()
    {
        ui->treeView_Files->edit(index);
    });
    menu.exec(ui->treeView_Files->mapToGlobal(pos));
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
    // 设置根路径并获取索引
    fsModel->setRootPath(path);
    QModelIndex index = fsModel->index(path);
    // 检查索引是否有效
    if(!index.isValid())
    {
        qWarning() << "Invalid folder path:" << path;
        return;
    }
    ui->treeView_Files->setRootIndex(index);
    ui->treeView_Files->expand(index);
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
    if(!recentFoldersMenu)
    {
        return;
    }
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
    // 添加安全检查
    if(recentFoldersMenu && !recentFolders.isEmpty())
    {
        menu.addMenu(recentFoldersMenu);
    }
    menu.addSeparator();
    menu.addAction(QIcon(":/icons/icons/refresh.png"), tr("刷新"), [this]()
    {
        if(fsModel)
        {
            fsModel->setRootPath(fsModel->rootPath());
        }
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
            QDir currentDir(fsModel->filePath(ui->treeView_Files->rootIndex()));
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
    menu.exec(ui->treeView_Files->viewport()->mapToGlobal(pos));
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
    QModelIndexList indexes = ui->treeView_Files->selectionModel()->selectedRows();
    for(const QModelIndex &index : indexes)
    {
        QString path = fsModel->filePath(index);
        QFileInfo fileInfo(path);
        if(fileInfo.isFile())    // 只处理文件，不处理目录
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
    // 后缀修改规则
    rules.extensionEnabled = ui->checkBox_ExtensionModify->isChecked();
    rules.extensionAction = static_cast<RenameRules::ExtensionAction>(ui->comboBox_ExtensionAction->currentIndex());
    rules.extensionValue = ui->lineEdit_ExtensionValue->text().trimmed();
    return rules;
}

//处理文件后缀修改
QString MainWindow::processExtension(const QString& original, const RenameRules& rules) const
{
    if(!rules.extensionEnabled)
    {
        return original;
    }
    QFileInfo fi(original);
    QString baseName = fi.completeBaseName();
    QString extension = fi.suffix();
    QString newName = baseName;
    switch(rules.extensionAction)
    {
        case RenameRules::ExtensionAction_Add:
            if(!extension.isEmpty())
            {
                newName += "." + extension;
            }
            if(!rules.extensionValue.isEmpty())
            {
                newName += "." + rules.extensionValue;
            }
            break;
        case RenameRules::ExtensionAction_Replace:
            if(!rules.extensionValue.isEmpty())
            {
                newName += "." + rules.extensionValue;
            }
            break;
        case RenameRules::ExtensionAction_Remove:
            break;
    }
    return newName;
}

// 新增方法：验证后缀修改规则
bool MainWindow::validateExtensionRules(const RenameRules& rules, QString& error) const
{
    if(!rules.extensionEnabled)
    {
        return true; // 未启用后缀修改，无需验证
    }
    // 检查新后缀是否包含非法字符
    static QRegularExpression illegalChars(R"([\/:*?"<>|])");
    if(rules.extensionValue.contains(illegalChars))
    {
        error = tr("后缀包含非法字符");
        return false;
    }
    // 检查保留文件名
    static QStringList reservedNames = {"CON", "PRN", "AUX", "NUL", "COM1", "LPT1"};
    if(reservedNames.contains(rules.extensionValue.toUpper()))
    {
        error = tr("后缀是系统保留名称");
        return false;
    }
    // 对于替换操作，必须提供新后缀
    if(rules.extensionAction == RenameRules::ExtensionAction_Replace && rules.extensionValue.isEmpty())
    {
        error = tr("替换后缀操作需要指定新后缀");
        return false;
    }
    return true;
}

void MainWindow::initRecentFoldersMenu()
{
    // 从设置加载最近文件夹列表
    QSettings settings;
    recentFolders = settings.value("recentFolders").toStringList();
    // 设置菜单标题和图标
    recentFoldersMenu->setTitle(tr("最近文件夹"));
    recentFoldersMenu->setIcon(QIcon(":/icons/icons/folder.png"));
    // 更新菜单内容
    updateRecentFoldersMenu();
}


QString MainWindow::applyRenameRules(const QString &original, int& counter) const
{
    RenameRules rules = collectRules();
    QFileInfo fi(original);
    QString baseName = fi.completeBaseName();
    QString extension = fi.suffix();
    QString newName = baseName;
    // 处理序号规则
    if(rules.sequenceEnabled)
    {
        QString sequenceStr = QString("%1").arg(counter, rules.minDigits, 10, QChar('0'));
        counter += rules.sequenceStep;
        switch(rules.sequencePos)
        {
            case RenameRules::SequencePosition_Prefix:
                newName = sequenceStr + "_" + newName;  // 前缀添加下划线
                break;
            case RenameRules::SequencePosition_Suffix:
                newName = newName + "_" + sequenceStr;  // 后缀添加下划线
                break;
            case RenameRules::SequencePosition_Replace:
                newName = sequenceStr;  // 完全替换原名
                break;
        }
    }
    // 处理替换规则
    if(rules.replaceEnabled && !rules.replaceFrom.isEmpty())
    {
        if(rules.useRegex)
        {
            QRegularExpression regex(rules.replaceFrom);
            newName.replace(regex, rules.replaceTo);
        }
        else
        {
            newName.replace(rules.replaceFrom, rules.replaceTo);
        }
    }
    // 处理大小写规则
    if(rules.caseEnabled)
    {
        switch(rules.caseType)
        {
            case RenameRules::CaseType_Upper:
                newName = newName.toUpper();
                break;
            case RenameRules::CaseType_Lower:
                newName = newName.toLower();
                break;
            case RenameRules::CaseType_Title:
                if(!newName.isEmpty())
                {
                    newName = newName[0].toUpper() + newName.mid(1).toLower();
                }
                break;
        }
    }
    // 处理扩展名规则
    if(rules.keepExtension && !extension.isEmpty())
    {
        newName += "." + extension;
    }
    // 处理后缀修改规则
    if(rules.extensionEnabled)
    {
        newName = processExtension(newName, rules);
    }
    return newName;
}

void MainWindow::onGeneratePreview()
{
    // 清除旧数据
    previewModel->clear();
    // 获取选中的文件
    QFileInfoList files = getSelectedFiles();
    if(files.isEmpty())
    {
        QMessageBox::information(this, tr("提示"), tr("没有选中任何文件"));
        return;
    }
    // 收集当前规则
    RenameRules rules = collectRules();
    // 验证规则
    QString error;
    if(!validateExtensionRules(rules, error))
    {
        QMessageBox::warning(this, tr("错误"), error);
        return;
    }
    // 生成预览项
    QList<PreviewItem> previewItems;
    int counter = rules.sequenceStart;
    for(const QFileInfo &fileInfo : files)
    {
        QString original = fileInfo.fileName();
        PreviewItem item;
        item.original = original;
        try
        {
            item.newName = applyRenameRules(original, counter);
            item.valid = true;
            // 验证新文件名是否合法
            if(item.newName.isEmpty() || item.newName.contains(QRegularExpression(R"([\/:*?"<>|])")))
            {
                item.valid = false;
                item.error = tr("生成的文件名包含非法字符");
            }
            // 检查文件名是否重复
            for(const PreviewItem &existing : previewItems)
            {
                if(existing.newName == item.newName)
                {
                    item.valid = false;
                    item.error = tr("文件名重复");
                    break;
                }
            }
        }
        catch(const std::exception &e)
        {
            item.valid = false;
            item.error = tr("规则应用错误: %1").arg(e.what());
        }
        previewItems.append(item);
        // 按步长递增计数器
        if(rules.sequenceEnabled)
        {
            counter += rules.sequenceStep;
        }
    }
    // 更新模型
    previewModel->updateData(previewItems);
    ui->tabWidget->setCurrentIndex(1); // 切换到预览标签页
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
    QModelIndex rootIndex = ui->treeView_Files->rootIndex();
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
