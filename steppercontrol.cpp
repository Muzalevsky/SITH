#include "steppercontrol.h"

#include <cstdlib>

#include <QDebug>
#include <QTimer>

//step_number(66) = 0,4 мм на штоке
//step_number(82) = 0,5 мм на штоке

StepperControl::StepperControl( Port* ext_port, QWidget* parent ) : QMainWindow( parent ),
    step_number(82),
    default_Ver(0x02),
    speed_limit(1000),
    abs_position(0),
    passwd({ 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF } )
{
    port = ext_port;
    passwd_length = 8;
    gridLayout      = new QGridLayout();
    speedEdit       = new QLineEdit();
    stepNumberEdit  = new QLineEdit();
    stepSizeEdit    = new QLineEdit();
    PortNameBox = new QComboBox();
    m_settingsDialog = new SettingsDialog(this);

    receiveFlag = false;
    sendFlag = false;

    mutex = new QMutex();
//    timer = new QTimer();
//    timer->setInterval( 300 );
//    connect( timer, &QTimer::timeout, this, &StepperControl::processVector );

    initBtn = new QPushButton("Init", this);
    sendBtn = new QPushButton("Passwd", this);
    getPosBtn = new QPushButton("getPos", this);
    setSpdBtn = new QPushButton("setSpd", this);
    connect( initBtn, &QPushButton::clicked, this, &StepperControl::slotInit );
    connect( sendBtn, &QPushButton::clicked, this, &StepperControl::slotSend );
    connect( getPosBtn, &QPushButton::clicked, this, &StepperControl::slotGetPos );
    connect( setSpdBtn, &QPushButton::clicked, this, &StepperControl::slotSetSpeed );

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
    gridLayout->addWidget(getPosBtn,5,3,1,1);
    gridLayout->addWidget(setSpdBtn,5,4,1,1);

    QPushButton *optionsButton = new QPushButton( "Options" );
    connect( optionsButton, &QPushButton::clicked, m_settingsDialog, &QDialog::show);
    gridLayout->addWidget( optionsButton, 6,1,1,1);

    setButton = new QPushButton( "Set" );
    connect( setButton, &QPushButton::clicked, this, &StepperControl::saveSettings );
    gridLayout->addWidget( setButton, 7,1,1,1);
    openButton = new QPushButton( "Open" );
    connect( openButton, &QPushButton::clicked, port, &Port::openPort );
    gridLayout->addWidget( openButton, 7,2,1,1);
    closeButton = new QPushButton( "Close" );
    connect( closeButton, &QPushButton::clicked, port, &Port::closePort );
    gridLayout->addWidget( closeButton, 7,3,1,1);
    QPushButton *searchButton = new QPushButton( "Search" );
    connect( searchButton, &QPushButton::clicked, this, &StepperControl::searchPorts );
    gridLayout->addWidget( searchButton, 7,4,1,1);

    QWidget* mainWidget = new QWidget;
    mainWidget->setLayout( gridLayout );
    setCentralWidget( mainWidget );
    setWindowTitle( "Stepper settings" );

    connect(port, SIGNAL(outPortByteArray(QByteArray)), this, SLOT(getResponse(QByteArray)));
    connect(this, &StepperControl::writeCmdToPort, port, &Port::WriteToPort);
    connect(speedEdit, SIGNAL( textChanged(QString) ), this, SLOT( updateSpeedLimit(QString)));
    connect(stepNumberEdit, SIGNAL( textChanged(QString) ), this, SLOT( updateStepNumber(QString)));
    connect(this, &StepperControl::hasAnswer, this, &StepperControl::processVector );
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
//    uint8_t result =  (uint8_t)( (xor_temp) ^ 0xFF);
//    qDebug() << "xor_sum = " << result;
//    return result;

    return (xor_temp ^ 0xFF);
}

void StepperControl::sendPassword()
{
    // {B8}{01}{00}{7F}{08}{00}{01}{23}{45}{67}{89}{AB}{CD}{EF}
    request_message_t cmd;
    cmd.XOR_SUM = 0x00;
    cmd.Ver = 0x02;
    cmd.CMD_TYPE = CODE_CMD_REQUEST;
    cmd.CMD_IDENTIFICATION = 0xEE;
    cmd.LENGTH_DATA = 0x08;
    for ( int cnt = 0; cnt < passwd_length; cnt++ )
    {
        cmd.DATA_ARR[cnt] = passwd[cnt];
    }
    cmd.XOR_SUM = xor_sum((uint8_t*)&cmd.XOR_SUM, sizeof(cmd));

    QByteArray arr = serialize( cmd );
    emit writeCmdToPort( arr );
    qDebug() << "sendPassword" << arr;
}

void StepperControl::getResponse( QByteArray arr )
{
    in_message_t cmd = deserialize( arr );

    if ( cmd.CMD_TYPE == CODE_CMD_RESPONSE ) {
        if (cmd.DATA.ERROR_OR_COMMAND == ErrorList::OK_ACCESS ) {
            qDebug() << "Успешная авторизация (USB)";
        } else if ( cmd.DATA.ERROR_OR_COMMAND == ErrorList::ERROR_XOR ) {
            qDebug() << "Ошибка контрольной суммы" << cmd.DATA.ERROR_OR_COMMAND;
        } else {
            qDebug() << "Response error: " << cmd.DATA.ERROR_OR_COMMAND;
        }
    }
    if ( cmd.CMD_TYPE == CODE_CMD_POWERSTEP01 ) {
        if ( cmd.DATA.ERROR_OR_COMMAND == COMMAND_GET_ABS_POS ) {
            abs_position = cmd.DATA.RETURN_DATA;
            emit updatePos( QString::number( abs_position ) );
            qDebug() << "position" << abs_position;
        } else if ( cmd.CMD_IDENTIFICATION == CMD_PowerSTEP01_SET_MAX_SPEED ) {
            qDebug() << "set max speed" << cmd.DATA.RETURN_DATA;
        } else if ( cmd.CMD_IDENTIFICATION == CMD_PowerSTEP01_SET_MIN_SPEED ) {
            qDebug() << "set min speed" << cmd.DATA.RETURN_DATA;
        } else if ( cmd.CMD_IDENTIFICATION == CMD_PowerSTEP01_RESET_POS ) {
            qDebug() << "reset position" << cmd.DATA.RETURN_DATA;
        } else if ( cmd.CMD_IDENTIFICATION == CMD_PowerSTEP01_MOVE_F ) {
            qDebug() << "Move F Status: " << cmd.DATA.ERROR_OR_COMMAND;
            sendCommandPowerStep( CMD_PowerSTEP01_GET_ABS_POS, 0 );
        } else if ( cmd.CMD_IDENTIFICATION == CMD_PowerSTEP01_MOVE_R ) {
            qDebug() << "Move R Status: " << cmd.DATA.ERROR_OR_COMMAND;
            sendCommandPowerStep( CMD_PowerSTEP01_GET_ABS_POS, 0 );
        } else {
            qDebug() << "POWERSTEP01 ERROR_OR_COMMAND: " << cmd.DATA.ERROR_OR_COMMAND;

        }

    }

//    emit hasAnswer();
}

void StepperControl::sendCommandPowerStep( CMD_PowerSTEP command, uint32_t data )
{
    out_message_t cmd;
    cmd.XOR_SUM = 0x00;
    cmd.Ver = default_Ver;
    cmd.CMD_TYPE = CODE_CMD_POWERSTEP01;
    cmd.LENGTH_DATA = sizeof(SMSD_CMD_Type);
    cmd.CMD_IDENTIFICATION = command;
    cmd.DATA.COMMAND = command;
    cmd.DATA.DATA = data;
    cmd.DATA.ACTION = 0;
    cmd.XOR_SUM = xor_sum((uint8_t*)&cmd.XOR_SUM, sizeof(cmd));
    QByteArray arr = serialize( cmd );

    emit addCmdToQueue( arr );


    emit writeCmdToPort( arr );
    qDebug() << "sendCommandPowerStep" << arr;

}




QByteArray StepperControl::serialize( out_message_t &cmd )
{
    QByteArray byteArray;
    byteArray.clear();
    QDataStream stream(&byteArray, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_10);
    stream.setByteOrder( QDataStream::LittleEndian );

    uint8_t start = 0xFA, stop = 0xFB;
    uint32_t *n_data = reinterpret_cast<uint32_t*>(&cmd.DATA);

    stream << start
           << cmd.XOR_SUM
           << cmd.Ver
           << cmd.CMD_TYPE
           << cmd.CMD_IDENTIFICATION
           << cmd.LENGTH_DATA
           << *n_data
           << stop;

    qDebug() << "Out >>>" << "XOR" << cmd.XOR_SUM
             << "Ver" << cmd.Ver
             << "CMD_TYPE" << cmd.CMD_TYPE
             << "CMD_ID" << cmd.CMD_IDENTIFICATION
             << "LENGTH_DATA" << cmd.LENGTH_DATA
             << "Data.Action" << cmd.DATA.ACTION
             << "Data.COMMAND" << cmd.DATA.COMMAND
             << "Data.DATA" << cmd.DATA.DATA;

    return byteArray;
}

QByteArray StepperControl::serialize( request_message_t &cmd )
{
    QByteArray byteArray;
    byteArray.clear();
    QDataStream stream(&byteArray, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_10);
    stream.setByteOrder( QDataStream::LittleEndian );

    uint8_t start = 0xFA, stop = 0xFB;

    stream << start
           << cmd.XOR_SUM
           << cmd.Ver
           << cmd.CMD_TYPE
           << cmd.CMD_IDENTIFICATION
           << cmd.LENGTH_DATA;
    for (int i = 0; i < passwd_length; i++)
           stream << cmd.DATA_ARR[ passwd_length - 1 - i ];

    stream << stop;

    qDebug() << "Out >>>" << "XOR" << cmd.XOR_SUM
             << "Ver" << cmd.Ver
             << "CMD_TYPE" << cmd.CMD_TYPE
             << "CMD_ID" << cmd.CMD_IDENTIFICATION
             << "LENGTH_DATA" << cmd.LENGTH_DATA
            << "Data.DATA" << cmd.DATA_ARR;

    return byteArray;
}

in_message_t StepperControl::deserialize(const QByteArray& byteArray)
{
    in_message_t cmd;
    qDebug() << "In <<<" << "arr" << byteArray.toHex();

    QByteArray localArray = byteArray;
    if( localArray.size() < 13 )
    {
        qDebug() << "Неправильная команда!";
        return cmd;
    } else if ( localArray.size() > 13 ) {
        if ( localArray.contains( 0xFE ) )
            qDebug() << "Специальный символ!";
    }

    localArray.remove( 0, 1 ); // leading 0xFA
    localArray.remove( byteArray.size() - 1, 1); // ending 0xFB

    uint8_t index = 0;
    cmd.XOR_SUM = localArray.at(index++);
    cmd.Ver = localArray.at(index++);
    cmd.CMD_TYPE = localArray.at(index++);
    cmd.CMD_IDENTIFICATION = localArray.at(index++);
    uint16_t length = (uint16_t)(localArray.at(index+1) << 8) +
                                         localArray.at(index);
    cmd.LENGTH_DATA = length;

    COMMANDS_RETURN_DATA_Type data;
    // TODO data.STATUS_POWERSTEP01
//    data.STATUS_POWERSTEP01 = byteArray.at(8) << 8 +
//                              byteArray.at(7);
    index += 4;
    data.ERROR_OR_COMMAND = localArray.at(index++);
    data.RETURN_DATA = (uint32_t)(localArray.at(index + 3) << 24) +
                       (uint32_t)(localArray.at(index + 2) << 16) +
                       (uint32_t)(localArray.at(index +1) << 8) +
                       (uint32_t)localArray.at(index);
    cmd.DATA = data;

    qDebug() << "XOR" << cmd.XOR_SUM
             << "Ver" << cmd.Ver
             << "CMD_TYPE" << cmd.CMD_TYPE
             << "CMD_ID" << cmd.CMD_IDENTIFICATION
             << "LENGTH_DATA" << cmd.LENGTH_DATA
             << "Data.ERROR" << cmd.DATA.ERROR_OR_COMMAND
             << "Data.DATA" << cmd.DATA.RETURN_DATA;

    return cmd;
}

void StepperControl::stepForward()
{
    qDebug() << "step+";
//    moveMotor( step_number, false );
    sendCommandPowerStep( CMD_PowerSTEP01_MOVE_F, step_number );

}

void StepperControl::stepBackward()
{
    qDebug() << "step-";
//    moveMotor( step_number, true );

    sendCommandPowerStep( CMD_PowerSTEP01_MOVE_R, step_number );
}

void StepperControl::slotInit()
{
//    sendCommandPowerStep( CMD_PowerSTEP01_RESET_POS, 0 );
//    sendCommandPowerStep( CMD_PowerSTEP01_SET_MAX_SPEED, speed_limit );
//    sendCommandPowerStep( CMD_PowerSTEP01_SET_MIN_SPEED, speed_limit / 2 );
    sendCommandPowerStep( CMD_PowerSTEP01_SET_MIN_SPEED, speed_limit / 2 );

}

void StepperControl::slotSend()
{
    sendPassword();
}

void StepperControl::slotGetPos()
{
    sendCommandPowerStep( CMD_PowerSTEP01_GET_ABS_POS, 0 );

}

void StepperControl::slotSetSpeed()
{
    sendCommandPowerStep( CMD_PowerSTEP01_SET_MAX_SPEED, speed_limit );
//    sendCommandPowerStep( CMD_PowerSTEP01_SET_MIN_SPEED, speed_limit / 2 );

}

void StepperControl::processVector()
{
//    mutex->lock();
//    QByteArray arr = command_buf.takeFirst();
//    mutex->unlock();
//    emit writeCmdToPort( arr );
//    qDebug() << "sendCommandPowerStep" << arr;
}
