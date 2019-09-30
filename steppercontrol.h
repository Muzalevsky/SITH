#ifndef STEPPERCONTROL_H
#define STEPPERCONTROL_H

#include <QMainWindow>
//#include <QGridLayout>
//#include <QComboBox>
//#include <QLabel>
//#include <QPushButton>
//#include <QLineEdit>
#include <QObject>
#include <QDataStream>
//#include <QMutex>

#include <port.h>
#include <smsd_header.h>
#include <settingsdialog.h>
#include <cstdint>

class StepperControl : public QMainWindow
{
    Q_OBJECT
public:
    explicit StepperControl( Port* ext_port, QWidget* parent );
    ~StepperControl();

    uint8_t         xor_sum(uint8_t *data,uint16_t length);
    void            sendPassword();
    int             step_per_mm;
    QByteArray      serialize( out_message_t &cmd );
    QByteArray      serialize( request_message_t &cmd );
    in_message_t    deserialize(const QByteArray& byteArray);
    void            sendCommandPowerStep( CMD_PowerSTEP command, uint32_t data );
    uint32_t        step_number;
    SettingsDialog  *m_settingsDialog;
    Port            *port;
    QString         portName;
signals:
    void writeCmdToPort(QByteArray arr);
    void updatePos(QString);
    void hasAnswer();
    void addCmdToQueue( QByteArray arr );
    void isLineSwitchOn(bool);

public slots:
    void saveSettings();
    void getResponse( QByteArray );
//    void updateSpeedLimit(QString str);
    void updateStepNumber(double );
    void stepForward();
    void stepBackward();
    void slotInit();
    void slotSend();
    void slotGetPos();
    void slotSetSpeed();
    void resetMotorSupply();
//    void slotSetRele();
//    void slotClearRele();
//    void processVector();
    void lineSwitchClicked(bool isPressed);

    void relayOn();
    void relayOff();

private:
    QTimer      *timer;
    QVector <QByteArray>    command_buf;
//    QMutex          *mutex;
    uint8_t     passwd_length;
    uint8_t     default_Ver;
    uint32_t    speed_limit;
    uint32_t    abs_position;
    int         passwd[8];
    bool        receiveFlag;
    bool        sendFlag;

    bool        isRelayOn;


};

#endif // STEPPERCONTROL_H
