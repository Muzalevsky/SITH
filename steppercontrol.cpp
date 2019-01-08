#include "steppercontrol.h"

#include <cstdlib>

#include <QDebug>
#include <QTimer>

StepperControl::StepperControl( Port* ext_port, QWidget* parent ) : QMainWindow( parent ),
    step_number(0),
    default_Ver(0x02),
    speed_limit(0),
    abs_position(0)
{
    port = ext_port;
    gridLayout      = new QGridLayout();
    speedEdit       = new QLineEdit();
    stepNumberEdit  = new QLineEdit();
    stepSizeEdit    = new QLineEdit();

    initBtn = new QPushButton("Init", this);
    connect( initBtn, &QPushButton::clicked, this, &StepperControl::slotInit );

    sendBtn = new QPushButton("Send", this);
    connect( sendBtn, &QPushButton::clicked, this, &StepperControl::slotSend );

    PortNameBox = new QComboBox();
    m_settingsDialog = new SettingsDialog(this);

    timer = new QTimer();
    timer->setInterval(250);
    connect( timer, &QTimer::timeout, this, &StepperControl::getAbsPos );


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

    connect(port, SIGNAL(outPortByteArray(QByteArray)), this, SLOT(getResponse(QByteArray)));
    connect(this, &StepperControl::writeCmdToPort, port, &Port::WriteToPort);
    connect(speedEdit, SIGNAL( textChanged(QString) ), this, SLOT( updateSpeedLimit(QString)));
    connect(stepNumberEdit, SIGNAL( textChanged(QString) ), this, SLOT( updateStepNumber(QString)));

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
//    LAN_COMMAND_Type *cmd;
//    cmd = (LAN_COMMAND_Type *)calloc( 1, sizeof( LAN_COMMAND_Type* ) );

//    cmd->Ver = default_Ver;
//    cmd->CMD_TYPE = CODE_CMD_REQUEST;
//    cmd->LENGTH_DATA = 0x0008;
////    cmd->DATA = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

//    cmd->XOR_SUM=0x00;
//    cmd->XOR_SUM=xor_sum((uint8_t*)&cmd->XOR_SUM, sizeof(cmd));

}

void StepperControl::getResponse( QByteArray arr )
{
    LAN_COMMAND_Type *cmd = deserialize( arr );
    qDebug() << "getResponse";

    if ( cmd->CMD_TYPE == CODE_CMD_RESPONSE ) {
        if ( cmd->CMD_IDENTIFICATION == CMD_PowerSTEP01_GET_ABS_POS ) {
            abs_position = ((COMMANDS_RETURN_DATA_Type*)cmd->DATA )->RETURN_DATA;
            emit updatePos( QString::number( abs_position ) );
        }
        if ( cmd->CMD_IDENTIFICATION == CMD_PowerSTEP01_MOVE_F ) {
            qDebug() << "Move F Status: " << ((COMMANDS_RETURN_DATA_Type*)cmd->DATA )->ERROR_OR_COMMAND;
        }
        if ( cmd->CMD_IDENTIFICATION == CMD_PowerSTEP01_MOVE_R ) {
            qDebug() << "Move R Status: " << ((COMMANDS_RETURN_DATA_Type*)cmd->DATA )->ERROR_OR_COMMAND;
        }
    }
}

void StepperControl::moveMotor( uint32_t step_number, bool reverse )
{
    LAN_COMMAND_Type *cmd;
    cmd = (LAN_COMMAND_Type *)calloc( 1, sizeof( LAN_COMMAND_Type* ) );
    cmd->Ver = default_Ver;
    cmd->CMD_TYPE = CODE_CMD_POWERSTEP01;
    cmd->LENGTH_DATA = sizeof(SMSD_CMD_Type);
    cmd->XOR_SUM=xor_sum((uint8_t*)&cmd->XOR_SUM, sizeof(cmd));

    SMSD_CMD_Type *inner_cmd;
    inner_cmd = (SMSD_CMD_Type *)calloc( 1, sizeof( SMSD_CMD_Type* ) );
    inner_cmd->DATA = step_number;

    if ( step_number ) {
        if ( reverse ) {
            inner_cmd->COMMAND = CMD_PowerSTEP01_MOVE_R;
            cmd->CMD_IDENTIFICATION = CMD_PowerSTEP01_MOVE_R;
        } else {
            inner_cmd->COMMAND = CMD_PowerSTEP01_MOVE_F;
            cmd->CMD_IDENTIFICATION = CMD_PowerSTEP01_MOVE_F;
        }
    } else {
        inner_cmd->COMMAND = CMD_PowerSTEP01_GO_ZERO;
        cmd->CMD_IDENTIFICATION = CMD_PowerSTEP01_GO_ZERO;
    }

    cmd->DATA = (uint8_t*)inner_cmd;
    QByteArray arr = serialize( cmd );
    emit writeCmdToPort( arr );

}

void StepperControl::setZero()
{
    LAN_COMMAND_Type *cmd;
    cmd = (LAN_COMMAND_Type *)calloc( 1, sizeof( LAN_COMMAND_Type* ) );
    cmd->Ver = default_Ver;
    cmd->CMD_TYPE = CODE_CMD_POWERSTEP01;
    cmd->LENGTH_DATA = sizeof(SMSD_CMD_Type);
    cmd->CMD_IDENTIFICATION = CMD_PowerSTEP01_RESET_POS;
    cmd->XOR_SUM=xor_sum((uint8_t*)&cmd->XOR_SUM, sizeof(cmd));

    SMSD_CMD_Type *inner_cmd;
    inner_cmd = (SMSD_CMD_Type *)calloc( 1, sizeof( SMSD_CMD_Type* ) );
    inner_cmd->COMMAND = CMD_PowerSTEP01_RESET_POS;

    cmd->DATA = (uint8_t*)inner_cmd;
    QByteArray arr = serialize( cmd );
    emit writeCmdToPort( arr );

}

void StepperControl::setMaxSpeed( uint32_t speed_limit )
{
    LAN_COMMAND_Type *cmd;
    cmd = (LAN_COMMAND_Type *)calloc( 1, sizeof( LAN_COMMAND_Type* ) );
    cmd->Ver = default_Ver;
    cmd->CMD_TYPE = CODE_CMD_POWERSTEP01;
    cmd->LENGTH_DATA = sizeof(SMSD_CMD_Type);
    cmd->CMD_IDENTIFICATION = CMD_PowerSTEP01_SET_MAX_SPEED;
    cmd->XOR_SUM=xor_sum((uint8_t*)&cmd->XOR_SUM, sizeof(cmd));

    SMSD_CMD_Type *inner_cmd;
    inner_cmd = (SMSD_CMD_Type *)calloc( 1, sizeof( SMSD_CMD_Type* ) );
    inner_cmd->COMMAND = CMD_PowerSTEP01_SET_MAX_SPEED;
    inner_cmd->DATA = speed_limit;

    cmd->DATA = (uint8_t*)inner_cmd;
    QByteArray arr = serialize( cmd );
    emit writeCmdToPort( arr );
    qDebug() << "setMaxSpeed" << speed_limit;
}

void StepperControl::getAbsPos()
{
    LAN_COMMAND_Type *cmd;
    cmd = (LAN_COMMAND_Type *)calloc( 1, sizeof( LAN_COMMAND_Type* ) );
    cmd->Ver = default_Ver;
    cmd->CMD_TYPE = CODE_CMD_POWERSTEP01;
    cmd->LENGTH_DATA = sizeof(SMSD_CMD_Type);
    cmd->CMD_IDENTIFICATION = CMD_PowerSTEP01_GET_ABS_POS;
    cmd->XOR_SUM=xor_sum((uint8_t*)&cmd->XOR_SUM, sizeof(cmd));

    SMSD_CMD_Type *inner_cmd;
    inner_cmd = (SMSD_CMD_Type *)calloc( 1, sizeof( SMSD_CMD_Type* ) );
    inner_cmd->COMMAND = CMD_PowerSTEP01_GET_ABS_POS;

    cmd->DATA = (uint8_t*)inner_cmd;
    QByteArray arr = serialize( cmd );
    emit writeCmdToPort( arr );
    qDebug() << "getAbsPos";
}

QByteArray StepperControl::serialize( LAN_COMMAND_Type *cmd )
{
    QByteArray byteArray;

    QDataStream stream(&byteArray, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_10);

    stream << cmd->XOR_SUM
           << cmd->Ver
           << cmd->CMD_TYPE
           << cmd->CMD_IDENTIFICATION
           << cmd->LENGTH_DATA
           << cmd->DATA;

    return byteArray;
}

LAN_COMMAND_Type* StepperControl::deserialize(const QByteArray& byteArray)
{
    LAN_COMMAND_Type *cmd;
    cmd = (LAN_COMMAND_Type *)calloc( 1, sizeof( LAN_COMMAND_Type* ) );

    QDataStream stream(byteArray);
    stream.setVersion(QDataStream::Qt_5_10);

    stream >> cmd->XOR_SUM
           >> cmd->Ver
           >> cmd->CMD_TYPE
           >> cmd->CMD_IDENTIFICATION
           >> cmd->LENGTH_DATA
           >> *cmd->DATA;

    return cmd;
}

void StepperControl::stepForward()
{
    qDebug() << "step+";
    moveMotor( step_number, false );
}

void StepperControl::stepBackward()
{
    qDebug() << "step-";
    moveMotor( step_number, true );
}

void StepperControl::slotInit()
{
    setZero();
    setMaxSpeed( 10 );
    qDebug() << "init";
}

void StepperControl::slotSend()
{

}
