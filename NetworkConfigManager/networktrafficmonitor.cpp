#if defined(_WIN32_WINNT) && _WIN32_WINNT < 0x0601
#undef _WIN32_WINNT
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include "networktrafficmonitor.h"

#include <QTimer>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#endif

NetworkTrafficMonitor::NetworkTrafficMonitor(QWidget *parent) : QLabel(parent)
{
    setText(tr("↑ --  ↓ --"));
    setToolTip(tr("当前选中网卡的实时发送与接收速率"));
    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() { refresh(); });
    timer->start(1000);
    refresh();
}

void NetworkTrafficMonitor::setInterfaceName(const QString &name)
{
    m_interfaceName = name;
    m_hasSample = false;
    refresh();
}

bool NetworkTrafficMonitor::readCounters(quint64 *received, quint64 *sent) const
{
#ifdef _WIN32
    MIB_IF_TABLE2 *table = nullptr;
    if(!m_interfaceName.isEmpty() && GetIfTable2(&table) == NO_ERROR && table)
    {
        for(ULONG index = 0; index < table->NumEntries; ++index)
        {
            const MIB_IF_ROW2 &row = table->Table[index];
            if(row.OperStatus != IfOperStatusUp || row.Type == IF_TYPE_SOFTWARE_LOOPBACK) continue;
            if(QString::fromWCharArray(row.Alias).compare(m_interfaceName, Qt::CaseInsensitive) != 0) continue;
            *received = row.InOctets;
            *sent = row.OutOctets;
            FreeMibTable(table);
            return true;
        }
        FreeMibTable(table);
        return false;
    }
    if(table) FreeMibTable(table);
#endif
    Q_UNUSED(received);
    Q_UNUSED(sent);
    return false;
}

QString NetworkTrafficMonitor::rateText(double rate)
{
    if(rate >= 1024.0 * 1024.0) return QString::number(rate / (1024.0 * 1024.0), 'f', 1) + " MiB/s";
    if(rate >= 1024.0) return QString::number(rate / 1024.0, 'f', 1) + " KiB/s";
    return QString::number(rate, 'f', 0) + " B/s";
}

void NetworkTrafficMonitor::refresh()
{
    quint64 received = 0, sent = 0;
    if(!readCounters(&received, &sent)) { setText(tr("↑ --  ↓ --")); m_hasSample = false; return; }
    if(m_hasSample && received >= m_lastReceived && sent >= m_lastSent)
    {
        const qint64 elapsed = m_elapsed.elapsed();
        if(elapsed > 0)
            setText(QString("↑ %1  ↓ %2").arg(rateText((sent - m_lastSent) * 1000.0 / elapsed),
                                               rateText((received - m_lastReceived) * 1000.0 / elapsed)));
    }
    m_lastReceived = received;
    m_lastSent = sent;
    m_hasSample = true;
    m_elapsed.restart();
}
