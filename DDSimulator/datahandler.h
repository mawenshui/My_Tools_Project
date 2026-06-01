#ifndef DATAHANDLER_H
#define DATAHANDLER_H

#include <QObject>
#include <QMutex>

class DataHandler : public QObject
{
    Q_OBJECT
public:
    explicit DataHandler(QObject *parent = nullptr);

signals:
    void message(const QString&);
    void process(int);

public slots:
    void StartWork(const QString&, qint64);
    void StopWork();

private:
    bool    m_bWork;
    qint64  m_fileSeekPos;
    QMutex  m_mutexWork;

    qint64  m_length;
    qint64  m_maxLength;
};

#endif // DATAHANDLER_H
