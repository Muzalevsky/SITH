#include "steppercontrol.h"

#include <cstdlib>

#include <QDebug>
#include <QTimer>

StepperControl::StepperControl( Port* ext_port, QWidget* parent ) : QMainWindow( parent ),
    step_number(66),
    default_Ver(0x02),
    speed_limit(0),
    abs_position(0),
    passwd({ 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF } )
{
    port = ext_port;
    passwd_length = 8;
    gridLayout      = new QGridLayout();
    speedEdit       = new QLineEdit();
    stepNumberEdit  = new QLineEdit();
    stepSizeEdit    = new QLineEdit();


//    uint8_t data[10] = { 0x00, 0x02, 0x02, 0x98, 0x04, 0x00, 0x20, 0x00, 0x00};
//    qDebug() << xor_sum(data, 10);
//    uint8_t data1[15] = { 0x00, 0x01, 0x00, 0x7F, 0x08, 0x00, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
//    qDebug() << xor_sum(data1, 14);



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
    qDebug() << "xor_temp" << (xor_temp ^ 0xFF);
    return ( xor_temp ^ 0xFF );
}

void StepperControl::sendPassword()
{
    // {B8}{01}{00}{7F}{08}{00}{01}{23}{45}{67}{89}{AB}{CD}{EF}
    request_message_t cmd;
    cmd.XOR_SUM = 0x00;
    cmd.Ver = 0x01;
    cmd.CMD_TYPE = CODE_CMD_REQUEST;
    cmd.CMD_IDENTIFICATION = 0x7F;
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
            qDebug() << "Password response";
        } else {
            qDebug() << "Wrong password";
        }
    }
    if ( cmd.CMD_TYPE == CODE_CMD_POWERSTEP01 ) {
        if ( cmd.CMD_IDENTIFICATION == CMD_PowerSTEP01_GET_ABS_POS ) {
            abs_position = cmd.DATA.RETURN_DATA;
            emit updatePos( QString::number( abs_position ) );
            qDebug() << "position" << abs_position;
        }
        if ( cmd.CMD_IDENTIFICATION == CMD_PowerSTEP01_MOVE_F ) {
            qDebug() << "Move F Status: " << cmd.DATA.ERROR_OR_COMMAND;
        }
        if ( cmd.CMD_IDENTIFICATION == CMD_PowerSTEP01_MOVE_R ) {
            qDebug() << "Move R Status: " << cmd.DATA.ERROR_OR_COMMAND;
        }
    }
}

void StepperControl::moveMotor( uint32_t step_number, bool reverse )
{
    out_message_t cmd;
    cmd.Ver = default_Ver;
    cmd.CMD_TYPE = CODE_CMD_POWERSTEP01;
    cmd.LENGTH_DATA = sizeof(SMSD_CMD_Type);
    cmd.DATA.DATA = step_number;

    if ( step_number ) {
        if ( reverse ) {
            cmd.DATA.COMMAND = CMD_PowerSTEP01_MOVE_R;
            cmd.CMD_IDENTIFICATION = CMD_PowerSTEP01_MOVE_R;
        } else {
            cmd.DATA.COMMAND = CMD_PowerSTEP01_MOVE_F;
            cmd.CMD_IDENTIFICATION = CMD_PowerSTEP01_MOVE_F;
        }
    } else {
        cmd.DATA.COMMAND = CMD_PowerSTEP01_GO_ZERO;
        cmd.CMD_IDENTIFICATION = CMD_PowerSTEP01_GO_ZERO;
    }

    cmd.XOR_SUM=xor_sum((uint8_t*)&cmd.XOR_SUM, sizeof(cmd));
    QByteArray arr = serialize( cmd );
    emit writeCmdToPort( arr );
    qDebug() << "moveMotor" << arr;

}

void StepperControl::setZero()
{
    out_message_t cmd;
    cmd.Ver = default_Ver;
    cmd.CMD_TYPE = CODE_CMD_POWERSTEP01;
    cmd.LENGTH_DATA = sizeof(SMSD_CMD_Type);
    cmd.CMD_IDENTIFICATION = CMD_PowerSTEP01_RESET_POS;
    cmd.DATA.COMMAND = CMD_PowerSTEP01_RESET_POS;
    cmd.XOR_SUM=xor_sum((uint8_t*)&cmd.XOR_SUM, sizeof(cmd));
    QByteArray arr = serialize( cmd );
    emit writeCmdToPort( arr );
    qDebug() << "setZero" << arr;

}

void StepperControl::setMaxSpeed( uint32_t speed_limit )
{
    out_message_t cmd;
    cmd.Ver = default_Ver;
    cmd.CMD_TYPE = CODE_CMD_POWERSTEP01;
    cmd.LENGTH_DATA = sizeof(SMSD_CMD_Type);
    cmd.CMD_IDENTIFICATION = CMD_PowerSTEP01_SET_MAX_SPEED;
    cmd.DATA.COMMAND = CMD_PowerSTEP01_SET_MAX_SPEED;
    cmd.DATA.DATA = speed_limit;
    cmd.XOR_SUM=xor_sum((uint8_t*)&cmd.XOR_SUM, sizeof(cmd));
    QByteArray arr = serialize( cmd );
    emit writeCmdToPort( arr );
    qDebug() << "setMaxSpeed" << arr;
}

void StepperControl::getAbsPos()
{
    out_message_t cmd;
    cmd.Ver = default_Ver;
    cmd.CMD_TYPE = CODE_CMD_POWERSTEP01;
    cmd.LENGTH_DATA = 1024;//sizeof(SMSD_CMD_Type);
    cmd.CMD_IDENTIFICATION = CMD_PowerSTEP01_GET_ABS_POS;
    cmd.DATA.COMMAND = CMD_PowerSTEP01_GET_ABS_POS;
    cmd.XOR_SUM=xor_sum((uint8_t*)&cmd.XOR_SUM, sizeof(cmd));
    QByteArray arr = serialize( cmd );
    emit writeCmdToPort( arr );
    qDebug() << "getAbsPos";
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

//    byteArray.append( start );
//    byteArray.append( cmd.XOR_SUM );
//    byteArray.append( cmd.Ver );
//    byteArray.append( cmd.CMD_TYPE );
//    byteArray.append( cmd.CMD_IDENTIFICATION );

//    uint8_t h_length = (uint8_t )(cmd.LENGTH_DATA >> 8);
//    uint8_t l_length = (uint8_t )(cmd.LENGTH_DATA && 0x00FF);

//    byteArray.append( l_length );
//    byteArray.append( h_length );


//    byteArray.append( stop );


    stream << start
           << cmd.XOR_SUM
           << cmd.Ver
           << cmd.CMD_TYPE
           << cmd.CMD_IDENTIFICATION
           << cmd.LENGTH_DATA
           << *n_data
           << stop;

    qDebug() << "XOR" << cmd.XOR_SUM
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
//    byteArray.append( start );
//    byteArray.append( cmd.XOR_SUM );
//    byteArray.append( cmd.Ver );
//    byteArray.append( cmd.CMD_TYPE );
//    byteArray.append( cmd.CMD_IDENTIFICATION );
//    uint8_t h_length = (uint8_t )(cmd.LENGTH_DATA >> 8);
//    uint8_t l_length = (uint8_t )(cmd.LENGTH_DATA && 0x00FF);
//    byteArray.append( l_length );
//    byteArray.append( h_length );

//    byteArray.append( stop );


    stream << start
           << cmd.XOR_SUM
           << cmd.Ver
           << cmd.CMD_TYPE
           << cmd.CMD_IDENTIFICATION
           << cmd.LENGTH_DATA;
    for (int i = 0; i < passwd_length; i++)
           stream << cmd.DATA_ARR[ passwd_length - 1 - i ];

    stream << stop;

    qDebug() << "XOR" << cmd.XOR_SUM
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
    qDebug() << "arr" << byteArray;

    if( byteArray.size() != 15 )
    {
        qDebug() << "Wrong command!";
        return cmd;
    }

    cmd.XOR_SUM = byteArray.at(1);
    cmd.Ver = byteArray.at(2);
    cmd.CMD_TYPE = byteArray.at(3);
    cmd.CMD_IDENTIFICATION = byteArray.at(4);
    uint16_t length = (uint16_t)(byteArray.at(6) << 8) +
                                         byteArray.at(5);
    cmd.LENGTH_DATA = length;

    COMMANDS_RETURN_DATA_Type data;
    // TODO data.STATUS_POWERSTEP01
//    data.STATUS_POWERSTEP01 = byteArray.at(8) << 8 +
//                              byteArray.at(7);
    data.ERROR_OR_COMMAND = byteArray.at(9);
    data.RETURN_DATA = (uint32_t)(byteArray.at(13) << 24) +
                       (uint32_t)(byteArray.at(12) << 16) +
                       (uint32_t)(byteArray.at(11) << 8) +
                       (uint32_t)byteArray.at(10);
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
    qDebug() << "step+" << sizeof(COMMANDS_RETURN_DATA_Type);
    moveMotor( step_number, false );
}

void StepperControl::stepBackward()
{
    qDebug() << "step-";
    moveMotor( step_number, true );
}

void StepperControl::slotInit()
{
    sendPassword();
    setZero();
    setMaxSpeed( 10 );
    qDebug() << "init";
}

void StepperControl::slotSend()
{
    sendPassword();

}
