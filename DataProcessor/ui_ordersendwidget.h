/********************************************************************************
** Form generated from reading UI file 'ordersendwidget.ui'
**
** Created by: Qt User Interface Compiler version 5.12.8
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ORDERSENDWIDGET_H
#define UI_ORDERSENDWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_OrderSendWidget
{
public:
    QGridLayout *gridLayout_2;
    QSplitter *splitter_2;
    QSplitter *splitter;
    QGroupBox *groupBox;
    QGridLayout *gridLayout_7;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout;
    QLineEdit *lineEditIP;
    QGroupBox *groupBox_3;
    QGridLayout *gridLayout_5;
    QLineEdit *lineEditPort;
    QGroupBox *groupBox_4;
    QGridLayout *gridLayout_6;
    QLineEdit *lineEdit_searchOrder;
    QComboBox *comboBox;
    QHBoxLayout *horizontalLayout_2;
    QPushButton *pushButtonstart;
    QPushButton *pushButton;
    QPushButton *pushButtonStop;
    QSpacerItem *verticalSpacer;
    QGroupBox *groupBoxOptions;
    QGridLayout *gridLayout_3;
    QCheckBox *noCRC;
    QCheckBox *noreplay;
    QCheckBox *replyNOCRC;
    QCheckBox *reNOCRC;
    QCheckBox *noReturnReply;
    QCheckBox *ReERR;
    QCheckBox *noreturn;
    QCheckBox *noShake;
    QCheckBox *isReplyTimeout;
    QCheckBox *isReturnTimeout;
    QCheckBox *isReReplyTimeout;
    QGroupBox *groupBoxLog;
    QGridLayout *gridLayout_4;
    QTextEdit *textEdit;
    QPushButton *pushButtonClearLog;

    void setupUi(QWidget *OrderSendWidget)
    {
        if (OrderSendWidget->objectName().isEmpty())
            OrderSendWidget->setObjectName(QString::fromUtf8("OrderSendWidget"));
        OrderSendWidget->resize(700, 425);
        gridLayout_2 = new QGridLayout(OrderSendWidget);
        gridLayout_2->setSpacing(6);
        gridLayout_2->setContentsMargins(11, 11, 11, 11);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        splitter_2 = new QSplitter(OrderSendWidget);
        splitter_2->setObjectName(QString::fromUtf8("splitter_2"));
        splitter_2->setOrientation(Qt::Horizontal);
        splitter = new QSplitter(splitter_2);
        splitter->setObjectName(QString::fromUtf8("splitter"));
        splitter->setOrientation(Qt::Horizontal);
        groupBox = new QGroupBox(splitter);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        gridLayout_7 = new QGridLayout(groupBox);
        gridLayout_7->setSpacing(6);
        gridLayout_7->setContentsMargins(11, 11, 11, 11);
        gridLayout_7->setObjectName(QString::fromUtf8("gridLayout_7"));
        groupBox_2 = new QGroupBox(groupBox);
        groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
        gridLayout = new QGridLayout(groupBox_2);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        lineEditIP = new QLineEdit(groupBox_2);
        lineEditIP->setObjectName(QString::fromUtf8("lineEditIP"));

        gridLayout->addWidget(lineEditIP, 0, 0, 1, 1);


        gridLayout_7->addWidget(groupBox_2, 0, 0, 1, 1);

        groupBox_3 = new QGroupBox(groupBox);
        groupBox_3->setObjectName(QString::fromUtf8("groupBox_3"));
        gridLayout_5 = new QGridLayout(groupBox_3);
        gridLayout_5->setSpacing(6);
        gridLayout_5->setContentsMargins(11, 11, 11, 11);
        gridLayout_5->setObjectName(QString::fromUtf8("gridLayout_5"));
        lineEditPort = new QLineEdit(groupBox_3);
        lineEditPort->setObjectName(QString::fromUtf8("lineEditPort"));

        gridLayout_5->addWidget(lineEditPort, 0, 0, 1, 1);


        gridLayout_7->addWidget(groupBox_3, 1, 0, 1, 1);

        groupBox_4 = new QGroupBox(groupBox);
        groupBox_4->setObjectName(QString::fromUtf8("groupBox_4"));
        gridLayout_6 = new QGridLayout(groupBox_4);
        gridLayout_6->setSpacing(6);
        gridLayout_6->setContentsMargins(11, 11, 11, 11);
        gridLayout_6->setObjectName(QString::fromUtf8("gridLayout_6"));
        lineEdit_searchOrder = new QLineEdit(groupBox_4);
        lineEdit_searchOrder->setObjectName(QString::fromUtf8("lineEdit_searchOrder"));
        lineEdit_searchOrder->setClearButtonEnabled(true);

        gridLayout_6->addWidget(lineEdit_searchOrder, 0, 0, 1, 1);

        comboBox = new QComboBox(groupBox_4);
        comboBox->setObjectName(QString::fromUtf8("comboBox"));

        gridLayout_6->addWidget(comboBox, 1, 0, 1, 1);


        gridLayout_7->addWidget(groupBox_4, 2, 0, 1, 1);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        pushButtonstart = new QPushButton(groupBox);
        pushButtonstart->setObjectName(QString::fromUtf8("pushButtonstart"));

        horizontalLayout_2->addWidget(pushButtonstart);

        pushButton = new QPushButton(groupBox);
        pushButton->setObjectName(QString::fromUtf8("pushButton"));

        horizontalLayout_2->addWidget(pushButton);

        pushButtonStop = new QPushButton(groupBox);
        pushButtonStop->setObjectName(QString::fromUtf8("pushButtonStop"));
        pushButtonStop->setEnabled(false);

        horizontalLayout_2->addWidget(pushButtonStop);


        gridLayout_7->addLayout(horizontalLayout_2, 3, 0, 1, 1);

        verticalSpacer = new QSpacerItem(20, 141, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_7->addItem(verticalSpacer, 4, 0, 1, 1);

        splitter->addWidget(groupBox);
        groupBoxOptions = new QGroupBox(splitter);
        groupBoxOptions->setObjectName(QString::fromUtf8("groupBoxOptions"));
        gridLayout_3 = new QGridLayout(groupBoxOptions);
        gridLayout_3->setSpacing(6);
        gridLayout_3->setContentsMargins(11, 11, 11, 11);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        noCRC = new QCheckBox(groupBoxOptions);
        noCRC->setObjectName(QString::fromUtf8("noCRC"));

        gridLayout_3->addWidget(noCRC, 0, 0, 1, 1);

        noreplay = new QCheckBox(groupBoxOptions);
        noreplay->setObjectName(QString::fromUtf8("noreplay"));

        gridLayout_3->addWidget(noreplay, 1, 0, 1, 1);

        replyNOCRC = new QCheckBox(groupBoxOptions);
        replyNOCRC->setObjectName(QString::fromUtf8("replyNOCRC"));

        gridLayout_3->addWidget(replyNOCRC, 2, 0, 1, 1);

        reNOCRC = new QCheckBox(groupBoxOptions);
        reNOCRC->setObjectName(QString::fromUtf8("reNOCRC"));

        gridLayout_3->addWidget(reNOCRC, 3, 0, 1, 1);

        noReturnReply = new QCheckBox(groupBoxOptions);
        noReturnReply->setObjectName(QString::fromUtf8("noReturnReply"));

        gridLayout_3->addWidget(noReturnReply, 4, 0, 1, 1);

        ReERR = new QCheckBox(groupBoxOptions);
        ReERR->setObjectName(QString::fromUtf8("ReERR"));

        gridLayout_3->addWidget(ReERR, 5, 0, 1, 1);

        noreturn = new QCheckBox(groupBoxOptions);
        noreturn->setObjectName(QString::fromUtf8("noreturn"));

        gridLayout_3->addWidget(noreturn, 6, 0, 1, 1);

        noShake = new QCheckBox(groupBoxOptions);
        noShake->setObjectName(QString::fromUtf8("noShake"));

        gridLayout_3->addWidget(noShake, 7, 0, 1, 1);

        isReplyTimeout = new QCheckBox(groupBoxOptions);
        isReplyTimeout->setObjectName(QString::fromUtf8("isReplyTimeout"));

        gridLayout_3->addWidget(isReplyTimeout, 8, 0, 1, 1);

        isReturnTimeout = new QCheckBox(groupBoxOptions);
        isReturnTimeout->setObjectName(QString::fromUtf8("isReturnTimeout"));

        gridLayout_3->addWidget(isReturnTimeout, 9, 0, 1, 1);

        isReReplyTimeout = new QCheckBox(groupBoxOptions);
        isReReplyTimeout->setObjectName(QString::fromUtf8("isReReplyTimeout"));

        gridLayout_3->addWidget(isReReplyTimeout, 10, 0, 1, 1);

        splitter->addWidget(groupBoxOptions);
        splitter_2->addWidget(splitter);
        groupBoxLog = new QGroupBox(splitter_2);
        groupBoxLog->setObjectName(QString::fromUtf8("groupBoxLog"));
        gridLayout_4 = new QGridLayout(groupBoxLog);
        gridLayout_4->setSpacing(6);
        gridLayout_4->setContentsMargins(11, 11, 11, 11);
        gridLayout_4->setObjectName(QString::fromUtf8("gridLayout_4"));
        textEdit = new QTextEdit(groupBoxLog);
        textEdit->setObjectName(QString::fromUtf8("textEdit"));
        textEdit->setReadOnly(true);

        gridLayout_4->addWidget(textEdit, 0, 0, 1, 1);

        pushButtonClearLog = new QPushButton(groupBoxLog);
        pushButtonClearLog->setObjectName(QString::fromUtf8("pushButtonClearLog"));

        gridLayout_4->addWidget(pushButtonClearLog, 1, 0, 1, 1);

        splitter_2->addWidget(groupBoxLog);

        gridLayout_2->addWidget(splitter_2, 0, 0, 1, 1);


        retranslateUi(OrderSendWidget);

        QMetaObject::connectSlotsByName(OrderSendWidget);
    } // setupUi

    void retranslateUi(QWidget *OrderSendWidget)
    {
        OrderSendWidget->setWindowTitle(QApplication::translate("OrderSendWidget", "\347\275\221\347\273\234\346\225\260\346\215\256\345\217\221\351\200\201\345\267\245\345\205\267", nullptr));
        groupBox->setTitle(QApplication::translate("OrderSendWidget", "\345\217\221\351\200\201\350\256\276\347\275\256", nullptr));
        groupBox_2->setTitle(QApplication::translate("OrderSendWidget", "\347\233\256\346\240\207\345\234\260\345\235\200\357\274\232", nullptr));
#ifndef QT_NO_TOOLTIP
        lineEditIP->setToolTip(QApplication::translate("OrderSendWidget", "\350\257\267\350\276\223\345\205\245\347\233\256\346\240\207IP\345\234\260\345\235\200", nullptr));
#endif // QT_NO_TOOLTIP
        groupBox_3->setTitle(QApplication::translate("OrderSendWidget", "\347\253\257\345\217\243\345\217\267\357\274\232", nullptr));
#ifndef QT_NO_TOOLTIP
        lineEditPort->setToolTip(QApplication::translate("OrderSendWidget", "\350\257\267\350\276\223\345\205\245\347\253\257\345\217\243\345\217\267\357\274\2101-65535\357\274\211", nullptr));
#endif // QT_NO_TOOLTIP
        groupBox_4->setTitle(QApplication::translate("OrderSendWidget", "\346\214\207\344\273\244\351\200\211\346\213\251\357\274\232", nullptr));
        lineEdit_searchOrder->setPlaceholderText(QApplication::translate("OrderSendWidget", "\346\220\234\347\264\242\346\214\207\344\273\244...", nullptr));
#ifndef QT_NO_TOOLTIP
        comboBox->setToolTip(QApplication::translate("OrderSendWidget", "\350\257\267\351\200\211\346\213\251\350\246\201\345\217\221\351\200\201\347\232\204\345\221\275\344\273\244", nullptr));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        pushButtonstart->setToolTip(QApplication::translate("OrderSendWidget", "\347\202\271\345\207\273\345\217\221\351\200\201\346\225\260\346\215\256", nullptr));
#endif // QT_NO_TOOLTIP
        pushButtonstart->setText(QApplication::translate("OrderSendWidget", "\345\274\200\345\247\213\345\217\221\351\200\201", nullptr));
#ifndef QT_NO_TOOLTIP
        pushButton->setToolTip(QApplication::translate("OrderSendWidget", "\345\276\252\347\216\257\345\217\221\351\200\201\351\200\211\344\270\255\347\232\204\345\221\275\344\273\244", nullptr));
#endif // QT_NO_TOOLTIP
        pushButton->setText(QApplication::translate("OrderSendWidget", "\345\276\252\347\216\257\345\217\221\351\200\201", nullptr));
#ifndef QT_NO_TOOLTIP
        pushButtonStop->setToolTip(QApplication::translate("OrderSendWidget", "\345\201\234\346\255\242\345\276\252\347\216\257\345\217\221\351\200\201", nullptr));
#endif // QT_NO_TOOLTIP
        pushButtonStop->setText(QApplication::translate("OrderSendWidget", "\345\201\234\346\255\242", nullptr));
        groupBoxOptions->setTitle(QApplication::translate("OrderSendWidget", "\351\200\211\351\241\271\350\256\276\347\275\256", nullptr));
#ifndef QT_NO_TOOLTIP
        noCRC->setToolTip(QApplication::translate("OrderSendWidget", "\345\220\257\347\224\250\345\221\275\344\273\244\346\240\241\351\252\214\351\224\231\350\257\257\346\250\241\346\213\237", nullptr));
#endif // QT_NO_TOOLTIP
        noCRC->setText(QApplication::translate("OrderSendWidget", "\345\221\275\344\273\244\346\240\241\351\252\214\351\224\231", nullptr));
#ifndef QT_NO_TOOLTIP
        noreplay->setToolTip(QApplication::translate("OrderSendWidget", "\346\250\241\346\213\237\346\227\240\345\272\224\347\255\224\345\234\272\346\231\257", nullptr));
#endif // QT_NO_TOOLTIP
        noreplay->setText(QApplication::translate("OrderSendWidget", "\346\227\240\345\272\224\347\255\224", nullptr));
#ifndef QT_NO_TOOLTIP
        replyNOCRC->setToolTip(QApplication::translate("OrderSendWidget", "\346\250\241\346\213\237\345\272\224\347\255\224\346\240\241\351\252\214\351\224\231\350\257\257", nullptr));
#endif // QT_NO_TOOLTIP
        replyNOCRC->setText(QApplication::translate("OrderSendWidget", "\345\272\224\347\255\224\346\240\241\351\252\214\351\224\231", nullptr));
#ifndef QT_NO_TOOLTIP
        reNOCRC->setToolTip(QApplication::translate("OrderSendWidget", "\346\250\241\346\213\237\345\217\215\351\246\210\346\240\241\351\252\214\351\224\231\350\257\257", nullptr));
#endif // QT_NO_TOOLTIP
        reNOCRC->setText(QApplication::translate("OrderSendWidget", "\345\217\215\351\246\210\346\240\241\351\252\214\351\224\231", nullptr));
#ifndef QT_NO_TOOLTIP
        noReturnReply->setToolTip(QApplication::translate("OrderSendWidget", "\346\250\241\346\213\237\345\217\215\351\246\210\346\227\240\345\272\224\347\255\224\345\234\272\346\231\257", nullptr));
#endif // QT_NO_TOOLTIP
        noReturnReply->setText(QApplication::translate("OrderSendWidget", "\345\217\215\351\246\210\346\227\240\345\272\224\347\255\224", nullptr));
#ifndef QT_NO_TOOLTIP
        ReERR->setToolTip(QApplication::translate("OrderSendWidget", "\346\250\241\346\213\237\345\217\215\351\246\210\351\224\231\350\257\257", nullptr));
#endif // QT_NO_TOOLTIP
        ReERR->setText(QApplication::translate("OrderSendWidget", "\345\217\215\351\246\210\351\224\231\350\257\257", nullptr));
#ifndef QT_NO_TOOLTIP
        noreturn->setToolTip(QApplication::translate("OrderSendWidget", "\346\250\241\346\213\237\346\227\240\345\217\215\351\246\210\345\234\272\346\231\257", nullptr));
#endif // QT_NO_TOOLTIP
        noreturn->setText(QApplication::translate("OrderSendWidget", "\346\227\240\345\217\215\351\246\210", nullptr));
#ifndef QT_NO_TOOLTIP
        noShake->setToolTip(QApplication::translate("OrderSendWidget", "\346\250\241\346\213\237\346\227\240\345\217\215\351\246\210\345\234\272\346\231\257", nullptr));
#endif // QT_NO_TOOLTIP
        noShake->setText(QApplication::translate("OrderSendWidget", "\346\227\240\351\234\200\345\272\224\347\255\224", nullptr));
#ifndef QT_NO_TOOLTIP
        isReplyTimeout->setToolTip(QApplication::translate("OrderSendWidget", "\346\250\241\346\213\237\346\227\240\345\217\215\351\246\210\345\234\272\346\231\257", nullptr));
#endif // QT_NO_TOOLTIP
        isReplyTimeout->setText(QApplication::translate("OrderSendWidget", "\345\272\224\347\255\224\350\266\205\346\227\266", nullptr));
#ifndef QT_NO_TOOLTIP
        isReturnTimeout->setToolTip(QApplication::translate("OrderSendWidget", "\346\250\241\346\213\237\346\227\240\345\217\215\351\246\210\345\234\272\346\231\257", nullptr));
#endif // QT_NO_TOOLTIP
        isReturnTimeout->setText(QApplication::translate("OrderSendWidget", "\345\217\215\351\246\210\350\266\205\346\227\266", nullptr));
#ifndef QT_NO_TOOLTIP
        isReReplyTimeout->setToolTip(QApplication::translate("OrderSendWidget", "\346\250\241\346\213\237\346\227\240\345\217\215\351\246\210\345\234\272\346\231\257", nullptr));
#endif // QT_NO_TOOLTIP
        isReReplyTimeout->setText(QApplication::translate("OrderSendWidget", "\345\217\215\351\246\210\345\272\224\347\255\224\350\266\205\346\227\266", nullptr));
        groupBoxLog->setTitle(QApplication::translate("OrderSendWidget", "\351\200\232\344\277\241\346\227\245\345\277\227", nullptr));
#ifndef QT_NO_TOOLTIP
        textEdit->setToolTip(QApplication::translate("OrderSendWidget", "\346\230\276\347\244\272\345\217\221\351\200\201\345\222\214\346\216\245\346\224\266\347\232\204\346\227\245\345\277\227\344\277\241\346\201\257", nullptr));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        pushButtonClearLog->setToolTip(QApplication::translate("OrderSendWidget", "\346\270\205\347\251\272\346\227\245\345\277\227\345\206\205\345\256\271", nullptr));
#endif // QT_NO_TOOLTIP
        pushButtonClearLog->setText(QApplication::translate("OrderSendWidget", "\346\270\205\347\251\272\346\227\245\345\277\227", nullptr));
    } // retranslateUi

};

namespace Ui {
    class OrderSendWidget: public Ui_OrderSendWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ORDERSENDWIDGET_H
