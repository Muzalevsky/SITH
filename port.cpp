#include "port.h"
#include <QDebug>
#include <QMessageBox>

#include <QThread>

Port::Port(QObject *parent) :
    QObject(parent),
    portMode(0)
{
}

Port::~Port()
{
    emit finished_Port();
}

//Q_DECLARE_METATYPE(QSerialPort::SerialPortError);
void Port::process_Port()
{

    qDebug() << "New serial port" << this << "in thread" << this->thread();
    qDebug() << "Port's parent is" << parent();


    qRegisterMetaType<QSerialPort::SerialPortError>("QSerialPort::SerialPortError");

    connect(&thisPort, &QSerialPort::errorOccurred, this, &Port::handleError);
    connect(&thisPort, SIGNAL(readyRead()),this,SLOT(ReadInPort()));
    connect(this, SIGNAL(error_(QString)), this, SLOT(errorHandler(QString)));
}

void Port::setPortSettings(QString name, int baudrate,int DataBits,
                         int Parity,int StopBits, int FlowControl)
{
    SettingsPort.name = name;
    SettingsPort.baudRate = (QSerialPort::BaudRate) baudrate;
    SettingsPort.dataBits = (QSerialPort::DataBits) DataBits;
    SettingsPort.parity = (QSerialPort::Parity) Parity;
    SettingsPort.stopBits = (QSerialPort::StopBits) StopBits;
    SettingsPort.flowControl = (QSerialPort::FlowControl) FlowControl;
}

void Port::setPortOpenMode(QIODevice::OpenModeFlag flag)
{
    portMode = flag;
}

void Port::openPort()
{
    thisPort.setPortName(SettingsPort.name);
    qDebug() << "Opening " << thisPort.portName();
    if (thisPort.open(QIODevice::ReadWrite)) {
        if ( thisPort.setBaudRate(SettingsPort.baudRate) &&
             thisPort.setDataBits(SettingsPort.dataBits) &&
             thisPort.setParity(SettingsPort.parity) &&
             thisPort.setStopBits(SettingsPort.stopBits) &&
             thisPort.setFlowControl(SettingsPort.flowControl) ) {
            if ( thisPort.isOpen() ) {
//                error_(SettingsPort.name + " >> Открыт!");
                qDebug() << thisPort.portName() << " serial port opened";
                emit connectionStateChanged(true);
            }
        } else {
            thisPort.close();
            error_(thisPort.errorString());
        }
    } else {
        thisPort.close();
        error_(thisPort.errorString());
    }
}

void Port::handleError(QSerialPort::SerialPortError error)
{
    if ( (thisPort.isOpen()) && (error == QSerialPort::ResourceError) ) {
        error_(thisPort.errorString());
        closePort();
    }
}

void Port::closePort()
{
    if ( thisPort.isOpen() ) {
        thisPort.close();
        error_(SettingsPort.name + " >> Закрыт!\r");
        emit connectionStateChanged(false);
    }
}

void Port::WriteToPort(QByteArray data)
{
    if ( thisPort.isOpen() ) {
        thisPort.write(data);
    }
}

void Port::ReadInPort()
{
    QByteArray data;
    data.append(thisPort.readAll());
    emit outPortByteArray(data);
    emit outPort(data);
}

//void Port::readyReadSlot()
//{
//    while (!thisPort.atEnd()) {
//        QByteArray data = thisPort.readAll();
//        qDebug() << "data in Port" << data;
//    }
//}

void Port::errorHandler( QString err )
{
    qDebug() << err;
}

void Port::connect_clicked()
{
//    if (thisPort != nullptr )
//        return;

    if ( thisPort.isOpen() ) {
        closePort();
    } else {
        openPort();
    }
}

bool Port::isOpened()
{
    return thisPort.isOpen();
}

void Port::reconnectPort()
{
    closePort();
    openPort();
}
