#ifndef STEPPERCONTROL_H
#define STEPPERCONTROL_H

#include <QObject>
#include <QDataStream>

#include <port.h>
#include <smsd_header.h>
#include <settingsdialog.h>
#include <cstdint>

class StepperControl : public QObject
{
    Q_OBJECT
public:
    explicit StepperControl( QObject* parent = nullptr );
// QA: what does it for? 
    ~StepperControl();

    bool            isAuthorized();
    void            sendPassword();
    void            getRelayState();

    uint32_t        step_number;
    SettingsDialog  *m_settingsDialog;
    Port            *port;
    QString         portName;
    int             step_per_mm;

private:
    uint8_t         xor_sum(uint8_t *data, uint16_t length);
    QByteArray      serialize( out_message_t &cmd );
    QByteArray      serialize( request_message_t &cmd );
    in_message_t    deserialize(const QByteArray& byteArray);
    void            sendCommandPowerStep( CMD_PowerSTEP command, uint32_t data );
signals:
    void writeCmdToPort(QByteArray arr);
    void updatePos(QString);
    void isLineSwitchOn(bool);
    void disableAll();
public slots:
    void saveSettings();
    void getResponse( QByteArray );
    void initialize();

    void disableElectricity();

    void resetMotorPosition();

    void updateStepNumber(double );
    void stepForward();
    void stepBackward();
//    void slotGetPos();
//    void slotSetSpeed();
    void resetMotorSupply();
    void lineSwitchClicked();
    void reconnectStepper();
//    void recreatePort();
    void relayOn();
    void relayOff();

private:
    QTimer      *timer;
    QVector <QByteArray>    command_buf;
    uint8_t     passwd_length;
    uint8_t     default_Ver;
    uint32_t    speed_limit;
    uint32_t    abs_position;

    bool        _isAuthorized;
    bool        receiveFlag;
    bool        sendFlag;
    bool        isRelayOn;
    int         passwd[8];
};

#endif // STEPPERCONTROL_H
