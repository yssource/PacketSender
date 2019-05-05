#include "udpflooding.h"
#include "ui_udpflooding.h"
#include <QNetworkInterface>
#include <QHostAddress>
#include <QAbstractSocket>
#include <QList>
#include <QDebug>
#include <QNetworkAddressEntry>
#include "packetnetwork.h"
#include "globals.h"
#include "packet.h"


UDPFlooding::UDPFlooding(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UDPFlooding)
{
    ui->setupUi(this);


    setWindowTitle("UDP Traffic Flooding");

    setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    thread = new ThreadSender(this);
    thread->ip = "192.168.1.1";
    thread->port = 5000;
    thread->delay = 0;
    thread->ascii = "Hello World";

    ui->ipEdit->setText(thread->ip);
    ui->portEdit->setText(QString::number(thread->port));
    ui->delayEdit->setText(QString::number(thread->delay));
    ui->asciiEdit->setText(thread->ascii);

    thread->issending = false;
    thread->stopsending = false;


    ui->startButton->setDisabled(true);
    ui->stopButton->setDisabled(true);

    //logging
    refreshTimer.setInterval(500);
    connect(&refreshTimer, SIGNAL(timeout()), this, SLOT(refreshTimerTimeout())) ;
    refreshTimer.start();



}

UDPFlooding::~UDPFlooding()
{
    if(thread->issending) {
        thread->terminate();
    }
    delete ui;
}

void UDPFlooding::on_startButton_clicked()
{
    QDEBUG();

    bool ok1, ok2;

    ui->portEdit->text().toUInt(&ok1);
    ui->delayEdit->text().toUInt(&ok2);


    if(!ok1) {
        ui->portEdit->setText("Invalid");
        return;
    }

    if(!ok2) {
        ui->delayEdit->setText("Invalid");
        return;
    }

    thread->ascii = ui->asciiEdit->text();
    thread->ip = ui->ipEdit->text();
    thread->port = static_cast<quint16>(ui->portEdit->text().toUInt(&ok1));
    thread->delay = ui->delayEdit->text().toUInt(&ok2);

    // Do it.
    thread->start();


}



void UDPFlooding::on_stopButton_clicked()
{
    QDEBUG() << "Flagging stop send";

    thread->stopsending = true;
}

void UDPFlooding::refreshTimerTimeout()
{


    if(!thread->issending) {
        ui->startButton->setDisabled(false);
        ui->stopButton->setDisabled(true);

    } else {
        ui->startButton->setDisabled(true);
        ui->stopButton->setDisabled(false);

        QString str = "";
        QTextStream out(&str);

        out << "UDP " << "(" << thread->sourcePort << ") ://" << thread->ip <<":"<< thread->port << "\r\n";
        out << "Total Sent: " << thread->packetssent << " packets \r\n";
        out << "Run time: " << thread->getElapsedMS() << " ms \r\n";

        double actualRate = thread->getRatekHz(thread->elapsedTimer, thread->packetssent);
        QString rateStr = QString::number(actualRate, 'f', 4);

        out << "Send Rate: " << rateStr << " kHz \r\n";
        out << "Send kbps: " << (actualRate * thread->hex.size()*8) << " kbps \r\n";

        ui->statsLabel->setText(str);

    }

}


ThreadSender::ThreadSender(QObject *parent) : QThread(parent)
{
    QDEBUG();
}

ThreadSender::~ThreadSender()
{
    stopsending = true;
}

qint64 ThreadSender::getElapsedMS()
{
    return elapsedTimer.elapsed();
}



double ThreadSender::getRatekHz(QElapsedTimer eTimer, quint64 pkts)
{

    auto ms = static_cast<double>(eTimer.elapsed());
    auto packetsentDouble = static_cast<double>(pkts);
    return packetsentDouble / ms;
}



void ThreadSender::run()
{
    QDEBUG() << "Begin send";

    QHostAddress addy(ip);

    QUdpSocket * socket = new QUdpSocket();
    socket->bind(0);

    sourcePort = socket->localPort();

    packetssent = 0;

    QHostAddress resolved = PacketNetwork::resolveDNS(ip);
    QString data = Packet::ASCIITohex(ascii);
    hex = Packet::HEXtoByteArray(data);


    issending = true;
    stopsending = false;

    msleep(10); //momentarily break thread

    //return;

    elapsedTimer.start();
    while(!stopsending) {
        //intense send
        socket->writeDatagram(hex, resolved, port);
        packetssent++;

        if(delay > 0) {
          msleep(delay);
        } else {
            if((packetssent % 1000) == 0) {
                usleep(1);
            }
        }
    }

    msleep(1);
    socket->close();
    msleep(1);
    socket->deleteLater();
    msleep(1);


    stopsending = false;
    issending = false;

    QDEBUG() << "End send";

}