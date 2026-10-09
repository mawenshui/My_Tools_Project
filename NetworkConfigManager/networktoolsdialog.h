#ifndef NETWORKTOOLSDIALOG_H
#define NETWORKTOOLSDIALOG_H

#include <QDialog>
#include <QStringList>

class ConfigManager;
class QComboBox;
class QListWidget;

class NetworkToolsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit NetworkToolsDialog(ConfigManager *manager, QWidget *parent = nullptr);

private:
    QStringList checkedInterfaces() const;
    void runBatch(bool dhcp);
    void restoreSelected();
    void refreshInterfaces();

    ConfigManager *m_manager;
    QComboBox *m_profiles;
    QListWidget *m_interfaces;
};

#endif
