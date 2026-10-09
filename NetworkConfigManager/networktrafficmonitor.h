#ifndef NETWORKTRAFFICMONITOR_H
#define NETWORKTRAFFICMONITOR_H

#include <QLabel>
#include <QElapsedTimer>

class NetworkTrafficMonitor : public QLabel
{
public:
    explicit NetworkTrafficMonitor(QWidget *parent = nullptr);
    void setInterfaceName(const QString &name);

private:
    void refresh();
    bool readCounters(quint64 *received, quint64 *sent) const;
    static QString rateText(double bytesPerSecond);

    QElapsedTimer m_elapsed;
    quint64 m_lastReceived = 0;
    quint64 m_lastSent = 0;
    bool m_hasSample = false;
    QString m_interfaceName;
};

#endif
