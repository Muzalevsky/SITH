#include "benchwindow.h"

#include <QThread>
#include <QLabel>

BenchWindow::BenchWindow(QWidget *parent) : QMainWindow( parent )
{
    initializeWindow();


    timer = new QTimer();
//    timer->setInterval(1000);
//    connect( timer, &QTimer::timeout, this, &BenchWindow::updateForceEdit );


    //Default
    force_serial->setPortSettings( QString( "//./COM5" ), 9600, 8, 0, 0, 0 );
    stepper_serial->setPortSettings( QString( "//./COM4" ), 115200, 8, 0, 0, 0 );

    force_str = "A";
    connect(force_serial, SIGNAL(outPort(QString)), this, SLOT(setForceValue(QString)));
    connect(force_serial, SIGNAL(error_(QString)), this, SLOT(errorHandler(QString)));

    //Force value thread
    QThread *thread_Force = new QThread;
    force_serial->moveToThread(thread_Force);
    force_serial->thisPort.moveToThread(thread_Force);
    connect(force_serial, SIGNAL(finished_Port()), thread_Force, SLOT(deleteLater()));
    connect(force_serial, SIGNAL(finished_Port()), thread_Force, SLOT(quit()));

    connect(thread_Force, SIGNAL(started()), force_serial, SLOT(process_Port()));//Переназначения метода run
    connect(thread_Force, SIGNAL(finished()), force_serial, SLOT(deleteLater()));//Удалить к чертям поток
    thread_Force->start();

    //Modbus thread
    QThread *thread_Modbus = new QThread;
    rs485_serial->moveToThread(thread_Modbus);
    connect(thread_Modbus, SIGNAL(finished()), rs485_serial, SLOT(deleteLater()));//Удалить к чертям поток
    thread_Modbus->start();

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

    rs485_serial = new ModbusListener();
    connect( rs485_serial, &ModbusListener::getReply, this, &BenchWindow::getVoltage);

    stepper_serial = new Port();
    stepper_ui = new StepperControl(this);

    layout_work     = new QGridLayout(workWidget);
    forceEdit       = new QLineEdit();
    forceEdit->setReadOnly( true );
    QLabel *force_lbl       = new QLabel( tr("Force") );
    QLabel *voltage_lbl     = new QLabel( tr("Voltage: ") );
    QLabel *frequency_lbl   = new QLabel( tr("Frequency: ") );
    QLabel *current_lbl     = new QLabel( tr("Current: ") );
    QLabel *resistance_lbl  = new QLabel( tr("Resistance: ") );

    voltageA_edit           = new QLineEdit();
    voltageB_edit           = new QLineEdit();
    voltageC_edit           = new QLineEdit();
    frequency_edit          = new QLineEdit();
    currentA_edit           = new QLineEdit();
    resistance_edit         = new QLineEdit();

    voltageA_edit->setReadOnly( true );
    voltageB_edit->setReadOnly( true );
    voltageC_edit->setReadOnly( true );
    frequency_edit->setReadOnly( true );
    currentA_edit->setReadOnly( true );
    resistance_edit->setReadOnly( true );

    QPushButton *stepForward = new QPushButton( "Step +" );
    connect( stepForward, SIGNAL( pressed() ), this, SLOT( sendStepForward() )  );
    QPushButton *stepBackward = new QPushButton( "Step -" );
    connect( stepBackward, SIGNAL( pressed() ), this, SLOT( sendStepBackward() )  );


    layout_work->addWidget( force_lbl, 1, 2, 1, 1 );
    layout_work->addWidget( forceEdit, 1, 3, 1, 1 );
    layout_work->addWidget( voltage_lbl, 2, 2, 1, 1 );
    layout_work->addWidget( voltageA_edit, 2, 3, 1, 1 );
    layout_work->addWidget( voltageB_edit, 2, 4, 1, 1 );
    layout_work->addWidget( voltageC_edit, 2, 5, 1, 1 );
    layout_work->addWidget( frequency_lbl, 3, 2, 1, 1 );
    layout_work->addWidget( frequency_edit, 3, 3, 1, 1 );
    layout_work->addWidget( current_lbl, 4, 2, 1, 1 );
    layout_work->addWidget( currentA_edit, 4, 3, 1, 1 );
    layout_work->addWidget( resistance_lbl, 5, 2, 1, 1 );
    layout_work->addWidget( resistance_edit, 5, 3, 1, 1 );
    layout_work->addWidget( stepForward, 6, 1, 1, 1 );
    layout_work->addWidget( stepBackward, 6, 2, 1, 1 );

    layout_work->setHorizontalSpacing( 10 );
    layout_work->setVerticalSpacing( 10 );
    workWidget->setLayout( layout_work );
    mainWgt = new QTabWidget(this);
    mainWgt->addTab( workWidget, tr("Work") );

    layout_tune             = new QGridLayout(tuneWidget);
    force_serialButton      = new QPushButton( "Force" );
    connect( force_serialButton, SIGNAL( pressed() ), this, SLOT( openForce() ) );
    rs485_serialButton      = new QPushButton( "RS485" );
    connect( rs485_serialButton, SIGNAL( pressed() ), this, SLOT( openModbus() ) );
    stepper_serialButton    = new QPushButton( "Stepper" );

    layout_tune->addWidget( force_ui, 1, 1, 1, 1 );
    layout_tune->addWidget( rs485_serial, 2, 1, 1, 1 );
    layout_tune->addWidget( stepper_ui, 1, 2, 1, 1 );
    tuneWidget->setLayout( layout_tune );

    mainWgt->addTab( tuneWidget, tr("Tune") );
    setCentralWidget( mainWgt );
    setWindowTitle( "Bench control" );
}

void BenchWindow::setForceValue( QString str )
{
    QString end_str = ")";
    QString start_str = "=";

    if ( str == start_str )
    {
        force_str.clear();
    }

    if ( str == end_str )
    {
        force_str.append( str );
        forceEdit->setText( force_str );

    }
    else if ( str != end_str && str != start_str )
    {
        force_str.append( str );
    }

}

void BenchWindow::updateForceEdit()
{
    forceEdit->setText( force_str );
}

void BenchWindow::getVoltage()
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

void BenchWindow::sendStepForward()
{
    QByteArray command;
    stepper_serial->WriteToPort( command );
    qDebug() << "step+";
}

void BenchWindow::sendStepBackward()
{
    QByteArray command;
    stepper_serial->WriteToPort( command );
    qDebug() << "step-";

}

void BenchWindow::openForce()
{
    force_ui->show();
}

void BenchWindow::openModbus()
{
    rs485_serial->show();
}

void BenchWindow::errorHandler( QString err )
{
    qDebug() << err.toLocal8Bit();
}

