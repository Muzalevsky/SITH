#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDebug>
#include <QTimer>
#include <QSettings>
#include <QTime>
#include <QDate>
#include <QFile>
#include <QMainWindow>

#include "port.h"
#include "steppercontrol.h"
#include "modbuslistener.h"
#include "settingsdialog.h"
#include "forcewindow.h"
#include "encodercontrol.h"

#define ORGANIZATION_NAME "LETI"
#define ORGANIZATION_DOMAIN "www.etu.ru"
#define APPLICATION_NAME "SITH Control program"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private:
    Ui::MainWindow *ui;
    void initializeWindow();
    void spec_delay();
    void writeSettings();
    void loadSettings();
    void readSettings();
    void printProtocolHeader();

    QWidget             *mainwidget;
    QSettings           *settings;

    Port                *force_serial;
    ModbusListener      *rs485_serial;
    Port                *stepper_serial;
    Port                *encoder_port;

    EncoderControl      *encoder_ui;
    ForceWindow         *force_ui;
    StepperControl      *stepper_ui;

    QTimer              *update_timer;

    uint32_t            manual_step_number;
    uint32_t            auto_step_number;
    QString             force_str;
    float               currentPosition;
    double              voltagePhaseA;
    double              voltagePhaseB;
    double              voltagePhaseC;
    double              currentPhaseA;
    double              currentPhaseB;
    double              currentPhaseC;
    double              resistance;
    double              frequency;
    float               temperature;
    QFile               *file;
    QString             position_raw;
    float               force_kg;
    int                 stepNumber;
    bool                stop_flag;
signals:
    void doStepForward();
    void doStepBackward();
    void updateElectricalValues();
    void positionChanged(QString);

public slots:
    void updateElectricParameters();
    void updateForceValue( QString str );
    void updateManualStep( double );
    void updatePosition();
    void updateStopFlag( bool );
    void updateTemperature();
    void updateTime();

    void calculateStepNumber(double val);
    void changeButtonText(bool);
    void connect_clicked();
    void modeChanged(int);
    void printString();
    void searchSerialPorts();
    void setSerialSettings();
    void setZeroForce();
    void startAuto();
//    void changeLineSwitch();
    void setOperatorName(QString);
};

#endif // MAINWINDOW_H
