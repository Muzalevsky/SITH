#ifndef STEPPERCONTROL_H
#define STEPPERCONTROL_H

#include <QMainWindow>
#include <QGridLayout>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QObject>
#include <QDataStream>


#include <port.h>
#include <smsd_header.h>
#include <settingsdialog.h>
#include <cstdint>

class StepperControl : public QMainWindow
{
    Q_OBJECT
public:
    explicit StepperControl( Port* ext_port, QWidget* parent );
    uint8_t             xor_sum(uint8_t *data,uint16_t length);
    void                sendPassword();
    void                request();
    void                response();
    void                moveMotor( uint32_t step_number, bool reverse );
    void                moveMotor1( uint32_t step_number, bool reverse );

    void                setZero();
    void                setMaxSpeed( uint32_t speed_limit );
    void                getAbsPos();

    QByteArray          serialize( out_message_t &cmd );
    QByteArray          serialize( request_message_t &cmd );

    in_message_t        deserialize(const QByteArray& byteArray);

    QPushButton     *setButton;
    QPushButton     *openButton;
    uint32_t        step_number;

signals:
    void writeCmdToPort(QByteArray arr);
    void updatePos(QString);
public slots:
    void saveSettings();
    void searchPorts();
    void getResponse( QByteArray );
    void updateSpeedLimit(QString str) { speed_limit = str.toDouble(); }
    void updateStepNumber(QString str) { step_number = str.toDouble(); }
    void stepForward();
    void stepBackward();
    void slotInit();
    void slotSend();

private:
    QTimer              *timer;
    uint8_t         passwd_length;
    QComboBox       *PortNameBox;
    uint8_t         default_Ver;
    Port            *port;
    QGridLayout     *gridLayout;
    QLineEdit       *speedEdit;
    QLineEdit       *stepNumberEdit;
    QLineEdit       *stepSizeEdit;
    QPushButton     *initBtn;
    QPushButton     *sendBtn;
    SettingsDialog  *m_settingsDialog;

    uint32_t speed_limit;
    uint32_t abs_position;
    int      passwd[8];

};

#endif // STEPPERCONTROL_H
