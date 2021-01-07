#include "steppercontrol.h"

#include <cstdlib>

#include <QDebug>
#include <QTimer>

//step_number(66) = 0,4 мм на штоке
//step_number(82) = 0,5 мм на штоке

// Tuning constant depends on mechanic
//int step_per_mm = 164;

StepperControl::StepperControl(QObject *parent ) : QObject( parent ),
    step_number(82),
    step_per_mm(164),
    passwd_length(8),
    default_Ver(0x02),
    speed_limit(250),
    abs_position(0),
    _isAuthorized(false),
    isRelayOn(false),
    passwd{ 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF }
{
    port = new Port();
    port->setPortOpenMode(QIODevice::ReadWrite);
    m_settingsDialog = new SettingsDialog();

    connect(port, SIGNAL(outPortByteArray(QByteArray)), this, SLOT(getResponse(QByteArray)));
    connect(this, &StepperControl::writeCmdToPort, port, &Port::WriteToPort);
    connect(this, &StepperControl::disableAll, this, &StepperControl::disableElectricity);

}

StepperControl::~StepperControl()
{
    emit disableAll();

//    disableElectricity();
    //Here we stop talking with stepper driver
//    port->closePort();
}

void StepperControl::disableElectricity()
{
    qDebug() << "GOT disableElectricity";

    resetMotorSupply();
    relayOff();
}

void StepperControl::resetMotorSupply()
{
    qDebug() << "Sending soft stop";
    sendCommandPowerStep(CMD_PowerSTEP01_SOFT_HI_Z, 0);
}


void StepperControl::saveSettings()
{
    port->setPortSettings( portName,
                           m_settingsDialog->settings().baud,
                           m_settingsDialog->settings().dataBits,
                           m_settingsDialog->settings().parity,
                           m_settingsDialog->settings().stopBits,
                           QSerialPort::NoFlowControl );

    qDebug() << "Stepper port settings saved.";
}


uint8_t StepperControl::xor_sum(uint8_t *data,uint16_t length)
{
    uint8_t xor_temp = 0xFF;
    while( length-- )
    {
        xor_temp += *data;
        data++;
    }

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

void StepperControl::initialize()
{
    //    sendCommandPowerStep( CMD_PowerSTEP01_RESET_POS, 0 );
    //    sendCommandPowerStep( CMD_PowerSTEP01_SET_MAX_SPEED, speed_limit );
    //    sendCommandPowerStep( CMD_PowerSTEP01_SET_MIN_SPEED, speed_limit / 2 );
    sendCommandPowerStep( CMD_PowerSTEP01_SET_MAX_SPEED, speed_limit );

}


void StepperControl::resetMotorPosition()
{
    sendCommandPowerStep( CMD_PowerSTEP01_RESET_POS, 0 );
    qDebug() << "resetMotorPosition()";
}


void StepperControl::getResponse( QByteArray arr )
{
    in_message_t cmd = deserialize( arr );

    if ( cmd.CMD_TYPE == CODE_CMD_RESPONSE )
    {
        uint8_t answer = cmd.DATA.ERROR_OR_COMMAND;
        switch (answer)
        {
            case ErrorList::OK_ACCESS:
                qDebug() << "Успешная авторизация (USB)";
                _isAuthorized = true;
                break;

            case ErrorList::ERROR_ACCESS:
                qDebug() << "Ошибка авторизации" << answer;
                _isAuthorized = true;
                break;
            case ErrorList::ERROR_XOR:
                qDebug() << "Ошибка контрольной суммы" << answer;
                break;

            case ErrorList::STATUS_RELE_SET:
                qDebug() << "Статус: Реле ВКЛ" << answer;
                isRelayOn = true;
                emit isLineSwitchOn( isRelayOn );
                break;

            case ErrorList::STATUS_RELE_CLR:
                qDebug() << "Статус: Реле ВЫКЛ" << answer;
                isRelayOn = false;
                emit isLineSwitchOn( isRelayOn );
                break;

            default:
                qDebug() << "Response error: " << answer;
                break;
        }
    }
    if ( cmd.CMD_TYPE == CODE_CMD_POWERSTEP01 )
    {
        if ( cmd.DATA.ERROR_OR_COMMAND == COMMAND_GET_ABS_POS )
        {
            abs_position = cmd.DATA.RETURN_DATA;
            emit updatePos( QString::number( abs_position ) );
            qDebug() << "abs_position from motor" << abs_position;
        }
        else if ( cmd.CMD_IDENTIFICATION == CMD_PowerSTEP01_SET_MAX_SPEED )
        {
            qDebug() << "set max speed" << cmd.DATA.RETURN_DATA;
        }
        else if ( cmd.CMD_IDENTIFICATION == CMD_PowerSTEP01_SET_MIN_SPEED )
        {
            qDebug() << "set min speed" << cmd.DATA.RETURN_DATA;
        }
        else if ( cmd.CMD_IDENTIFICATION == CMD_PowerSTEP01_RESET_POS )
        {
            qDebug() << "reset position" << cmd.DATA.RETURN_DATA;
            sendCommandPowerStep( CMD_PowerSTEP01_GET_ABS_POS, 0 );
        }
        else if ( cmd.CMD_IDENTIFICATION == CMD_PowerSTEP01_MOVE_F )
        {
            qDebug() << "Move F Status: " << cmd.DATA.ERROR_OR_COMMAND;
            sendCommandPowerStep( CMD_PowerSTEP01_GET_ABS_POS, 0 );
        }
        else if ( cmd.CMD_IDENTIFICATION == CMD_PowerSTEP01_MOVE_R )
        {
            qDebug() << "Move R Status: " << cmd.DATA.ERROR_OR_COMMAND;
            sendCommandPowerStep( CMD_PowerSTEP01_GET_ABS_POS, 0 );
        }
        else if ( cmd.CMD_IDENTIFICATION == CMD_PowerSTEP01_SOFT_HI_Z )
        {
            qDebug() << "Soft stop status: " << cmd.DATA.ERROR_OR_COMMAND;
        }
        else if ( cmd.CMD_IDENTIFICATION == CMD_PowerSTEP01_SET_RELE )
        {
            isRelayOn = true;
            emit isLineSwitchOn( isRelayOn );
            qDebug() << "Set relay: " << cmd.DATA.ERROR_OR_COMMAND;
        }
        else if ( cmd.CMD_IDENTIFICATION == CMD_PowerSTEP01_CLR_RELE )
        {
            isRelayOn = false;
            emit isLineSwitchOn( isRelayOn );
            qDebug() << "Clear relay: " << cmd.DATA.ERROR_OR_COMMAND;
        }
        else
        {
            qDebug() << "POWERSTEP01 ERROR_OR_COMMAND: " << cmd.DATA.ERROR_OR_COMMAND;
        }
    }
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

    emit writeCmdToPort( arr );
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
    qDebug() << "Stepper IN <<<" << byteArray.toHex();

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
    qDebug() << "step-" << step_number;
    sendCommandPowerStep( CMD_PowerSTEP01_MOVE_F, step_number );
}

void StepperControl::stepBackward()
{
    qDebug() << "step+" << step_number;
    sendCommandPowerStep( CMD_PowerSTEP01_MOVE_R, step_number );
}

//void StepperControl::slotGetPos()
//{
//    sendCommandPowerStep( CMD_PowerSTEP01_GET_ABS_POS, 0 );
//}

//void StepperControl::slotSetSpeed()
//{
//    sendCommandPowerStep( CMD_PowerSTEP01_SET_MAX_SPEED, speed_limit );
////    sendCommandPowerStep( CMD_PowerSTEP01_SET_MIN_SPEED, speed_limit / 2 );

//}

void StepperControl::updateStepNumber( double step_mm )
{
    step_number = static_cast <uint32_t>( step_mm * step_per_mm );
}

void StepperControl::relayOn()
{
    qDebug() << "Sending relay ON";
    sendCommandPowerStep( CMD_PowerSTEP01_SET_RELE, 0 );
}

void StepperControl::relayOff()
{
    qDebug() << "Sending relay OFF";
    sendCommandPowerStep( CMD_PowerSTEP01_CLR_RELE, 0 );
}

void StepperControl::lineSwitchClicked()
{
    if ( isRelayOn ) {
        resetMotorSupply();

        // Solenoid supply OFF
        relayOff();
    } else {
        // Solenoid supply ON
        relayOn();
    }
}

void StepperControl::reconnectStepper()
{
    port->closePort();
    _isAuthorized = false;
    port->openPort();
    sendPassword();
    getRelayState();
}

/*
 * May be BUG in the SMSD block but
 * it makes strange response
 */
void StepperControl::getRelayState()
{
    sendCommandPowerStep( CMD_PowerSTEP01_GET_RELE, 0 );
    qDebug() << "getRelayState";
}



bool StepperControl::isAuthorized()
{
    return _isAuthorized;
}
