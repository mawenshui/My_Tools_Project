/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.12.8
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionRefresh;
    QAction *actionExit;
    QAction *actionAbout;
    QAction *actionShowAllDrives;
    QAction *actionHideEmptyDrives;
    QAction *actionViewHistory;
    QAction *actionExportCSV;
    QAction *actionExportExcel;
    QAction *actionSettings;
    QAction *actionAutoStart;
    QAction *actionMinimizeToTray;
    QAction *actionStartMinimized;
    QAction *actionCloseToTray;
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout_main;
    QToolBar *toolBar;
    QSplitter *mainSplitter;
    QGroupBox *driveGroupBox;
    QVBoxLayout *driveVerticalLayout;
    QScrollArea *driveScrollArea;
    QWidget *driveScrollAreaWidgetContents;
    QGroupBox *processGroupBox;
    QVBoxLayout *processVerticalLayout;
    QLabel *processInfoLabel;
    QTreeWidget *processTreeWidget;
    QMenuBar *menubar;
    QMenu *menuFile;
    QMenu *menuView;
    QMenu *menuData;
    QMenu *menuSettings;
    QMenu *menuHelp;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(1200, 800);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/disk.png"), QSize(), QIcon::Normal, QIcon::Off);
        MainWindow->setWindowIcon(icon);
        MainWindow->setStyleSheet(QString::fromUtf8("QMainWindow {\n"
"    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,\n"
"                                stop: 0 #f8f9fa, stop: 1 #e9ecef);\n"
"    color: #212529;\n"
"}\n"
"\n"
"QGroupBox {\n"
"    font-weight: 600;\n"
"    border: 2px solid #dee2e6;\n"
"    border-radius: 12px;\n"
"    margin-top: 1ex;\n"
"    padding-top: 12px;\n"
"    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,\n"
"                                stop: 0 rgba(255, 255, 255, 0.95), \n"
"                                stop: 1 rgba(248, 249, 250, 0.9));\n"
"    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);\n"
"}\n"
"\n"
"QGroupBox::title {\n"
"    subcontrol-origin: margin;\n"
"    left: 15px;\n"
"    padding: 0 8px 0 8px;\n"
"    color: #495057;\n"
"    font-size: 11pt;\n"
"    font-weight: 600;\n"
"}\n"
"\n"
"QTreeWidget {\n"
"    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,\n"
"                                stop: 0 rgba(255, 255, 255, 0.98), \n"
"                                stop: 1 rgba(248, 249, 250, 0.9"
                        "5));\n"
"    border: 1px solid #dee2e6;\n"
"    border-radius: 8px;\n"
"    selection-background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,\n"
"                                                stop: 0 #007bff, stop: 1 #0056b3);\n"
"    alternate-background-color: rgba(248, 249, 250, 0.6);\n"
"    gridline-color: #e9ecef;\n"
"    font-size: 9pt;\n"
"    outline: none;\n"
"}\n"
"\n"
"QTreeWidget::item {\n"
"    padding: 6px 4px;\n"
"    border-bottom: 1px solid rgba(233, 236, 239, 0.5);\n"
"    min-height: 24px;\n"
"}\n"
"\n"
"QTreeWidget::item:selected {\n"
"    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,\n"
"                                stop: 0 #007bff, stop: 1 #0056b3);\n"
"    color: white;\n"
"    border-radius: 4px;\n"
"}\n"
"\n"
"QTreeWidget::item:hover {\n"
"    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,\n"
"                                stop: 0 rgba(0, 123, 255, 0.1), \n"
"                                stop: 1 rgba(0, 86, 179, 0.1));\n"
"    border-radius: 4px;\n"
""
                        "}\n"
"\n"
"QTreeWidget::header {\n"
"    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,\n"
"                                stop: 0 #f8f9fa, stop: 1 #e9ecef);\n"
"    border: none;\n"
"    border-bottom: 2px solid #dee2e6;\n"
"    font-weight: 600;\n"
"    color: #495057;\n"
"}\n"
"\n"
"QTreeWidget::header::section {\n"
"    background: transparent;\n"
"    padding: 8px 4px;\n"
"    border: none;\n"
"    border-right: 1px solid #dee2e6;\n"
"}\n"
"\n"
"QTreeWidget::header::section:hover {\n"
"    background: rgba(0, 123, 255, 0.1);\n"
"}\n"
"\n"
"QScrollArea {\n"
"    border: 1px solid #dee2e6;\n"
"    border-radius: 8px;\n"
"    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,\n"
"                                stop: 0 rgba(255, 255, 255, 0.95), \n"
"                                stop: 1 rgba(248, 249, 250, 0.9));\n"
"}\n"
"\n"
"QScrollBar:vertical {\n"
"    background: rgba(233, 236, 239, 0.5);\n"
"    width: 12px;\n"
"    border-radius: 6px;\n"
"    margin: 0;\n"
"}\n"
"\n"
"QScrollBar::ha"
                        "ndle:vertical {\n"
"    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,\n"
"                                stop: 0 #adb5bd, stop: 1 #6c757d);\n"
"    border-radius: 6px;\n"
"    min-height: 20px;\n"
"    margin: 2px;\n"
"}\n"
"\n"
"QScrollBar::handle:vertical:hover {\n"
"    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,\n"
"                                stop: 0 #6c757d, stop: 1 #495057);\n"
"}\n"
"\n"
"QComboBox {\n"
"    border: 2px solid #dee2e6;\n"
"    border-radius: 8px;\n"
"    padding: 6px 12px;\n"
"    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,\n"
"                                stop: 0 white, stop: 1 #f8f9fa);\n"
"    min-width: 100px;\n"
"    font-weight: 500;\n"
"    color: #495057;\n"
"}\n"
"\n"
"QComboBox:hover {\n"
"    border-color: #007bff;\n"
"    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,\n"
"                                stop: 0 white, stop: 1 rgba(0, 123, 255, 0.05));\n"
"}\n"
"\n"
"QComboBox:focus {\n"
"    border-color: #007bff;\n"
"    ou"
                        "tline: none;\n"
"}\n"
"\n"
"QComboBox::drop-down {\n"
"    subcontrol-origin: padding;\n"
"    subcontrol-position: top right;\n"
"    width: 25px;\n"
"    border-left-width: 1px;\n"
"    border-left-color: #dee2e6;\n"
"    border-left-style: solid;\n"
"    border-top-right-radius: 8px;\n"
"    border-bottom-right-radius: 8px;\n"
"    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,\n"
"                                stop: 0 #f8f9fa, stop: 1 #e9ecef);\n"
"}\n"
"\n"
"QComboBox::drop-down:hover {\n"
"    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,\n"
"                                stop: 0 rgba(0, 123, 255, 0.1), \n"
"                                stop: 1 rgba(0, 123, 255, 0.05));\n"
"}\n"
"\n"
"QComboBox::down-arrow {\n"
"    width: 0;\n"
"    height: 0;\n"
"    border-left: 5px solid transparent;\n"
"    border-right: 5px solid transparent;\n"
"    border-top: 6px solid #6c757d;\n"
"}\n"
"\n"
"QComboBox::down-arrow:hover {\n"
"    border-top-color: #007bff;\n"
"}\n"
"\n"
"QPushButton {\n"
""
                        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,\n"
"                                stop: 0 #007bff, stop: 1 #0056b3);\n"
"    border: none;\n"
"    color: white;\n"
"    padding: 10px 20px;\n"
"    border-radius: 8px;\n"
"    font-weight: 600;\n"
"    min-width: 100px;\n"
"    font-size: 9pt;\n"
"}\n"
"\n"
"QPushButton:hover {\n"
"    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,\n"
"                                stop: 0 #0056b3, stop: 1 #004085);\n"
"    transform: translateY(-1px);\n"
"}\n"
"\n"
"QPushButton:pressed {\n"
"    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,\n"
"                                stop: 0 #004085, stop: 1 #002752);\n"
"    transform: translateY(1px);\n"
"}\n"
"\n"
"QLabel {\n"
"    color: #495057;\n"
"    font-size: 9pt;\n"
"    font-weight: 500;\n"
"}\n"
"\n"
"QToolBar {\n"
"    border: none;\n"
"    border-bottom: 1px solid #dee2e6;\n"
"    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,\n"
"                                stop: 0 #ffffff"
                        ", stop: 1 #f8f9fa);\n"
"    spacing: 8px;\n"
"    padding: 8px;\n"
"    font-weight: 500;\n"
"}\n"
"\n"
"QToolBar::separator {\n"
"    background: #dee2e6;\n"
"    width: 1px;\n"
"    margin: 4px 2px;\n"
"}\n"
"\n"
"QStatusBar {\n"
"    border-top: 1px solid #dee2e6;\n"
"    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,\n"
"                                stop: 0 #f8f9fa, stop: 1 #e9ecef);\n"
"    color: #495057;\n"
"    font-weight: 500;\n"
"    padding: 4px 8px;\n"
"}\n"
"\n"
"QMenuBar {\n"
"    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,\n"
"                                stop: 0 #ffffff, stop: 1 #f8f9fa);\n"
"    border-bottom: 1px solid #dee2e6;\n"
"    color: #495057;\n"
"    font-weight: 500;\n"
"}\n"
"\n"
"QMenuBar::item {\n"
"    background: transparent;\n"
"    padding: 6px 12px;\n"
"    border-radius: 4px;\n"
"    margin: 2px;\n"
"}\n"
"\n"
"QMenuBar::item:selected {\n"
"    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,\n"
"                                stop: 0 rgb"
                        "a(0, 123, 255, 0.1), \n"
"                                stop: 1 rgba(0, 123, 255, 0.05));\n"
"    color: #007bff;\n"
"}\n"
"\n"
"QMenuBar::item:pressed {\n"
"    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,\n"
"                                stop: 0 rgba(0, 123, 255, 0.2), \n"
"                                stop: 1 rgba(0, 123, 255, 0.1));\n"
"}\n"
"\n"
"QMenu {\n"
"    background: white;\n"
"    border: 1px solid #dee2e6;\n"
"    border-radius: 8px;\n"
"    padding: 4px;\n"
"    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);\n"
"}\n"
"\n"
"QMenu::item {\n"
"    background: transparent;\n"
"    padding: 8px 16px;\n"
"    border-radius: 4px;\n"
"    margin: 1px;\n"
"    color: #495057;\n"
"}\n"
"\n"
"QMenu::item:selected {\n"
"    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,\n"
"                                stop: 0 #007bff, stop: 1 #0056b3);\n"
"    color: white;\n"
"}\n"
"\n"
"QMenu::separator {\n"
"    height: 1px;\n"
"    background: #dee2e6;\n"
"    margin: 4px 8px;\n"
"}\n"
"\n"
""
                        "QSplitter::handle {\n"
"    background: #dee2e6;\n"
"    border-radius: 2px;\n"
"}\n"
"\n"
"QSplitter::handle:horizontal {\n"
"    width: 4px;\n"
"    margin: 2px 0;\n"
"}\n"
"\n"
"QSplitter::handle:vertical {\n"
"    height: 4px;\n"
"    margin: 0 2px;\n"
"}\n"
"\n"
"QSplitter::handle:hover {\n"
"    background: #007bff;\n"
"}"));
        actionRefresh = new QAction(MainWindow);
        actionRefresh->setObjectName(QString::fromUtf8("actionRefresh"));
        actionExit = new QAction(MainWindow);
        actionExit->setObjectName(QString::fromUtf8("actionExit"));
        actionAbout = new QAction(MainWindow);
        actionAbout->setObjectName(QString::fromUtf8("actionAbout"));
        actionShowAllDrives = new QAction(MainWindow);
        actionShowAllDrives->setObjectName(QString::fromUtf8("actionShowAllDrives"));
        actionShowAllDrives->setCheckable(true);
        actionShowAllDrives->setChecked(true);
        actionHideEmptyDrives = new QAction(MainWindow);
        actionHideEmptyDrives->setObjectName(QString::fromUtf8("actionHideEmptyDrives"));
        actionHideEmptyDrives->setCheckable(true);
        actionViewHistory = new QAction(MainWindow);
        actionViewHistory->setObjectName(QString::fromUtf8("actionViewHistory"));
        actionExportCSV = new QAction(MainWindow);
        actionExportCSV->setObjectName(QString::fromUtf8("actionExportCSV"));
        actionExportExcel = new QAction(MainWindow);
        actionExportExcel->setObjectName(QString::fromUtf8("actionExportExcel"));
        actionSettings = new QAction(MainWindow);
        actionSettings->setObjectName(QString::fromUtf8("actionSettings"));
        actionAutoStart = new QAction(MainWindow);
        actionAutoStart->setObjectName(QString::fromUtf8("actionAutoStart"));
        actionAutoStart->setCheckable(true);
        actionMinimizeToTray = new QAction(MainWindow);
        actionMinimizeToTray->setObjectName(QString::fromUtf8("actionMinimizeToTray"));
        actionMinimizeToTray->setCheckable(true);
        actionStartMinimized = new QAction(MainWindow);
        actionStartMinimized->setObjectName(QString::fromUtf8("actionStartMinimized"));
        actionStartMinimized->setCheckable(true);
        actionCloseToTray = new QAction(MainWindow);
        actionCloseToTray->setObjectName(QString::fromUtf8("actionCloseToTray"));
        actionCloseToTray->setCheckable(true);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        verticalLayout_main = new QVBoxLayout(centralwidget);
        verticalLayout_main->setSpacing(6);
        verticalLayout_main->setObjectName(QString::fromUtf8("verticalLayout_main"));
        verticalLayout_main->setContentsMargins(8, 8, 8, 8);
        toolBar = new QToolBar(centralwidget);
        toolBar->setObjectName(QString::fromUtf8("toolBar"));
        toolBar->setMovable(false);
        toolBar->setAllowedAreas(Qt::TopToolBarArea);
        toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        toolBar->setFloatable(false);

        verticalLayout_main->addWidget(toolBar);

        mainSplitter = new QSplitter(centralwidget);
        mainSplitter->setObjectName(QString::fromUtf8("mainSplitter"));
        mainSplitter->setOrientation(Qt::Horizontal);
        mainSplitter->setHandleWidth(6);
        driveGroupBox = new QGroupBox(mainSplitter);
        driveGroupBox->setObjectName(QString::fromUtf8("driveGroupBox"));
        driveGroupBox->setMinimumSize(QSize(400, 0));
        driveVerticalLayout = new QVBoxLayout(driveGroupBox);
        driveVerticalLayout->setSpacing(6);
        driveVerticalLayout->setObjectName(QString::fromUtf8("driveVerticalLayout"));
        driveVerticalLayout->setContentsMargins(8, 8, 8, 8);
        driveScrollArea = new QScrollArea(driveGroupBox);
        driveScrollArea->setObjectName(QString::fromUtf8("driveScrollArea"));
        driveScrollArea->setWidgetResizable(true);
        driveScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        driveScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        driveScrollAreaWidgetContents = new QWidget();
        driveScrollAreaWidgetContents->setObjectName(QString::fromUtf8("driveScrollAreaWidgetContents"));
        driveScrollAreaWidgetContents->setGeometry(QRect(0, 0, 382, 69));
        driveScrollArea->setWidget(driveScrollAreaWidgetContents);

        driveVerticalLayout->addWidget(driveScrollArea);

        mainSplitter->addWidget(driveGroupBox);
        processGroupBox = new QGroupBox(mainSplitter);
        processGroupBox->setObjectName(QString::fromUtf8("processGroupBox"));
        processGroupBox->setMinimumSize(QSize(500, 0));
        processVerticalLayout = new QVBoxLayout(processGroupBox);
        processVerticalLayout->setSpacing(6);
        processVerticalLayout->setObjectName(QString::fromUtf8("processVerticalLayout"));
        processVerticalLayout->setContentsMargins(8, 8, 8, 8);
        processInfoLabel = new QLabel(processGroupBox);
        processInfoLabel->setObjectName(QString::fromUtf8("processInfoLabel"));
        processInfoLabel->setAlignment(Qt::AlignCenter);
        processInfoLabel->setStyleSheet(QString::fromUtf8("color: #666666; font-style: italic;"));

        processVerticalLayout->addWidget(processInfoLabel);

        processTreeWidget = new QTreeWidget(processGroupBox);
        processTreeWidget->setObjectName(QString::fromUtf8("processTreeWidget"));
        processTreeWidget->setAlternatingRowColors(true);
        processTreeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        processTreeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        processTreeWidget->setSortingEnabled(true);
        processTreeWidget->setColumnCount(4);
        processTreeWidget->header()->setVisible(true);
        processTreeWidget->header()->setStretchLastSection(false);

        processVerticalLayout->addWidget(processTreeWidget);

        mainSplitter->addWidget(processGroupBox);

        verticalLayout_main->addWidget(mainSplitter);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 1200, 22));
        menuFile = new QMenu(menubar);
        menuFile->setObjectName(QString::fromUtf8("menuFile"));
        menuView = new QMenu(menubar);
        menuView->setObjectName(QString::fromUtf8("menuView"));
        menuData = new QMenu(menubar);
        menuData->setObjectName(QString::fromUtf8("menuData"));
        menuSettings = new QMenu(menubar);
        menuSettings->setObjectName(QString::fromUtf8("menuSettings"));
        menuHelp = new QMenu(menubar);
        menuHelp->setObjectName(QString::fromUtf8("menuHelp"));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        statusbar->setSizeGripEnabled(true);
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menuFile->menuAction());
        menubar->addAction(menuView->menuAction());
        menubar->addAction(menuData->menuAction());
        menubar->addAction(menuSettings->menuAction());
        menubar->addAction(menuHelp->menuAction());
        menuFile->addAction(actionRefresh);
        menuFile->addSeparator();
        menuFile->addAction(actionExit);
        menuView->addAction(actionShowAllDrives);
        menuView->addAction(actionHideEmptyDrives);
        menuData->addAction(actionViewHistory);
        menuData->addSeparator();
        menuData->addAction(actionExportCSV);
        menuData->addAction(actionExportExcel);
        menuSettings->addAction(actionSettings);
        menuSettings->addSeparator();
        menuSettings->addAction(actionAutoStart);
        menuSettings->addAction(actionMinimizeToTray);
        menuSettings->addAction(actionStartMinimized);
        menuSettings->addAction(actionCloseToTray);
        menuHelp->addAction(actionAbout);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "\347\243\201\347\233\230\347\233\221\346\216\247\345\231\250 - Disk Monitor", nullptr));
        actionRefresh->setText(QApplication::translate("MainWindow", "\345\210\267\346\226\260(&R)", nullptr));
#ifndef QT_NO_SHORTCUT
        actionRefresh->setShortcut(QApplication::translate("MainWindow", "F5", nullptr));
#endif // QT_NO_SHORTCUT
        actionExit->setText(QApplication::translate("MainWindow", "\351\200\200\345\207\272(&X)", nullptr));
#ifndef QT_NO_SHORTCUT
        actionExit->setShortcut(QApplication::translate("MainWindow", "Ctrl+Q", nullptr));
#endif // QT_NO_SHORTCUT
        actionAbout->setText(QApplication::translate("MainWindow", "\345\205\263\344\272\216(&A)", nullptr));
        actionShowAllDrives->setText(QApplication::translate("MainWindow", "\346\230\276\347\244\272\346\211\200\346\234\211\351\251\261\345\212\250\345\231\250", nullptr));
        actionHideEmptyDrives->setText(QApplication::translate("MainWindow", "\351\232\220\350\227\217\347\251\272\351\251\261\345\212\250\345\231\250", nullptr));
        actionViewHistory->setText(QApplication::translate("MainWindow", "\346\237\245\347\234\213\345\216\206\345\217\262\346\225\260\346\215\256(&H)", nullptr));
#ifndef QT_NO_SHORTCUT
        actionViewHistory->setShortcut(QApplication::translate("MainWindow", "Ctrl+H", nullptr));
#endif // QT_NO_SHORTCUT
        actionExportCSV->setText(QApplication::translate("MainWindow", "\345\257\274\345\207\272\344\270\272CSV(&C)", nullptr));
#ifndef QT_NO_SHORTCUT
        actionExportCSV->setShortcut(QApplication::translate("MainWindow", "Ctrl+E", nullptr));
#endif // QT_NO_SHORTCUT
        actionExportExcel->setText(QApplication::translate("MainWindow", "\345\257\274\345\207\272\344\270\272Excel(&X)", nullptr));
#ifndef QT_NO_SHORTCUT
        actionExportExcel->setShortcut(QApplication::translate("MainWindow", "Ctrl+Shift+E", nullptr));
#endif // QT_NO_SHORTCUT
        actionSettings->setText(QApplication::translate("MainWindow", "\350\256\276\347\275\256(&S)", nullptr));
#ifndef QT_NO_SHORTCUT
        actionSettings->setShortcut(QApplication::translate("MainWindow", "Ctrl+,", nullptr));
#endif // QT_NO_SHORTCUT
        actionAutoStart->setText(QApplication::translate("MainWindow", "\345\274\200\346\234\272\350\207\252\345\220\257\345\212\250(&A)", nullptr));
        actionMinimizeToTray->setText(QApplication::translate("MainWindow", "\346\234\200\345\260\217\345\214\226\345\210\260\346\211\230\347\233\230(&T)", nullptr));
        actionStartMinimized->setText(QApplication::translate("MainWindow", "\345\220\257\345\212\250\346\227\266\346\234\200\345\260\217\345\214\226(&M)", nullptr));
        actionCloseToTray->setText(QApplication::translate("MainWindow", "\345\205\263\351\227\255\346\227\266\346\234\200\345\260\217\345\214\226\345\210\260\346\211\230\347\233\230(&C)", nullptr));
        toolBar->setWindowTitle(QApplication::translate("MainWindow", "\345\267\245\345\205\267\346\240\217", nullptr));
        driveGroupBox->setTitle(QApplication::translate("MainWindow", "\347\243\201\347\233\230\347\233\221\346\216\247", nullptr));
        processGroupBox->setTitle(QApplication::translate("MainWindow", "\350\277\233\347\250\213\350\257\246\346\203\205", nullptr));
        processInfoLabel->setText(QApplication::translate("MainWindow", "\351\200\211\346\213\251\344\270\200\344\270\252\347\243\201\347\233\230\346\237\245\347\234\213\347\233\270\345\205\263\350\277\233\347\250\213\344\277\241\346\201\257", nullptr));
        QTreeWidgetItem *___qtreewidgetitem = processTreeWidget->headerItem();
        ___qtreewidgetitem->setText(3, QApplication::translate("MainWindow", "\351\207\215\350\246\201\347\250\213\345\272\246", nullptr));
        ___qtreewidgetitem->setText(2, QApplication::translate("MainWindow", "\347\243\201\347\233\230\344\275\277\347\224\250", nullptr));
        ___qtreewidgetitem->setText(1, QApplication::translate("MainWindow", "PID", nullptr));
        ___qtreewidgetitem->setText(0, QApplication::translate("MainWindow", "\350\277\233\347\250\213\345\220\215\347\247\260", nullptr));
        menuFile->setTitle(QApplication::translate("MainWindow", "\346\226\207\344\273\266(&F)", nullptr));
        menuView->setTitle(QApplication::translate("MainWindow", "\350\247\206\345\233\276(&V)", nullptr));
        menuData->setTitle(QApplication::translate("MainWindow", "\346\225\260\346\215\256(&D)", nullptr));
        menuSettings->setTitle(QApplication::translate("MainWindow", "\350\256\276\347\275\256(&S)", nullptr));
        menuHelp->setTitle(QApplication::translate("MainWindow", "\345\270\256\345\212\251(&H)", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
