#include "steppercontrol.h"

#include <cstdlib>

#include <QDebug>
StepperControl::StepperControl( Port* ext_port, QWidget* parent ) : QMainWindow( parent )
{
    port = ext_port;
    gridLayout = new QGridLayout();
    speedEdit       = new QLineEdit();
    stepNumberEdit  = new QLineEdit();
    stepSizeEdit    = new QLineEdit();
    initBtn = new QPushButton("Init", this);
    sendBtn = new QPushButton("Send", this);
    PortNameBox = new QComboBox();

    m_settingsDialog = new SettingsDialog(this);

    gridLayout->addWidget(new QLabel(tr("Stepper Port: ")),1,1,1,1);
    gridLayout->addWidget(PortNameBox,1,2,1,1);

    gridLayout->addWidget(new QLabel(tr("Set speed: ")),2,1,1,1);
    gridLayout->addWidget(new QLabel(tr("Set step number: ")),3,1,1,1);
    gridLayout->addWidget(new QLabel(tr("Set step size (mm): ")),4,1,1,1);
    gridLayout->addWidget(speedEdit,2,2,1,1);
    gridLayout->addWidget(stepNumberEdit,3,2,1,1);
    gridLayout->addWidget(stepSizeEdit,4,2,1,1);

    gridLayout->addWidget(initBtn,5,1,1,1);
    gridLayout->addWidget(sendBtn,5,2,1,1);

    QPushButton *optionsButton = new QPushButton( "Options" );
    connect( optionsButton, &QPushButton::clicked, m_settingsDialog, &QDialog::show);
    gridLayout->addWidget( optionsButton, 6,1,1,1);

    setButton = new QPushButton( "Set" );
    connect( setButton, &QPushButton::clicked, this, &StepperControl::saveSettings );
    gridLayout->addWidget( setButton, 7,1,1,1);

    openButton = new QPushButton( "Open" );
    connect( openButton, &QPushButton::clicked, port, &Port::openPort );
    gridLayout->addWidget( openButton, 7,2,1,1);

    QPushButton *searchButton = new QPushButton( "Search" );
    connect( searchButton, &QPushButton::clicked, this, &StepperControl::searchPorts );
    gridLayout->addWidget( searchButton, 7,3,1,1);




    QWidget* mainWidget = new QWidget;
    mainWidget->setLayout( gridLayout );
    setCentralWidget( mainWidget );
    setWindowTitle( "Stepper settings" );
}

uint8_t StepperControl::xor_sum(uint8_t *data,uint16_t length)
{
    uint8_t xor_temp = 0xFF;
    while( length-- )
    {
        xor_temp += *data;
        data++;
    }
    return ( xor_temp ^ 0xFF );
}

void StepperControl::sendPassword()
{
    LAN_COMMAND_Type *cmd;
    cmd = (LAN_COMMAND_Type *)calloc( 1, sizeof( LAN_COMMAND_Type* ) );

    cmd->Ver = 0x02;
    cmd->CMD_TYPE = CODE_CMD_REQUEST;
    cmd->LENGTH_DATA = 0x0008;
//    cmd->DATA = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

    cmd->XOR_SUM=0x00;
    cmd->XOR_SUM=xor_sum((uint8_t*)&cmd->XOR_SUM, sizeof(cmd));

}

void StepperControl::request()
{

}

void StepperControl::response()
{

}

void StepperControl::powerStep01()
{

}

void StepperControl::searchPorts()
{
    PortNameBox->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        PortNameBox->addItem(info.portName());
    }
    qDebug() << "Stepper port search.";
}

void StepperControl::saveSettings()
{
    port->setPortSettings( PortNameBox->currentText(),
                           m_settingsDialog->settings().baud,
                           m_settingsDialog->settings().dataBits,
                           m_settingsDialog->settings().parity,
                           m_settingsDialog->settings().stopBits,
                           QSerialPort::NoFlowControl );

    qDebug() << "New stepper port settings saved.";
}
