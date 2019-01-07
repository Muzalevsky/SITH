#include "modbuslistener.h"
#include "ui_modbuslistener.h"
#include "settingsdialog.h"

#include <QModbusRtuSerialMaster>
#include <QStandardItemModel>
#include <QStatusBar>

#include <QDebug>
#include <QPointer>

enum ModbusConnection {
    Serial
};

ModbusListener::ModbusListener(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ModbusListener)
    , lastRequest(nullptr)
    , modbusDevice(nullptr)
{

    timer = new QTimer();
    timer->setInterval(1000);
    connect( timer, &QTimer::timeout, this, &ModbusListener::on_readButton_clicked );

    ui->setupUi(this);
    m_settingsDialog = new SettingsDialog(this);
    initActions();
    ui->connectType->setCurrentIndex(0);
    on_connectType_currentIndexChanged(0);

    strList = new QStringList();

}

ModbusListener::~ModbusListener()
{
    if (modbusDevice)
        modbusDevice->disconnectDevice();
    delete modbusDevice;
    delete ui;
}

void ModbusListener::initActions()
{
    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionExit->setEnabled(true);
    ui->actionOptions->setEnabled(true);

    connect(ui->actionConnect, &QAction::triggered,
            this, &ModbusListener::on_connectButton_clicked);
    connect(ui->actionDisconnect, &QAction::triggered,
            this, &ModbusListener::on_connectButton_clicked);

    connect(ui->actionExit, &QAction::triggered, this, &QMainWindow::close);
    connect(ui->actionOptions, &QAction::triggered, m_settingsDialog, &QDialog::show);


    connect(ui->readButton, &QPushButton::clicked, this, ModbusListener::on_readButton_clicked);
    connect(ui->optionsButton, &QPushButton::clicked, m_settingsDialog, &QDialog::show);
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
        statusBar()->showMessage(modbusDevice->errorString(), 5000);
    });

    if (!modbusDevice)
    {
        ui->connectButton->setDisabled(true);
        statusBar()->showMessage(tr("Could not create Modbus master."), 5000);
    } else {
        connect(modbusDevice, &QModbusClient::stateChanged,
                this, &ModbusListener::onStateChanged);
    }
}

void ModbusListener::on_connectButton_clicked()
{
    if (!modbusDevice)
        return;

    statusBar()->clearMessage();
    if (modbusDevice->state() != QModbusDevice::ConnectedState)
    {
        modbusDevice->setConnectionParameter(QModbusDevice::SerialPortNameParameter,
            ui->portEdit->text());
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
            statusBar()->showMessage(tr("Connect failed: ") + modbusDevice->errorString(), 5000);
        } else {
            ui->actionConnect->setEnabled(false);
            ui->actionDisconnect->setEnabled(true);
            timer->start();
        }
    } else {
        modbusDevice->disconnectDevice();
        ui->actionConnect->setEnabled(true);
        ui->actionDisconnect->setEnabled(false);
    }
}

void ModbusListener::onStateChanged(int state)
{
    bool connected = (state != QModbusDevice::UnconnectedState);
    ui->actionConnect->setEnabled(!connected);
    ui->actionDisconnect->setEnabled(connected);

    if (state == QModbusDevice::UnconnectedState)
        ui->connectButton->setText(tr("Connect"));
    else if (state == QModbusDevice::ConnectedState)
        ui->connectButton->setText(tr("Disconnect"));
}

void ModbusListener::on_readButton_clicked()
{
    if (!modbusDevice)
        return;
    ui->readValue->clear();
    statusBar()->clearMessage();

    if (auto *reply = modbusDevice->sendReadRequest(readRequest(), ui->serverEdit->value())) {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &ModbusListener::readReady);
        else
            delete reply; // broadcast replies return immediately
    } else {
        statusBar()->showMessage(tr("Read error: ") + modbusDevice->errorString(), 5000);
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
        for (uint i = 0; i < unit.valueCount(); i++)
        {
            const QString entry = tr("%1").arg(QString::number(unit.value(i), 10 ));
            ui->readValue->addItem(entry);
            strList->append( entry );
            if ( i == 34)
                voltagePhaseA = entry.toDouble() / 10;
            if ( i == 35)
                voltagePhaseB = entry.toDouble() / 10;
            if ( i == 36)
                voltagePhaseC = entry.toDouble() / 10;
            if ( i == 40 )
                currentPhaseA = entry.toDouble() / 1000;
            if ( i == 43)
                frequency = entry.toDouble() / 100;



        }
        emit getReply();

    } else if (reply->error() == QModbusDevice::ProtocolError) {
        statusBar()->showMessage(tr("Read response error: %1 (Mobus exception: 0x%2)").
                                    arg(reply->errorString()).
                                    arg(reply->rawResult().exceptionCode(), -1, 16), 5000);
    } else {
        statusBar()->showMessage(tr("Read response error: %1 (code: 0x%2)").
                                    arg(reply->errorString()).
                                    arg(reply->error(), -1, 16), 5000);
    }

    reply->deleteLater();
}

QModbusDataUnit ModbusListener::readRequest() const
{
    const auto table = QModbusDataUnit::HoldingRegisters;
    int startAddress = 0;
    int numberOfEntries = 50;
    return QModbusDataUnit(table, startAddress, numberOfEntries);
}
