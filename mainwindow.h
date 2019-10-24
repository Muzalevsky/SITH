#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDebug>
#include <QTimer>
#include <QSettings>
#include <QTime>
#include <QDate>
#include <QFile>
#include <QMainWindow>
#include <QCloseEvent>

#include "port.h"
#include "steppercontrol.h"
#include "modbuslistener.h"
#include "settingsdialog.h"
#include "forcewindow.h"
#include "encodercontrol.h"

#define ORGANIZATION_NAME "LETI"
#define ORGANIZATION_DOMAIN "www.etu.ru"
#define APPLICATION_NAME "SITH Control program"

enum WorkingModes
{
    MANUAL_MODE = 0,
    SEMIAUTO_MODE,
    FULLAUTO_MODE
};

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

    void writeSettings();
    void loadSettings();

    int checkProtocolHeader();
    int checkConnectionStates();
    int checkConnectionStatesMuted();

    int isSerialAlive();
    void setStateButtonColor(int colorCode);

//    void updatePositionFromEncoder();
//    void updateForceFromTensoHead();

    void closeEvent(QCloseEvent *event);

    QWidget             *mainwidget;
    QSettings           *settings;

    QFile               *protocolFile;
    QFile               *protocolCsvFile;

    Port                *force_serial;
    Port                *stepper_serial;
    Port                *encoder_serial;

    ModbusListener      *rs485_serial;
    EncoderControl      *encoder_ui;
    ForceWindow         *force_ui;
    StepperControl      *stepper_ui;

    QTimer              *update_timer;
    QTimer              *measuringTimeoutTimer;
    QTimer              *motorSupplyOffTimer;

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

    QString             position_raw;
    float               force_kg;
    int                 stepNumber;
    int                 stepDoneNumber;
    bool                stop_flag;
    bool                isBusy;
signals:
    void doStepForward();
    void doStepBackward();
    void resetStepperSupply();
    void reconnectForceWindow();
    void reconnectEncoder();
    void reconnectModbus();
    void sendModbusRequest();
    void goingToClose();
    void weAreGoingToBreakSensor();
    void tooMuchForce();

public slots:
    void lineSwitchClicked();
    void updateElectricParameters();
    void updateForceValue( float forceValue );
    void updateManualStep( double );
    void updatePosition();
    void updateStopFlag( bool );
    void updateTemperature();
    void updateTime();
    void updateState();

    void endMeasuring();

    void calculateStepNumber(double val);
    void connect_clicked();
    void changeButtonText(bool);
    void modeChanged(int);

    //Protocol processing
    void printProtocolHeader();
    void printCsvHeader();
    void printCsvString();
    void printString();

    void searchSerialPorts();
    void setOperatorName(QString);
    void startAuto();

    void gotStepBackward();
    void gotStepForward();

    void measureNextPoint();

};

#endif // MAINWINDOW_H
