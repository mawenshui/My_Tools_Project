#ifndef HISTORYDIALOG_H
#define HISTORYDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QLabel>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QPushButton>
#include <QGroupBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QProgressDialog>
#include <QApplication>
#include <QDateTime>
#include <QTimer>
#include "historymanager.h"

class HistoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HistoryDialog(HistoryManager *historyManager, QWidget *parent = nullptr);
    ~HistoryDialog();

private slots:
    void onRefreshData();
    void performRefresh();
    void onExportCSV();
    void onExportExcel();
    void onDriveSelectionChanged();
    void onTimeRangeChanged();
    void onAutoRefreshToggled(bool enabled);
    void onAutoRefreshTimer();

private:
    void setupUI();
    void setupTable();
    void setupControls();
    void loadHistoryData();
    void populateDriveComboBox();
    void updateStatusLabel();
    void formatTableData();
    QString formatBytes(qint64 bytes) const;
    QString formatDateTime(const QDateTime &dateTime) const;
    
    // UI组件
    QVBoxLayout *mainLayout;
    QHBoxLayout *controlLayout;
    QHBoxLayout *buttonLayout;
    
    QGroupBox *filterGroupBox;
    QGroupBox *dataGroupBox;
    
    QComboBox *driveComboBox;
    QDateTimeEdit *startTimeEdit;
    QDateTimeEdit *endTimeEdit;
    QPushButton *refreshButton;
    QPushButton *exportCSVButton;
    QPushButton *exportExcelButton;
    QPushButton *autoRefreshButton;
    
    QTableWidget *historyTable;
    QLabel *statusLabel;
    
    // 数据管理
    HistoryManager *historyManager;
    QList<HistoryRecord> currentData;
    QTimer *autoRefreshTimer;
    QTimer *debounceTimer;
    bool autoRefreshEnabled;
    bool isRefreshing;
    
    // 常量
    static const int AUTO_REFRESH_INTERVAL = 30000; // 30秒
    static const int MAX_DISPLAY_RECORDS = 1000;    // 最大显示记录数
    static const int DEBOUNCE_DELAY = 1000;         // 防抖延迟1秒
};

#endif // HISTORYDIALOG_H