#include "datahandler.h"
#include <QDebug>
#include <QDateTime>
#include <QThread>
#include <QFile>
#include <QUdpSocket>

DataHandler::DataHandler(QObject *parent) :
    QObject(parent),
    m_bWork(false),
    m_fileSeekPos(0),
    m_length(672),
    m_maxLength(0)
{

}

void DataHandler::StartWork(const QString& fileName, qint64 pos)
{
    m_bWork = !m_bWork;
    m_fileSeekPos = pos;

    // open data file
    QFile fd(fileName);
    if(!fd.open(QIODevice::ReadOnly)) {
        qInfo() << "打开数据文件失败，线程启动失败";
    }
    m_maxLength = fd.size();
    qInfo() << "file size: " << m_maxLength;

    QUdpSocket socket;
    QHostAddress addr("192.168.4.11");
    quint16 port = 2027;

    while (true) {
        m_mutexWork.lock();
        if(!m_bWork) {
            m_mutexWork.unlock();
            break;
        }
        m_mutexWork.unlock();

        if(!fd.isOpen()) {
            QThread::sleep(2);
            continue;
        }
        if(m_fileSeekPos >= m_maxLength)
            break;
        QByteArray data = fd.read(m_length);
        float value = m_fileSeekPos * 100. / m_maxLength;
        emit process(value);
        qInfo() << data.toHex();
        socket.writeDatagram(data, addr, port);

        m_fileSeekPos += m_length;
        qInfo() << "file pos: " << m_fileSeekPos;

        QThread::msleep(1);
    }

    // close data file
    fd.close();
}

void DataHandler::StopWork()
{
    QMutexLocker locker(&m_mutexWork);
    m_bWork = !m_bWork;
}
