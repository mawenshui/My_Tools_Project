#ifndef NETWORKADAPTERDIALOG_H
#define NETWORKADAPTERDIALOG_H

#include <QDialog>
#include <QJsonObject>
#include <QMap>
#include <functional>

class ConfigManager;
class QComboBox;
class QPlainTextEdit;
class QProcess;
class QPushButton;
class QTableWidget;
class QTimer;

class NetworkAdapterDialog : public QDialog
{
    Q_OBJECT
public:
    explicit NetworkAdapterDialog(ConfigManager *manager, QWidget *parent = nullptr);
    ~NetworkAdapterDialog() override;
    static QString snapshotScript();

private:
    using Completion = std::function<void(bool, const QString &)>;
    void runScript(const QString &script, const QMap<QString, QString> &values, Completion done);
    void refresh();
    void showSnapshot(const QJsonObject &snapshot);
    void refreshWifi();
    void mutate(const QString &title, const QString &script, const QMap<QString, QString> &values,
                const QJsonObject &undo);
    bool saveUndo(const QString &title, const QJsonObject &before, const QJsonObject &undo);
    void undoLast();
    void addAddress(bool ipv6);
    void removeAddress(bool ipv6);
    void addRoute();
    void removeRoute();
    void changeMtu();
    void flushDns();

    ConfigManager *m_manager;
    QComboBox *m_adapter;
    QPlainTextEdit *m_details;
    QPlainTextEdit *m_dns;
    QPlainTextEdit *m_wifi;
    QTableWidget *m_addresses;
    QTableWidget *m_routes;
    QPushButton *m_cancel;
    QProcess *m_process = nullptr;
    QTimer *m_timeout;
    QJsonObject m_snapshot;
    QString m_backupFile;
};

#endif
