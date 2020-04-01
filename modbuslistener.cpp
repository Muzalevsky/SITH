#include "modbuslistener.h"
#include "ui_modbuslistener.h"
#include "settingsdialog.h"

#include <QModbusRtuSerialMaster>
//#include <QStandardItemModel>
//#include <QStatusBar>

#include <QDebug>
//#include <QPointer>
//#include <QtEndian>

ModbusListener::ModbusListener(QObject *parent)
    : QObject(parent),
      modbusDevice(nullptr),
      _alive(false),
      lastRequest(nullptr)
{
    slaveNumber = 0;
    timer = new QTimer();
    timer->setInterval(270);
    connect( timer, &QTimer::timeout, this, &ModbusListener::on_readButton_clicked );

    m_settingsDialog = new SettingsDialog();

    modbusDevice = new QModbusRtuSerialMaster(this);

    connect(modbusDevice, &QModbusClient::errorOccurred,
            [this](QModbusDevice::Error) { qDebug() << modbusDevice->errorString(); } );

    connect(modbusDevice, &QModbusClient::stateChanged,
            this, &ModbusListener::onStateChanged);

    if (!modbusDevice)
        qDebug() << tr("Could not create Modbus master.");
}

ModbusListener::~ModbusListener()
{
    if (modbusDevice)
        modbusDevice->disconnectDevice();
    delete modbusDevice;
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
        if ( reply->serverAddress() == temperature_sensor_address )
        {
            uint32_t byte[4];
            byte[0] = (uint32_t)( (unit.value(2)) << 48 );
            byte[1] = (uint32_t)( (unit.value(3)) << 32 );
            byte[2] = (uint32_t)( (unit.value(0)) << 16 );
            byte[3] = (uint32_t)( (unit.value(1)) );

            uint32_t temp;
            temp = byte[0] + byte[1] + byte[2] + byte[3];
            temperature = *reinterpret_cast<float*>(&temp);
            _alive = true;
            emit getTemperature();
        }
        else if ( reply->serverAddress() == electrical_sensor_address )
        {
            for (uint i = 0; i < unit.valueCount(); i++)
            {
                const QString entry = tr("%1").arg(QString::number(unit.value(i), 10 ));
                switch (i)
                {
                    case 0x22:
                        voltagePhaseA = entry.toDouble() / 10;
                    break;

                    case 0x23:
                        voltagePhaseB = entry.toDouble() / 10;
                    break;

                    case 0x24:
                        voltagePhaseC = entry.toDouble() / 10;
                    break;

                    /*
                     * According to documentation from 2020,
                     * it is real phase voltage
                     */
                    case 0x25:
                        //voltagePhaseA = entry.toDouble() / 10;
                    break;

                    case 0x26:
                        //voltagePhaseB = entry.toDouble() / 10;
                    break;

                    case 0x27:
                        //voltagePhaseC = entry.toDouble() / 10;
                    break;

                    case 0x28:
                        currentPhaseA = entry.toDouble() * 8 / 1000;
                    break;

                    case 0x29:
                        currentPhaseB = entry.toDouble() * 8 / 1000;
                    break;

                    case 0x2A:
                        currentPhaseC = entry.toDouble() * 8 / 1000;
                    break;

                    case 0x2B:
                        frequency = entry.toDouble() / 100;
                    break;

                    case 0x2C:
                        power = entry.toDouble();
                    break;

                    default:
                    break;
                }

                _alive = true;
                emit getReply();
            }
        }
    }
    else if (reply->error() == QModbusDevice::ProtocolError)
    {
        _alive = false;
        qDebug() << tr("Read response error: ") << reply->errorString()
                 << "Modbus exception:" << reply->rawResult().exceptionCode();
    } else {
        _alive = false;
        qDebug() << tr("Read response error: ") << reply->errorString() <<
                    "code: " << reply->error();
        reconnectModbus();
    }

    reply->deleteLater();
}

void ModbusListener::updateSlaveNumber( int number )
{
    slaveNumber = number;
}

bool ModbusListener::isModbusConnected()
{
    return (m_connected != QModbusDevice::UnconnectedState);
}

bool ModbusListener::isModbusAlive()
{
    return _alive;
}

void ModbusListener::reconnectModbus()
{
    if (!modbusDevice)
        return;

    modbusDevice->disconnectDevice();

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

    if (!modbusDevice->connectDevice()) {
        qDebug() << tr("Connect failed: ") << modbusDevice->errorString();
    } else {
        timer->start();
    }

}
