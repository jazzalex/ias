#include <QObject>
#include <QUdpSocket>
#include <QQueue>

#ifndef UDP_H
#define UDP_H

#include <iostream>

class udp : public QObject
{
    Q_OBJECT

public:
    char *inputPacket;
    char *outputPacket;

    explicit udp(QObject *parent = 0);
    void sendInput();

    unsigned int recBytes;

    QQueue<char> fifo;

signals:

public slots:
    void readyRead();


private:
    QUdpSocket *socket;
    QIODevice *device;
};

#endif // UDP_H




