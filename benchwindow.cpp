#include "benchwindow.h"

#include <QThread>
#include <QLabel>

BenchWindow::BenchWindow(QWidget *parent) : QMainWindow( parent )
{
    initializeWindow();

    timer = new QTimer();
//    timer->setInterval(500);
//    connect( timer, &QTimer::timeout, this, &BenchWindow::updateForceEdit );

    //Default
    force_serial->setPortSettings( QString( "//./COM5" ), 9600, 8, 0, 0, 0 );
    stepper_serial->setPortSettings( QString( "//./COM4" ), 115200, 8, 0, 1, 0 );


    //Force value thread
    connect( force_ui, SIGNAL(updateForce(QString)), this, SLOT(updateForceEdit(QString)) );
    QThread *thread_Force = new QThread;
    force_serial->moveToThread(thread_Force);
    connect(force_serial, SIGNAL(finished_Port()), thread_Force, SLOT(deleteLater()));
    connect(force_serial, SIGNAL(finished_Port()), thread_Force, SLOT(quit()));
    connect(thread_Force, SIGNAL(started()), force_serial, SLOT(process_Port()));//Переназначения метода run
    connect(thread_Force, SIGNAL(finished()), force_serial, SLOT(deleteLater()));//Удалить к чертям поток
    thread_Force->start();

    //Modbus thread
    QThread *thread_Modbus = new QThread;
    rs485_serial->moveToThread(thread_Modbus);
    connect( rs485_serial, &ModbusListener::getReply, this, &BenchWindow::updateElectricParameters);
    connect(thread_Modbus, SIGNAL(finished()), rs485_serial, SLOT(deleteLater()));//Удалить к чертям поток
    thread_Modbus->start();

    //Stepper thread
    connect( stepper_ui, SIGNAL(updatePos(QString)), this, SLOT(updatePosEdit(QString)) );

    QThread *thread_Stepper = new QThread;
    stepper_serial->moveToThread(thread_Stepper);
    connect(stepper_serial, SIGNAL(finished_Port()), thread_Force, SLOT(deleteLater()));
    connect(stepper_serial, SIGNAL(finished_Port()), thread_Force, SLOT(quit()));
    connect(thread_Stepper, SIGNAL(started()), stepper_serial, SLOT(process_Port()));//Переназначения метода run
    connect(thread_Stepper, SIGNAL(finished()), stepper_serial, SLOT(deleteLater()));//Удалить к чертям поток
    thread_Stepper->start();
}

BenchWindow::~BenchWindow()
{

}

void BenchWindow::initializeWindow()
{
    workWidget      = new QWidget( this );
    tuneWidget      = new QWidget( this );
    force_serial    = new Port();
    force_ui        = new ForceWindow( force_serial, this);
    rs485_serial    = new ModbusListener();
    stepper_serial  = new Port();
    stepper_ui      = new StepperControl( stepper_serial, this);
    layout_work     = new QGridLayout(workWidget);
    forceEdit       = new QLineEdit();
    stepperPosEdit  = new QLineEdit();
    voltageA_edit   = new QLineEdit();
    voltageB_edit   = new QLineEdit();
    voltageC_edit   = new QLineEdit();
    frequency_edit  = new QLineEdit();
    currentA_edit   = new QLineEdit();
    resistance_edit = new QLineEdit();

    forceEdit->setReadOnly( true );
    stepperPosEdit->setReadOnly( true );
    voltageA_edit->setReadOnly( true );
    voltageB_edit->setReadOnly( true );
    voltageC_edit->setReadOnly( true );
    frequency_edit->setReadOnly( true );
    currentA_edit->setReadOnly( true );
    resistance_edit->setReadOnly( true );

    QPushButton *stepForward = new QPushButton( "Step +" );
    connect( stepForward, SIGNAL( pressed() ), stepper_ui, SLOT( stepForward() )  );
    QPushButton *stepBackward = new QPushButton( "Step -" );
    connect( stepBackward, SIGNAL( pressed() ), stepper_ui, SLOT( stepBackward() )  );

    layout_work->addWidget( new QLabel( tr("Force") ), 1, 2, 1, 1 );
    layout_work->addWidget( forceEdit, 1, 3, 1, 1 );
    layout_work->addWidget( new QLabel( tr("Voltage: ") ), 2, 2, 1, 1 );
    layout_work->addWidget( voltageA_edit, 2, 3, 1, 1 );
    layout_work->addWidget( voltageB_edit, 2, 4, 1, 1 );
    layout_work->addWidget( voltageC_edit, 2, 5, 1, 1 );
    layout_work->addWidget( new QLabel( tr("Frequency: ") ), 3, 2, 1, 1 );
    layout_work->addWidget( frequency_edit, 3, 3, 1, 1 );
    layout_work->addWidget( new QLabel( tr("Current: ") ), 4, 2, 1, 1 );
    layout_work->addWidget( currentA_edit, 4, 3, 1, 1 );
    layout_work->addWidget( new QLabel( tr("Resistance: ") ), 5, 2, 1, 1 );
    layout_work->addWidget( resistance_edit, 5, 3, 1, 1 );
    layout_work->addWidget( stepForward, 6, 2, 1, 1 );
    layout_work->addWidget( stepBackward, 6, 3, 1, 1 );
    layout_work->addWidget( new QLabel( tr("Abs position: ") ), 6, 4, 1, 1 );
    layout_work->addWidget( stepperPosEdit, 6, 5, 1, 1 );

    layout_work->setHorizontalSpacing( 10 );
    layout_work->setVerticalSpacing( 10 );
    workWidget->setLayout( layout_work );
    mainWgt = new QTabWidget(this);
    mainWgt->addTab( workWidget, tr("Work") );

    layout_tune = new QGridLayout(tuneWidget);
    layout_tune->addWidget( force_ui, 1, 1, 1, 1 );
    layout_tune->addWidget( rs485_serial, 2, 1, 1, 1 );
    layout_tune->addWidget( stepper_ui, 1, 2, 1, 1 );
    tuneWidget->setLayout( layout_tune );

    mainWgt->addTab( tuneWidget, tr("Tune") );
    setCentralWidget( mainWgt );
    setWindowTitle( "Bench control" );
}

void BenchWindow::updateForceEdit( QString str )
{
    forceEdit->setText( str );
}

void BenchWindow::updatePosEdit( QString str )
{
    stepperPosEdit->setText( str );
}

void BenchWindow::updateElectricParameters()
{
    voltagePhaseA   = rs485_serial->voltagePhaseA;
    voltagePhaseB   = rs485_serial->voltagePhaseB;
    voltagePhaseC   = rs485_serial->voltagePhaseC;
    currentPhaseA   = rs485_serial->currentPhaseA;
    frequency       = rs485_serial->frequency;
    resistance      = voltagePhaseA / currentPhaseA;
    voltageA_edit->setText( QString::number( voltagePhaseA ) );
    voltageB_edit->setText( QString::number( voltagePhaseB ) );
    voltageC_edit->setText( QString::number( voltagePhaseC ) );
    currentA_edit->setText( QString::number( currentPhaseA ) );
    frequency_edit->setText( QString::number( frequency ) );
    resistance_edit->setText( QString::number( resistance ) );

}

