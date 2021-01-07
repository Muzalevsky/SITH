#include "port.h"
#include <QDebug>
#include <QMessageBox>

#include <QThread>

#define RECONNECTION_RETRIES_NUMBER 20
Port::Port(QObject *parent) :
    QObject(parent),
    portMode(0),
    reconnect_counter(RECONNECTION_RETRIES_NUMBER)
{
    /*
     * see https://doc-snapshots.qt.io/qt5-5.10/qserialport.html
     *
     * This option is useful if the data is only read at certain points in time
     *  (for instance in a real-time streaming application) or if the serial
     * port should be protected against receiving too much data, which
     * may eventually cause the application to run out of memory.
     */
    thisPort.setReadBufferSize(1000);
}

Port::~Port()
{
    emit finished_Port();
}

void Port::process_Port()
{
    qDebug() << "Serial port" << this
             << "in thread" << this->thread()
             << "parent" << parent();

    //qRegisterMetaType<QSerialPort::SerialPortError>("QSerialPort::SerialPortError");

    connect(&thisPort, &QSerialPort::errorOccurred, this, &Port::handleError);
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

void Port::setPortOpenMode(QIODevice::OpenModeFlag flag)
{
    portMode = flag;
}

void Port::openPort()
{
    thisPort.setPortName(SettingsPort.name);
    qDebug() << "Opening " << thisPort.portName();

    if (thisPort.open(QIODevice::ReadWrite))
    {
        if ( thisPort.setBaudRate(SettingsPort.baudRate) &&
             thisPort.setDataBits(SettingsPort.dataBits) &&
             thisPort.setParity(SettingsPort.parity) &&
             thisPort.setStopBits(SettingsPort.stopBits) &&
             thisPort.setFlowControl(SettingsPort.flowControl) )
        {
            if ( thisPort.isOpen() )
            {
                qDebug() << SettingsPort.name + " >> Open!";
                thisPort.clear();
                emit connectionStateChanged(true);
                reconnect_counter = RECONNECTION_RETRIES_NUMBER;
            }
        }
        else
        {
            thisPort.close();
            qDebug() << thisPort.errorString();
        }
    }
    else
    {
        qDebug() << thisPort.errorString();
    }
}

// TODO Check how it is working during deleting now
void Port::handleError(QSerialPort::SerialPortError error)
{
    if ( error != QSerialPort::NoError )
        qDebug() << thisPort.portName() << "Error:" <<  thisPort.errorString();

    switch ( error )
    {
        /*
         * An error occurred while attempting to open an non-existing device.
         */
        case QSerialPort::DeviceNotFoundError:
            thisPort.close();
            emit connectionStateChanged(false);
        break;

        /*
         * An I/O error occurred when a resource becomes unavailable, e.g.
         * when the device is unexpectedly removed from the system.
         */
        case QSerialPort::ResourceError:
            thisPort.close();
            emit connectionStateChanged(false);
        break;

        case QSerialPort::TimeoutError:
            emit connectionStateChanged(false);
            if ( (thisPort.isOpen()) ) {
                reconnectPort();
            }
        break;

        default:
        break;
    }
}

void Port::closePort()
{
    if ( thisPort.isOpen() ) {
        thisPort.clear( QSerialPort::AllDirections );
        thisPort.close();
        qDebug() << SettingsPort.name << " >> Close!";
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
    if ( thisPort.isOpen() )
    {
        QByteArray data;
        data.append(thisPort.readAll());
        emit outPortByteArray(data);
        emit outPortString(data);
    }
}

void Port::connect_clicked()
{
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
    if ( reconnect_counter > 0 )
    {
        closePort();
        openPort();
        reconnect_counter--;
    }
}
