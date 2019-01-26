#ifndef BENCHWINDOW_H
#define BENCHWINDOW_H

#include <QDebug>
#include <QMainWindow>
#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>

#include <QTime>
#include <QDate>
#include <QFile>

#include "port.h"
#include "forcewindow.h"
#include "steppercontrol.h"
#include "modbuslistener.h"

class BenchWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit BenchWindow(QWidget *parent = 0);
    ~BenchWindow();
private:
    QTimer              *timer;
    void initializeWindow();
    void startAuto();

    Port                *force_serial;
    ForceWindow         *force_ui;
    ModbusListener      *rs485_serial;
    Port                *stepper_serial;
    StepperControl      *stepper_ui;
    QTabWidget          *mainWgt;

    QLineEdit           *voltageA_edit;
    QLineEdit           *voltageB_edit;
    QLineEdit           *voltageC_edit;
    QLineEdit           *frequency_edit;
    QLineEdit           *currentA_edit;
    QLineEdit           *resistance_edit;

    double              voltagePhaseA;
    double              voltagePhaseB;
    double              voltagePhaseC;
    double              currentPhaseA;
    double              resistance;
    double              frequency;

    QGridLayout         *layout_tune;
    QGridLayout         *layout_work;
    QLineEdit           *forceEdit;
    QLineEdit           *stepperPosEdit;
    QLineEdit           *pointsNumberEdit;

    QWidget             *tuneWidget;
    QWidget             *workWidget;
    QFile               *file;
    int                 position_counter;
signals:
    void stepForward();
    void stepBackward();
    void updateForceValue();
    void updateElectricalValues();

public slots:
    void updateElectricParameters();
    void updateForceEdit( QString str );
    void updatePosEdit( QString str );
    void printString();

};

#endif // BENCHWINDOW_H
