#include "modbuslistener.h"
#include "ui_modbuslistener.h"
#include "settingsdialog.h"

#include <QModbusRtuSerialMaster>
#include <QStandardItemModel>
#include <QStatusBar>

#include <QDebug>
#include <QPointer>
#include <QtEndian>

const int temperature_sensor_address = 1;
const int electrical_sensor_address = 2;

enum ModbusConnection {
    Serial
};

ModbusListener::ModbusListener(QWidget *parent)
    : QMainWindow(parent),
      modbusDevice(nullptr),
      lastRequest(nullptr)
{
    slaveNumber = 0;
    timer = new QTimer();
    timer->setInterval(500);
    connect( timer, &QTimer::timeout, this, &ModbusListener::on_readButton_clicked );

    m_settingsDialog = new SettingsDialog(this);
    on_connectType_currentIndexChanged(0);

    strList = new QStringList();

}

ModbusListener::~ModbusListener()
{
    if (modbusDevice)
        modbusDevice->disconnectDevice();
    delete modbusDevice;
}

void ModbusListener::on_connectType_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    if (modbusDevice) {
        modbusDevice->disconnectDevice();
        delete modbusDevice;
        modbusDevice = nullptr;
    }

    modbusDevice = new QModbusRtuSerialMaster(this);

    connect(modbusDevice, &QModbusClient::errorOccurred, [this](QModbusDevice::Error) {
        qDebug() << modbusDevice->errorString(); } );

    if (!modbusDevice)
    {
        qDebug() << tr("Could not create Modbus master.");
    } else {
        connect(modbusDevice, &QModbusClient::stateChanged,
                this, &ModbusListener::onStateChanged);
    }
}

void ModbusListener::on_connectButton_clicked()
{
    if (!modbusDevice)
        return;

    if (modbusDevice->state() != QModbusDevice::ConnectedState)
    {
        modbusDevice->setConnectionParameter(QModbusDevice::SerialPortNameParameter,
            portName );

        modbusDevice->setConnectionParameter(QModbusDevice::SerialParityParameter,
            m_settingsDialog->settings().parity);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialBaudRateParameter,
            m_settingsDialog->settings().baud);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialDataBitsParameter,
            m_settingsDialog->settings().dataBits);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialStopBitsParameter,
            m_settingsDialog->settings().stopBits);

        modbusDevice->setTimeout(m_settingsDialog->settings().responseTime);
        modbusDevice->setNumberOfRetries(m_settingsDialog->settings().numberOfRetries);
        if (!modbusDevice->connectDevice())
        {
            qDebug() << tr("Connect failed: ") << modbusDevice->errorString();
        } else {
            timer->start();
        }
    } else {
        modbusDevice->disconnectDevice();
    }
}

void ModbusListener::onStateChanged(int state)
{
    m_connected = (state != QModbusDevice::UnconnectedState);
    emit isConnected(m_connected);
}

void ModbusListener::on_readButton_clicked()
{
    if (!modbusDevice)
        return;

    if(!m_connected)
        return;

    for ( int slave_id = temperature_sensor_address; slave_id <= electrical_sensor_address; slave_id++ )
    {
        QModbusDataUnit *unit = new QModbusDataUnit();
        if ( slave_id == temperature_sensor_address ) {
            unit->setRegisterType( QModbusDataUnit::InputRegisters );
            unit->setStartAddress(0);
            unit->setValueCount( 4 );
        } else if ( slave_id == electrical_sensor_address ) {
            unit->setRegisterType( QModbusDataUnit::HoldingRegisters );
            unit->setStartAddress(0);
            unit->setValueCount( 50 );

        }

        if (auto *reply = modbusDevice->sendReadRequest(*unit, slave_id ) ) {
            if (!reply->isFinished())
                connect(reply, &QModbusReply::finished, this, &ModbusListener::readReady);
            else
                delete reply; // broadcast replies return immediately
        } else {
            qDebug() << tr("Read error: ") << modbusDevice->errorString();
        }
    }
}

void ModbusListener::readReady()
{
    auto reply = qobject_cast<QModbusReply *>(sender());
    if (!reply)
        return;

    if (reply->error() == QModbusDevice::NoError)
    {
        const QModbusDataUnit unit = reply->result();
        strList->clear();
        if ( reply->serverAddress() == temperature_sensor_address ) {
            uint32_t byte[4];
            byte[0] = (uint32_t)( (unit.value(2)) << 48 );
            byte[1] = (uint32_t)( (unit.value(3)) << 32 );
            byte[2] = (uint32_t)( (unit.value(0)) << 16 );
            byte[3] = (uint32_t)( (unit.value(1)) );

            uint32_t temp;
            temp = byte[0] + byte[1] + byte[2] + byte[3];
            temperature = *reinterpret_cast<float*>(&temp);
            emit getTemperature();
        } else if ( reply->serverAddress() == electrical_sensor_address ) {
            for (uint i = 0; i < unit.valueCount(); i++)
            {
                const QString entry = tr("%1").arg(QString::number(unit.value(i), 10 ));
                strList->append( entry );

                if ( i == 34 )
                    voltagePhaseA = entry.toDouble() / 10;
                if ( i == 35)
                    voltagePhaseB = entry.toDouble() / 10;
                if ( i == 36)
                    voltagePhaseC = entry.toDouble() / 10;
                if ( i == 37){}
                    //Linear voltage
                    //voltagePhaseC = entry.toDouble() / 10;
                if ( i == 38){}
                    //Linear voltage
                    //voltagePhaseC = entry.toDouble() / 10;
                if ( i == 39){}
                    //Linear voltage
                    //voltagePhaseC = entry.toDouble() / 10;


                if ( i == 40 )
                    currentPhaseA = entry.toDouble() * 8 / 1000;
                if ( i == 41 )
                    currentPhaseB = entry.toDouble() * 8 / 1000;
                if ( i == 42 )
                    currentPhaseC = entry.toDouble() * 8 / 1000;



                if ( i == 43 )
                    frequency = entry.toDouble() / 100;

                if ( i == 47 )
                {
                    //cos fi
//                    frequency = entry.toDouble() / 100;
                }


            }
            emit getReply();
        }
    } else if (reply->error() == QModbusDevice::ProtocolError) {
        qDebug() << tr("Read response error: ") << reply->errorString()
                 << "Modbus exception:" << reply->rawResult().exceptionCode();
    } else {
        qDebug() << tr("Read response error: ") << reply->errorString() <<
                    "code: " << reply->error();
    }

    reply->deleteLater();
}

//QModbusDataUnit ModbusListener::readRequest() const
//{
//    auto table = QModbusDataUnit::HoldingRegisters;
//    int startAddress = 0;
//    int numberOfEntries = 50;

//    if ( slaveNumber == 1 ) {
//        table = QModbusDataUnit::InputRegisters;
//        numberOfEntries = 4;
//    }
//    return QModbusDataUnit(table, startAddress, numberOfEntries);
//}

void ModbusListener::updateSlaveNumber( int number )
{
    slaveNumber = number;
}

bool ModbusListener::isModbusConnected()
{
    return (m_connected != QModbusDevice::UnconnectedState);
}
