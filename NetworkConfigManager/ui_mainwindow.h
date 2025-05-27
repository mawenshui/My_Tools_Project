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
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralWidget;
    QGridLayout *gridLayout_3;
    QVBoxLayout *verticalLayout;
    QGroupBox *groupBox_Interface;
    QGridLayout *gridLayout_2;
    QComboBox *networkInterfaceCombo;
    QPushButton *enableInterfaceBtn;
    QPushButton *disableInterfaceBtn;
    QPushButton *refreshInterfacesBtn;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout;
    QStatusBar *statusBar;
    QSplitter *splitter;
    QGroupBox *configGroupBox;
    QVBoxLayout *verticalLayout_2;
    QListWidget *configList;
    QGroupBox *detailGroupBox;
    QVBoxLayout *verticalLayout_3;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label;
    QComboBox *interfaceCombo;
    QPushButton *refreshButton;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_2;
    QRadioButton *staticRadio;
    QRadioButton *dhcpRadio;
    QHBoxLayout *horizontalLayout_4;
    QLabel *label_3;
    QLineEdit *ipEdit;
    QHBoxLayout *horizontalLayout_5;
    QLabel *label_4;
    QLineEdit *subnetEdit;
    QHBoxLayout *horizontalLayout_6;
    QLabel *label_5;
    QLineEdit *gatewayEdit;
    QHBoxLayout *horizontalLayout_7;
    QLabel *label_6;
    QLineEdit *primaryDnsEdit;
    QLabel *label_7;
    QLineEdit *secondaryDnsEdit;
    QHBoxLayout *horizontalLayout_8;
    QPushButton *addButton;
    QPushButton *updateButton;
    QPushButton *deleteButton;
    QPushButton *applyButton;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(913, 487);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/images/icon.png"), QSize(), QIcon::Normal, QIcon::Off);
        MainWindow->setWindowIcon(icon);
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        gridLayout_3 = new QGridLayout(centralWidget);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        groupBox_Interface = new QGroupBox(centralWidget);
        groupBox_Interface->setObjectName(QString::fromUtf8("groupBox_Interface"));
        gridLayout_2 = new QGridLayout(groupBox_Interface);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        networkInterfaceCombo = new QComboBox(groupBox_Interface);
        networkInterfaceCombo->setObjectName(QString::fromUtf8("networkInterfaceCombo"));

        gridLayout_2->addWidget(networkInterfaceCombo, 0, 0, 1, 1);

        enableInterfaceBtn = new QPushButton(groupBox_Interface);
        enableInterfaceBtn->setObjectName(QString::fromUtf8("enableInterfaceBtn"));

        gridLayout_2->addWidget(enableInterfaceBtn, 0, 1, 1, 1);

        disableInterfaceBtn = new QPushButton(groupBox_Interface);
        disableInterfaceBtn->setObjectName(QString::fromUtf8("disableInterfaceBtn"));

        gridLayout_2->addWidget(disableInterfaceBtn, 0, 2, 1, 1);

        refreshInterfacesBtn = new QPushButton(groupBox_Interface);
        refreshInterfacesBtn->setObjectName(QString::fromUtf8("refreshInterfacesBtn"));

        gridLayout_2->addWidget(refreshInterfacesBtn, 0, 3, 1, 1);


        verticalLayout->addWidget(groupBox_Interface);

        groupBox_2 = new QGroupBox(centralWidget);
        groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
        gridLayout = new QGridLayout(groupBox_2);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        statusBar = new QStatusBar(groupBox_2);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));

        gridLayout->addWidget(statusBar, 0, 0, 1, 1);


        verticalLayout->addWidget(groupBox_2);

        splitter = new QSplitter(centralWidget);
        splitter->setObjectName(QString::fromUtf8("splitter"));
        splitter->setOrientation(Qt::Horizontal);
        configGroupBox = new QGroupBox(splitter);
        configGroupBox->setObjectName(QString::fromUtf8("configGroupBox"));
        verticalLayout_2 = new QVBoxLayout(configGroupBox);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        configList = new QListWidget(configGroupBox);
        configList->setObjectName(QString::fromUtf8("configList"));
        configList->setMinimumSize(QSize(200, 0));

        verticalLayout_2->addWidget(configList);

        splitter->addWidget(configGroupBox);
        detailGroupBox = new QGroupBox(splitter);
        detailGroupBox->setObjectName(QString::fromUtf8("detailGroupBox"));
        verticalLayout_3 = new QVBoxLayout(detailGroupBox);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        label = new QLabel(detailGroupBox);
        label->setObjectName(QString::fromUtf8("label"));

        horizontalLayout_2->addWidget(label);

        interfaceCombo = new QComboBox(detailGroupBox);
        interfaceCombo->setObjectName(QString::fromUtf8("interfaceCombo"));

        horizontalLayout_2->addWidget(interfaceCombo);

        refreshButton = new QPushButton(detailGroupBox);
        refreshButton->setObjectName(QString::fromUtf8("refreshButton"));
        refreshButton->setIcon(icon);

        horizontalLayout_2->addWidget(refreshButton);


        verticalLayout_3->addLayout(horizontalLayout_2);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        label_2 = new QLabel(detailGroupBox);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        horizontalLayout_3->addWidget(label_2);

        staticRadio = new QRadioButton(detailGroupBox);
        staticRadio->setObjectName(QString::fromUtf8("staticRadio"));

        horizontalLayout_3->addWidget(staticRadio);

        dhcpRadio = new QRadioButton(detailGroupBox);
        dhcpRadio->setObjectName(QString::fromUtf8("dhcpRadio"));
        dhcpRadio->setChecked(true);

        horizontalLayout_3->addWidget(dhcpRadio);


        verticalLayout_3->addLayout(horizontalLayout_3);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        label_3 = new QLabel(detailGroupBox);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        horizontalLayout_4->addWidget(label_3);

        ipEdit = new QLineEdit(detailGroupBox);
        ipEdit->setObjectName(QString::fromUtf8("ipEdit"));

        horizontalLayout_4->addWidget(ipEdit);


        verticalLayout_3->addLayout(horizontalLayout_4);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
        label_4 = new QLabel(detailGroupBox);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        horizontalLayout_5->addWidget(label_4);

        subnetEdit = new QLineEdit(detailGroupBox);
        subnetEdit->setObjectName(QString::fromUtf8("subnetEdit"));

        horizontalLayout_5->addWidget(subnetEdit);


        verticalLayout_3->addLayout(horizontalLayout_5);

        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setObjectName(QString::fromUtf8("horizontalLayout_6"));
        label_5 = new QLabel(detailGroupBox);
        label_5->setObjectName(QString::fromUtf8("label_5"));

        horizontalLayout_6->addWidget(label_5);

        gatewayEdit = new QLineEdit(detailGroupBox);
        gatewayEdit->setObjectName(QString::fromUtf8("gatewayEdit"));

        horizontalLayout_6->addWidget(gatewayEdit);


        verticalLayout_3->addLayout(horizontalLayout_6);

        horizontalLayout_7 = new QHBoxLayout();
        horizontalLayout_7->setObjectName(QString::fromUtf8("horizontalLayout_7"));
        label_6 = new QLabel(detailGroupBox);
        label_6->setObjectName(QString::fromUtf8("label_6"));

        horizontalLayout_7->addWidget(label_6);

        primaryDnsEdit = new QLineEdit(detailGroupBox);
        primaryDnsEdit->setObjectName(QString::fromUtf8("primaryDnsEdit"));

        horizontalLayout_7->addWidget(primaryDnsEdit);

        label_7 = new QLabel(detailGroupBox);
        label_7->setObjectName(QString::fromUtf8("label_7"));

        horizontalLayout_7->addWidget(label_7);

        secondaryDnsEdit = new QLineEdit(detailGroupBox);
        secondaryDnsEdit->setObjectName(QString::fromUtf8("secondaryDnsEdit"));

        horizontalLayout_7->addWidget(secondaryDnsEdit);


        verticalLayout_3->addLayout(horizontalLayout_7);

        horizontalLayout_8 = new QHBoxLayout();
        horizontalLayout_8->setObjectName(QString::fromUtf8("horizontalLayout_8"));
        addButton = new QPushButton(detailGroupBox);
        addButton->setObjectName(QString::fromUtf8("addButton"));

        horizontalLayout_8->addWidget(addButton);

        updateButton = new QPushButton(detailGroupBox);
        updateButton->setObjectName(QString::fromUtf8("updateButton"));

        horizontalLayout_8->addWidget(updateButton);

        deleteButton = new QPushButton(detailGroupBox);
        deleteButton->setObjectName(QString::fromUtf8("deleteButton"));

        horizontalLayout_8->addWidget(deleteButton);

        applyButton = new QPushButton(detailGroupBox);
        applyButton->setObjectName(QString::fromUtf8("applyButton"));

        horizontalLayout_8->addWidget(applyButton);


        verticalLayout_3->addLayout(horizontalLayout_8);

        splitter->addWidget(detailGroupBox);

        verticalLayout->addWidget(splitter);


        gridLayout_3->addLayout(verticalLayout, 0, 0, 1, 1);

        MainWindow->setCentralWidget(centralWidget);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "IP\351\205\215\347\275\256\347\256\241\347\220\206\345\231\250(by:mws)", nullptr));
        groupBox_Interface->setTitle(QApplication::translate("MainWindow", "\347\275\221\345\215\241\347\256\241\347\220\206", nullptr));
        enableInterfaceBtn->setText(QApplication::translate("MainWindow", "\345\220\257\347\224\250\347\275\221\345\215\241", nullptr));
        disableInterfaceBtn->setText(QApplication::translate("MainWindow", "\347\246\201\347\224\250\347\275\221\345\215\241", nullptr));
        refreshInterfacesBtn->setText(QApplication::translate("MainWindow", "\345\210\267\346\226\260\345\210\227\350\241\250", nullptr));
        groupBox_2->setTitle(QApplication::translate("MainWindow", "\347\212\266\346\200\201\357\274\232", nullptr));
        configGroupBox->setTitle(QApplication::translate("MainWindow", "\351\205\215\347\275\256\345\210\227\350\241\250", nullptr));
        detailGroupBox->setTitle(QApplication::translate("MainWindow", "\351\205\215\347\275\256\350\257\246\346\203\205", nullptr));
        label->setText(QApplication::translate("MainWindow", "\347\275\221\347\273\234\346\216\245\345\217\243:", nullptr));
        refreshButton->setText(QApplication::translate("MainWindow", "\345\210\267\346\226\260", nullptr));
        label_2->setText(QApplication::translate("MainWindow", "IP\350\216\267\345\217\226\346\226\271\345\274\217:", nullptr));
        staticRadio->setText(QApplication::translate("MainWindow", "\351\235\231\346\200\201IP", nullptr));
        dhcpRadio->setText(QApplication::translate("MainWindow", "\350\207\252\345\212\250\350\216\267\345\217\226(DHCP)", nullptr));
        label_3->setText(QApplication::translate("MainWindow", "IP\345\234\260\345\235\200:", nullptr));
        label_4->setText(QApplication::translate("MainWindow", "\345\255\220\347\275\221\346\216\251\347\240\201:", nullptr));
        label_5->setText(QApplication::translate("MainWindow", "\351\273\230\350\256\244\347\275\221\345\205\263:", nullptr));
        label_6->setText(QApplication::translate("MainWindow", "DNS\346\234\215\345\212\241\345\231\250:", nullptr));
        label_7->setText(QApplication::translate("MainWindow", "\345\244\207\347\224\250DNS:", nullptr));
        addButton->setText(QApplication::translate("MainWindow", "\346\267\273\345\212\240\351\205\215\347\275\256", nullptr));
        updateButton->setText(QApplication::translate("MainWindow", "\346\233\264\346\226\260\351\205\215\347\275\256", nullptr));
        deleteButton->setText(QApplication::translate("MainWindow", "\345\210\240\351\231\244\351\205\215\347\275\256", nullptr));
        applyButton->setText(QApplication::translate("MainWindow", "\345\272\224\347\224\250\351\205\215\347\275\256", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
