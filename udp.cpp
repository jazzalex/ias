#include <udp.h>

using namespace std;

udp::udp(QObject *parent):
    QObject{parent}
{
    socket = new QUdpSocket(this);
    socket->bind(QHostAddress("0.0.0.0"),5401);
    connect(socket,SIGNAL(readyRead()),this,SLOT(readyRead()));

    inputPacket    = new char[1024];
    outputPacket   = new char[1024];

    recBytes = 1024;
}

void udp::sendInput(){
    socket->writeDatagram(inputPacket,1024,QHostAddress("144.76.81.210"),5401);
    //cout << "SENDING" << endl;
}


void udp::readyRead(){
    QHostAddress sender;
    quint16 senderPort;

    recBytes = socket->pendingDatagramSize();

    socket->readDatagram(outputPacket, socket->pendingDatagramSize(), &sender, &senderPort);

    /// ADD PACKET TO FIFO 
    for (unsigned int i=0;i<recBytes;i++) fifo.enqueue(outputPacket[i]);

    //cout << fifo.size() << endl;
}
