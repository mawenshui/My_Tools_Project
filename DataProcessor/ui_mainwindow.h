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
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "faultAlarmWidget/faultalarmwidget.h"
#include "orderSend/ordersendwidget.h"

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *action_2;
    QWidget *centralwidget;
    QGridLayout *gridLayout_3;
    QVBoxLayout *verticalLayout_5;
    QTabWidget *tabWidget_2;
    QWidget *tab;
    QGridLayout *gridLayout_2;
    QVBoxLayout *verticalLayout;
    QTabWidget *tabWidget;
    QWidget *basicTab;
    QGridLayout *gridLayout_9;
    QHBoxLayout *horizontalLayout;
    QLineEdit *dirEdit;
    QPushButton *browseButton;
    QHBoxLayout *horizontalLayout_2;
    QLineEdit *addrEdit;
    QPushButton *addAddrButton;
    QPushButton *removeAddrButton;
    QGroupBox *groupBox_3;
    QGridLayout *gridLayout_8;
    QListWidget *addrList;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label;
    QLineEdit *includeEdit;
    QLabel *label_2;
    QLineEdit *excludeEdit;
    QHBoxLayout *horizontalLayout_4;
    QLabel *label_3;
    QComboBox *orderCombo;
    QCheckBox *uniqueCheck;
    QLabel *label_4;
    QLineEdit *sendIntervalEdit;
    QWidget *logTab;
    QGridLayout *gridLayout;
    QTextEdit *logView;
    QHBoxLayout *horizontalLayout_5;
    QPushButton *startButton;
    QPushButton *pauseButton;
    QPushButton *stopButton;
    QLabel *statusLabel;
    QProgressBar *progressBar;
    QWidget *tab_2;
    QGridLayout *gridLayout_7;
    QGridLayout *gridLayout_6;
    QGroupBox *groupBox;
    QGridLayout *gridLayout_5;
    QVBoxLayout *verticalLayout_6;
    QVBoxLayout *verticalLayout_3;
    QLabel *label_5;
    QLineEdit *lineEdit;
    QTextEdit *textEdit;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout_4;
    QTextEdit *textEdit_2;
    QVBoxLayout *verticalLayout_4;
    QHBoxLayout *horizontalLayout_7;
    QLabel *label_7;
    QLineEdit *addrEdit_2;
    QHBoxLayout *horizontalLayout_6;
    QLabel *label_6;
    QLineEdit *sendIntervalEdit_2;
    QCheckBox *uniqueCheck_2;
    QHBoxLayout *horizontalLayout_8;
    QPushButton *startButton_2;
    QPushButton *pauseButton_2;
    QPushButton *stopButton_2;
    OrderSendWidget *tab_3;
    FaultAlarmWidget *tab_FaultAlarmWidget;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(847, 540);
        action_2 = new QAction(MainWindow);
        action_2->setObjectName(QString::fromUtf8("action_2"));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        gridLayout_3 = new QGridLayout(centralwidget);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        verticalLayout_5 = new QVBoxLayout();
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        tabWidget_2 = new QTabWidget(centralwidget);
        tabWidget_2->setObjectName(QString::fromUtf8("tabWidget_2"));
        tab = new QWidget();
        tab->setObjectName(QString::fromUtf8("tab"));
        gridLayout_2 = new QGridLayout(tab);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        tabWidget = new QTabWidget(tab);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        basicTab = new QWidget();
        basicTab->setObjectName(QString::fromUtf8("basicTab"));
        gridLayout_9 = new QGridLayout(basicTab);
        gridLayout_9->setObjectName(QString::fromUtf8("gridLayout_9"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        dirEdit = new QLineEdit(basicTab);
        dirEdit->setObjectName(QString::fromUtf8("dirEdit"));

        horizontalLayout->addWidget(dirEdit);

        browseButton = new QPushButton(basicTab);
        browseButton->setObjectName(QString::fromUtf8("browseButton"));

        horizontalLayout->addWidget(browseButton);


        gridLayout_9->addLayout(horizontalLayout, 0, 0, 1, 1);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        addrEdit = new QLineEdit(basicTab);
        addrEdit->setObjectName(QString::fromUtf8("addrEdit"));

        horizontalLayout_2->addWidget(addrEdit);

        addAddrButton = new QPushButton(basicTab);
        addAddrButton->setObjectName(QString::fromUtf8("addAddrButton"));

        horizontalLayout_2->addWidget(addAddrButton);

        removeAddrButton = new QPushButton(basicTab);
        removeAddrButton->setObjectName(QString::fromUtf8("removeAddrButton"));

        horizontalLayout_2->addWidget(removeAddrButton);


        gridLayout_9->addLayout(horizontalLayout_2, 1, 0, 1, 1);

        groupBox_3 = new QGroupBox(basicTab);
        groupBox_3->setObjectName(QString::fromUtf8("groupBox_3"));
        gridLayout_8 = new QGridLayout(groupBox_3);
        gridLayout_8->setObjectName(QString::fromUtf8("gridLayout_8"));
        addrList = new QListWidget(groupBox_3);
        addrList->setObjectName(QString::fromUtf8("addrList"));
        addrList->setSelectionMode(QAbstractItemView::ExtendedSelection);

        gridLayout_8->addWidget(addrList, 0, 0, 1, 1);


        gridLayout_9->addWidget(groupBox_3, 2, 0, 1, 1);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        label = new QLabel(basicTab);
        label->setObjectName(QString::fromUtf8("label"));

        horizontalLayout_3->addWidget(label);

        includeEdit = new QLineEdit(basicTab);
        includeEdit->setObjectName(QString::fromUtf8("includeEdit"));

        horizontalLayout_3->addWidget(includeEdit);

        label_2 = new QLabel(basicTab);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        horizontalLayout_3->addWidget(label_2);

        excludeEdit = new QLineEdit(basicTab);
        excludeEdit->setObjectName(QString::fromUtf8("excludeEdit"));

        horizontalLayout_3->addWidget(excludeEdit);


        gridLayout_9->addLayout(horizontalLayout_3, 3, 0, 1, 1);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        label_3 = new QLabel(basicTab);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        horizontalLayout_4->addWidget(label_3);

        orderCombo = new QComboBox(basicTab);
        orderCombo->addItem(QString());
        orderCombo->addItem(QString());
        orderCombo->setObjectName(QString::fromUtf8("orderCombo"));

        horizontalLayout_4->addWidget(orderCombo);

        uniqueCheck = new QCheckBox(basicTab);
        uniqueCheck->setObjectName(QString::fromUtf8("uniqueCheck"));

        horizontalLayout_4->addWidget(uniqueCheck);

        label_4 = new QLabel(basicTab);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        horizontalLayout_4->addWidget(label_4);

        sendIntervalEdit = new QLineEdit(basicTab);
        sendIntervalEdit->setObjectName(QString::fromUtf8("sendIntervalEdit"));

        horizontalLayout_4->addWidget(sendIntervalEdit);


        gridLayout_9->addLayout(horizontalLayout_4, 4, 0, 1, 1);

        tabWidget->addTab(basicTab, QString());
        logTab = new QWidget();
        logTab->setObjectName(QString::fromUtf8("logTab"));
        gridLayout = new QGridLayout(logTab);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        logView = new QTextEdit(logTab);
        logView->setObjectName(QString::fromUtf8("logView"));
        logView->setReadOnly(true);

        gridLayout->addWidget(logView, 0, 0, 1, 1);

        tabWidget->addTab(logTab, QString());

        verticalLayout->addWidget(tabWidget);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
        startButton = new QPushButton(tab);
        startButton->setObjectName(QString::fromUtf8("startButton"));

        horizontalLayout_5->addWidget(startButton);

        pauseButton = new QPushButton(tab);
        pauseButton->setObjectName(QString::fromUtf8("pauseButton"));
        pauseButton->setEnabled(false);

        horizontalLayout_5->addWidget(pauseButton);

        stopButton = new QPushButton(tab);
        stopButton->setObjectName(QString::fromUtf8("stopButton"));
        stopButton->setEnabled(false);

        horizontalLayout_5->addWidget(stopButton);


        verticalLayout->addLayout(horizontalLayout_5);

        statusLabel = new QLabel(tab);
        statusLabel->setObjectName(QString::fromUtf8("statusLabel"));

        verticalLayout->addWidget(statusLabel);

        progressBar = new QProgressBar(tab);
        progressBar->setObjectName(QString::fromUtf8("progressBar"));
        progressBar->setValue(0);

        verticalLayout->addWidget(progressBar);


        gridLayout_2->addLayout(verticalLayout, 0, 0, 1, 1);

        tabWidget_2->addTab(tab, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName(QString::fromUtf8("tab_2"));
        gridLayout_7 = new QGridLayout(tab_2);
        gridLayout_7->setObjectName(QString::fromUtf8("gridLayout_7"));
        gridLayout_6 = new QGridLayout();
        gridLayout_6->setObjectName(QString::fromUtf8("gridLayout_6"));
        groupBox = new QGroupBox(tab_2);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        gridLayout_5 = new QGridLayout(groupBox);
        gridLayout_5->setObjectName(QString::fromUtf8("gridLayout_5"));
        verticalLayout_6 = new QVBoxLayout();
        verticalLayout_6->setObjectName(QString::fromUtf8("verticalLayout_6"));
        verticalLayout_3 = new QVBoxLayout();
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        label_5 = new QLabel(groupBox);
        label_5->setObjectName(QString::fromUtf8("label_5"));

        verticalLayout_3->addWidget(label_5);

        lineEdit = new QLineEdit(groupBox);
        lineEdit->setObjectName(QString::fromUtf8("lineEdit"));
        lineEdit->setReadOnly(true);

        verticalLayout_3->addWidget(lineEdit);


        verticalLayout_6->addLayout(verticalLayout_3);

        textEdit = new QTextEdit(groupBox);
        textEdit->setObjectName(QString::fromUtf8("textEdit"));

        verticalLayout_6->addWidget(textEdit);


        gridLayout_5->addLayout(verticalLayout_6, 0, 0, 1, 1);


        gridLayout_6->addWidget(groupBox, 0, 0, 1, 1);

        groupBox_2 = new QGroupBox(tab_2);
        groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
        gridLayout_4 = new QGridLayout(groupBox_2);
        gridLayout_4->setObjectName(QString::fromUtf8("gridLayout_4"));
        textEdit_2 = new QTextEdit(groupBox_2);
        textEdit_2->setObjectName(QString::fromUtf8("textEdit_2"));

        gridLayout_4->addWidget(textEdit_2, 0, 0, 1, 1);


        gridLayout_6->addWidget(groupBox_2, 1, 0, 1, 1);

        verticalLayout_4 = new QVBoxLayout();
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        horizontalLayout_7 = new QHBoxLayout();
        horizontalLayout_7->setObjectName(QString::fromUtf8("horizontalLayout_7"));
        label_7 = new QLabel(tab_2);
        label_7->setObjectName(QString::fromUtf8("label_7"));

        horizontalLayout_7->addWidget(label_7);

        addrEdit_2 = new QLineEdit(tab_2);
        addrEdit_2->setObjectName(QString::fromUtf8("addrEdit_2"));
        addrEdit_2->setClearButtonEnabled(true);

        horizontalLayout_7->addWidget(addrEdit_2);


        verticalLayout_4->addLayout(horizontalLayout_7);

        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setObjectName(QString::fromUtf8("horizontalLayout_6"));
        label_6 = new QLabel(tab_2);
        label_6->setObjectName(QString::fromUtf8("label_6"));

        horizontalLayout_6->addWidget(label_6);

        sendIntervalEdit_2 = new QLineEdit(tab_2);
        sendIntervalEdit_2->setObjectName(QString::fromUtf8("sendIntervalEdit_2"));
        sendIntervalEdit_2->setClearButtonEnabled(true);

        horizontalLayout_6->addWidget(sendIntervalEdit_2);

        uniqueCheck_2 = new QCheckBox(tab_2);
        uniqueCheck_2->setObjectName(QString::fromUtf8("uniqueCheck_2"));

        horizontalLayout_6->addWidget(uniqueCheck_2);


        verticalLayout_4->addLayout(horizontalLayout_6);

        horizontalLayout_8 = new QHBoxLayout();
        horizontalLayout_8->setObjectName(QString::fromUtf8("horizontalLayout_8"));
        startButton_2 = new QPushButton(tab_2);
        startButton_2->setObjectName(QString::fromUtf8("startButton_2"));

        horizontalLayout_8->addWidget(startButton_2);

        pauseButton_2 = new QPushButton(tab_2);
        pauseButton_2->setObjectName(QString::fromUtf8("pauseButton_2"));
        pauseButton_2->setEnabled(false);

        horizontalLayout_8->addWidget(pauseButton_2);

        stopButton_2 = new QPushButton(tab_2);
        stopButton_2->setObjectName(QString::fromUtf8("stopButton_2"));
        stopButton_2->setEnabled(false);

        horizontalLayout_8->addWidget(stopButton_2);


        verticalLayout_4->addLayout(horizontalLayout_8);


        gridLayout_6->addLayout(verticalLayout_4, 2, 0, 1, 1);


        gridLayout_7->addLayout(gridLayout_6, 0, 0, 1, 1);

        tabWidget_2->addTab(tab_2, QString());
        tab_3 = new OrderSendWidget();
        tab_3->setObjectName(QString::fromUtf8("tab_3"));
        tabWidget_2->addTab(tab_3, QString());
        tab_FaultAlarmWidget = new FaultAlarmWidget();
        tab_FaultAlarmWidget->setObjectName(QString::fromUtf8("tab_FaultAlarmWidget"));
        tabWidget_2->addTab(tab_FaultAlarmWidget, QString());

        verticalLayout_5->addWidget(tabWidget_2);


        gridLayout_3->addLayout(verticalLayout_5, 0, 0, 1, 1);

        MainWindow->setCentralWidget(centralwidget);

        retranslateUi(MainWindow);

        tabWidget_2->setCurrentIndex(0);
        tabWidget->setCurrentIndex(1);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "Data Processor", nullptr));
        action_2->setText(QApplication::translate("MainWindow", "\347\247\221\346\212\200\350\223\235", nullptr));
        dirEdit->setPlaceholderText(QApplication::translate("MainWindow", "\351\273\230\350\256\244\344\270\272\347\250\213\345\272\217\350\277\220\350\241\214\347\233\256\345\275\225, \344\270\215\345\217\257\344\270\272\347\251\272", nullptr));
        browseButton->setText(QApplication::translate("MainWindow", "\346\265\217\350\247\210...", nullptr));
        addrEdit->setPlaceholderText(QApplication::translate("MainWindow", "\350\276\223\345\205\245\350\246\201\346\267\273\345\212\240\347\232\204\345\234\260\345\235\200\345\222\214\347\253\257\345\217\243\345\220\216\347\202\271\345\207\273\345\217\263\344\276\247\346\214\211\351\222\256\346\267\273\345\212\240, \346\240\274\345\274\217IP:Port", nullptr));
        addAddrButton->setText(QApplication::translate("MainWindow", "\346\267\273\345\212\240", nullptr));
        removeAddrButton->setText(QApplication::translate("MainWindow", "\347\247\273\351\231\244", nullptr));
        groupBox_3->setTitle(QApplication::translate("MainWindow", "\351\200\211\346\213\251\347\233\256\346\240\207\345\234\260\345\235\200\345\220\216\346\211\215\345\217\257\350\277\233\350\241\214\345\217\221\351\200\201, \346\214\211\344\275\217Ctrl+\351\274\240\346\240\207\345\267\246\351\224\256\345\217\257\345\244\232\351\200\211\347\233\256\347\232\204\345\234\260\345\235\200", nullptr));
        label->setText(QApplication::translate("MainWindow", "\344\273\205\345\217\221\351\200\201\344\270\273\351\242\230\345\217\267:", nullptr));
        includeEdit->setPlaceholderText(QApplication::translate("MainWindow", "\345\244\232\344\270\273\351\242\230\345\217\267\347\224\250\350\213\261\346\226\207\351\200\227\345\217\267\351\232\224\345\274\200, \344\276\213\345\246\202\357\274\23200C6,00C7", nullptr));
        label_2->setText(QApplication::translate("MainWindow", "\346\216\222\351\231\244\344\270\273\351\242\230\345\217\267:", nullptr));
        excludeEdit->setPlaceholderText(QApplication::translate("MainWindow", "\345\244\232\344\270\273\351\242\230\345\217\267\347\224\250\350\213\261\346\226\207\351\200\227\345\217\267\351\232\224\345\274\200, \344\276\213\345\246\202\357\274\23200C6,00C7", nullptr));
        label_3->setText(QApplication::translate("MainWindow", "\346\226\207\344\273\266\345\244\271\351\201\215\345\216\206\351\241\272\345\272\217:", nullptr));
        orderCombo->setItemText(0, QApplication::translate("MainWindow", "\351\241\272\345\272\217", nullptr));
        orderCombo->setItemText(1, QApplication::translate("MainWindow", "\345\200\222\345\272\217", nullptr));

        uniqueCheck->setText(QApplication::translate("MainWindow", "\346\257\217\344\270\252\344\270\273\351\242\230\345\217\267\344\273\205\345\217\221\351\200\201\344\270\200\346\254\241", nullptr));
        label_4->setText(QApplication::translate("MainWindow", "\345\217\221\351\200\201\351\227\264\351\232\224(\346\257\253\347\247\222)\357\274\232", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(basicTab), QApplication::translate("MainWindow", "\345\237\272\346\234\254\350\256\276\347\275\256", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(logTab), QApplication::translate("MainWindow", "\346\227\245\345\277\227\346\230\276\347\244\272", nullptr));
        startButton->setText(QApplication::translate("MainWindow", "\345\274\200\345\247\213", nullptr));
        pauseButton->setText(QApplication::translate("MainWindow", "\346\232\202\345\201\234(P)", nullptr));
        stopButton->setText(QApplication::translate("MainWindow", "\345\201\234\346\255\242", nullptr));
        statusLabel->setText(QApplication::translate("MainWindow", "\347\212\266\346\200\201:", nullptr));
        tabWidget_2->setTabText(tabWidget_2->indexOf(tab), QApplication::translate("MainWindow", "\346\225\260\346\215\256\346\226\207\344\273\266\345\217\221\351\200\201", nullptr));
        groupBox->setTitle(QApplication::translate("MainWindow", "\345\215\201\345\205\255\350\277\233\345\210\266\345\216\237\345\247\213\346\225\260\346\215\256(\346\257\217\350\241\214\344\270\200\344\270\252\346\225\260\346\215\256\345\270\247)\357\274\232", nullptr));
        label_5->setText(QApplication::translate("MainWindow", "\346\240\274\345\274\217\345\217\202\350\200\203(\345\270\246\344\270\215\345\270\246\345\210\206\351\232\224\347\254\246\345\235\207\345\217\257)\357\274\232", nullptr));
        lineEdit->setText(QApplication::translate("MainWindow", "80:21:00:C6:00:21:00:BA:F9:4C:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:01:00:00:00:00:02:8A:03", nullptr));
        textEdit->setPlaceholderText(QApplication::translate("MainWindow", "\350\257\267\350\276\223\345\205\245\351\234\200\350\246\201\345\217\221\351\200\201\347\232\204\345\216\237\345\247\213\346\225\260\346\215\256...", nullptr));
        groupBox_2->setTitle(QApplication::translate("MainWindow", "\345\217\221\351\200\201\346\227\245\345\277\227\357\274\232", nullptr));
        label_7->setText(QApplication::translate("MainWindow", "\345\217\221\351\200\201\345\210\260(\345\234\260\345\235\200:\347\253\257\345\217\243\345\217\267)\357\274\232", nullptr));
        addrEdit_2->setText(QApplication::translate("MainWindow", "239.255.1.21:9221", nullptr));
        addrEdit_2->setPlaceholderText(QApplication::translate("MainWindow", "IP:Port", nullptr));
        label_6->setText(QApplication::translate("MainWindow", "\345\217\221\351\200\201\351\227\264\351\232\224(\346\257\253\347\247\222)\357\274\232", nullptr));
        sendIntervalEdit_2->setText(QApplication::translate("MainWindow", "1000", nullptr));
        uniqueCheck_2->setText(QApplication::translate("MainWindow", "\346\230\257\345\220\246\345\276\252\347\216\257\345\217\221\351\200\201", nullptr));
        startButton_2->setText(QApplication::translate("MainWindow", "\345\274\200\345\247\213", nullptr));
        pauseButton_2->setText(QApplication::translate("MainWindow", "\346\232\202\345\201\234(P)", nullptr));
        stopButton_2->setText(QApplication::translate("MainWindow", "\345\201\234\346\255\242", nullptr));
        tabWidget_2->setTabText(tabWidget_2->indexOf(tab_2), QApplication::translate("MainWindow", "\350\207\252\345\256\232\344\271\211\346\225\260\346\215\256\345\217\221\351\200\201", nullptr));
        tabWidget_2->setTabText(tabWidget_2->indexOf(tab_3), QApplication::translate("MainWindow", "\346\214\207\344\273\244\346\225\260\346\215\256\345\217\221\351\200\201", nullptr));
        tabWidget_2->setTabText(tabWidget_2->indexOf(tab_FaultAlarmWidget), QApplication::translate("MainWindow", "\346\225\205\351\232\234\345\221\212\350\255\246\346\250\241\346\213\237", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
