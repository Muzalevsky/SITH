#ifndef MODBUSLISTENER_H
#define MODBUSLISTENER_H

#include <QModbusDataUnit>
#include <QTimer>

#include <settingsdialog.h>

class QModbusClient;
class QModbusReply;

namespace Ui {
class SettingsDialog;
}

class ModbusListener : public QObject
{
    Q_OBJECT
public:
    explicit ModbusListener(QObject *parent = nullptr);
    ~ModbusListener();
    QModbusDataUnit readRequest() const;

    bool isModbusConnected();
    bool isModbusAlive();

    const QModbusDataUnit *unit;

    double voltagePhaseA;
    double voltagePhaseB;
    double voltagePhaseC;
    double currentPhaseA;
    double currentPhaseB;
    double currentPhaseC;
    double frequency;
    double power;
    float  temperature;

    SettingsDialog *m_settingsDialog;
    QModbusClient *modbusDevice;

    QString     portName;
private:
    const int temperature_sensor_address = 1;
    const int electrical_sensor_address = 2;

    bool        m_connected;
    bool        _alive;

    QTimer      *timer;
    int         slaveNumber;
    QModbusReply *lastRequest;

public slots:
    void on_connectButton_clicked();
    void updateSlaveNumber( int number );
    void reconnectModbus();
private slots:
    void onStateChanged(int state);
    void on_readButton_clicked();
    void readReady();
signals:
    void getReply();
    void getTemperature();
    void isConnected(bool);
};

#endif // MODBUSLISTENER_H
