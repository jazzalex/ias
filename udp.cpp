#include <udp.h>

using namespace std;

/// CONSTRUCTOR
udp::udp(QObject *parent): QObject{parent}{

    /// PREPARATIONS
    inputPacket    = new char[1024];
    outputPacket   = new char[1024];
    recBytes = 0;

    /// CREATE UDP SOCKET AND MAKE IT LISTEN
    socket = new QUdpSocket(this);
    socket->bind(QHostAddress("0.0.0.0"),5401);

    /// CONNECT RECEIVER SOCKET TO SLOT FUNCTION
    connect(socket,SIGNAL(readyRead()),this,SLOT(readyRead()));
}

/// SENDER FUNCTION
void udp::sendInput(){
    socket->writeDatagram(inputPacket,1024,QHostAddress("144.76.81.210"),5401);
    //cout << "SENDING" << endl;
}

/// RECEIVER FUNCTION
void udp::readyRead(){
    QHostAddress sender;
    quint16 senderPort;

    /// FIGURE HOW MANY BYTES ARE AVAILABLE
    recBytes = socket->pendingDatagramSize();

    /// ACTUALLY READ FROM THE SOCKET
    socket->readDatagram(outputPacket, socket->pendingDatagramSize(), &sender, &senderPort);

    /// ADD RECEIVED SAMPLES TO FIFO JITTER BUFFER
    for (unsigned int i=0;i<recBytes;i++) fifo.enqueue(outputPacket[i]);

    cout << fifo.size() << endl;
}
