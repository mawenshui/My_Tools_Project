//sendworker.h
#ifndef SENDWORKER_H
#define SENDWORKER_H

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QThread>
#include <QDebug>

class SendWorker : public QObject
{
    Q_OBJECT
public:
    explicit SendWorker(QObject *parent = nullptr);
    ~SendWorker();

public slots:
    void sendCommand(const QString &address, quint16 port,
                     const QString &comname, const QMap<QString, int>& commandIdMap,
                     const QMap<QString, int>& workIdMap, const QMap<QString, double>& timeMap,
                     bool comCrc, bool reCrc, bool comReply, bool havereturn,
                     bool ReReply, bool replycrc, bool returnErr, bool noShake,
                     bool isReplyTimeout, bool isReturnTimeout, bool isReReplyTimeout);

signals:
    void logMessage(QString level, QString message);
    void finished();

private:
    QUdpSocket* m_sender;
    void crcData(unsigned char* data, int len);
};

#endif //SENDWORKER_H
