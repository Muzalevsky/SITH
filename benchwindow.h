#ifndef BENCHWINDOW_H
#define BENCHWINDOW_H

#include <QDebug>
#include <QMainWindow>
#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>

#include "port.h"
#include "forcewindow.h"
#include <steppercontrol.h>
#include <modbuslistener.h>

class BenchWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit BenchWindow(QWidget *parent = 0);
    ~BenchWindow();
private:
    QTimer              *timer;
    void initializeWindow();

    Port                *force_serial;
    ForceWindow         *force_ui;
    ModbusListener      *rs485_serial;
    Port                *stepper_serial;
    StepperControl      *stepper_ui;
    QString             force_str;
    QTabWidget          *mainWgt;
    QPushButton     *force_serialButton;
    QPushButton     *rs485_serialButton;
    QPushButton     *stepper_serialButton;

    QLineEdit       *voltageA_edit;
    QLineEdit       *voltageB_edit;
    QLineEdit       *voltageC_edit;
    QLineEdit       *frequency_edit;
    QLineEdit       *currentA_edit;
    QLineEdit       *resistance_edit;

    double              voltagePhaseA;
    double              voltagePhaseB;
    double              voltagePhaseC;
    double              currentPhaseA;
    double              resistance;
    double              frequency;

    QGridLayout         *layout_tune;
    QGridLayout         *layout_work;
    QLineEdit           *forceEdit;
    QWidget             *tuneWidget;
    QWidget             *workWidget;

signals:
    void stepForward();
    void stepBackward();
    void updateForceValue();
    void updateElectricalValues();

public slots:
    void setForceValue( QString );
    void sendStepForward();
    void sendStepBackward();
    void openForce();
    void openModbus();
    void getVoltage();
    void errorHandler( QString );
    void updateForceEdit();

};

#endif // BENCHWINDOW_H
