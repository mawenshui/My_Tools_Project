#ifndef NETWORKADAPTERMANAGER_H
#define NETWORKADAPTERMANAGER_H

#include <QObject>

class NetworkAdapterManager : public QObject
{
    Q_OBJECT
public:
    explicit NetworkAdapterManager(QObject *parent = nullptr);

signals:

};

#endif // NETWORKADAPTERMANAGER_H
