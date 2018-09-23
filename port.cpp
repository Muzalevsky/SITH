#include "port.h"
#include <QDebug>

Port::Port(QObject *parent) :
    QObject(parent)
{
    qDebug() << "new port" << this;
//    sw = new SettingsWindow( force_serial, this);
//    connect( sw->setButton, &QPushButton::clicked, this, &Port::openPort );
//    sw->show();

    force_str = new QString();
}

Port::~Port()
{
    emit finished_Port();
}

void Port::process_Port()
{
    connect(&thisPort,SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(handleError(QSerialPort::SerialPortError)));
    connect(&thisPort, SIGNAL(readyRead()),this,SLOT(ReadInPort()));
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

void Port::openPort()
{
    thisPort.setPortName(SettingsPort.name);
    if (thisPort.open(QIODevice::ReadWrite)) {
        if ( thisPort.setBaudRate(SettingsPort.baudRate) &&
             thisPort.setDataBits(SettingsPort.dataBits) &&
             thisPort.setParity(SettingsPort.parity) &&
             thisPort.setStopBits(SettingsPort.stopBits) &&
             thisPort.setFlowControl(SettingsPort.flowControl) )
        {
            if ( thisPort.isOpen() )
            {
                error_((SettingsPort.name+ " >> Открыт!\r").toLocal8Bit());
            }
            qDebug() << "open port";

        }
        else
        {
            thisPort.close();
            error_(thisPort.errorString().toLocal8Bit());
        }
    }
    else
    {
        thisPort.close();
        error_(thisPort.errorString().toLocal8Bit());
    }
}

void Port::handleError(QSerialPort::SerialPortError error)
{
    if ( (thisPort.isOpen()) && (error == QSerialPort::ResourceError) )
    {
        error_(thisPort.errorString().toLocal8Bit());
        closePort();
    }
}


void Port::closePort()
{
    if( thisPort.isOpen() )
    {
        thisPort.close();
        error_(SettingsPort.name.toLocal8Bit() + " >> Закрыт!\r");
    }
}

void Port::WriteToPort(QByteArray data)
{
    if( thisPort.isOpen() )
    {
        thisPort.write(data);
    }
}

void Port::ReadInPort()
{
    QByteArray data;
    data.append(thisPort.readAll());
    outPort(data);

}

