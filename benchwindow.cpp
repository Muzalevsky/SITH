#include "benchwindow.h"

#include <QThread>
#include <QLabel>
BenchWindow::BenchWindow(QWidget *parent) : QMainWindow( parent )
{
    initializeWindow();

    force_serial = new Port();
    force_ui = new ForceWindow( force_serial, this);

    timer = new QTimer();
//    timer->setInterval(1000);

//    connect( timer, &QTimer::timeout, this, &BenchWindow::updateForceEdit );


    rs485_serial = new ModbusListener();
    connect( rs485_serial, &ModbusListener::getReply, this, &BenchWindow::getVoltage);

    stepper_serial = new Port();
    force_serial->setPortSettings( QString( "//./COM5" ), 9600, 8, 0, 0, 0 );
//    rs485_serial->setPortSettings( QString( "//./COM3" ), 9600, 8, 0, 0, 0 );
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
    grid_layout         = new QGridLayout();
    forceEdit = new QLineEdit();
    forceEdit->setReadOnly( true );

    force_serialButton = new QPushButton( "Force" );
    connect( force_serialButton, SIGNAL( pressed() ), this, SLOT( openForce() ) );
    rs485_serialButton = new QPushButton( "RS485" );
    connect( rs485_serialButton, SIGNAL( pressed() ), this, SLOT( openModbus() ) );

    stepper_serialButton = new QPushButton( "Stepper" );

    QLabel *voltage_lbl = new QLabel( "Voltage: " );
    QLabel *frequency_lbl = new QLabel( "Frequency: " );
    QLabel *current_lbl = new QLabel( "Current: " );
    QLabel *resistance_lbl = new QLabel( "Resistance: " );

    voltageA_edit = new QLineEdit();
    voltageB_edit = new QLineEdit();
    voltageC_edit = new QLineEdit();
    frequency_edit = new QLineEdit();

    QLineEdit *current_edit = new QLineEdit();
    QLineEdit *resistance_edit = new QLineEdit();

    voltageA_edit->setReadOnly( true );
    voltageB_edit->setReadOnly( true );
    voltageC_edit->setReadOnly( true );
    frequency_edit->setReadOnly( true );

    current_edit->setReadOnly( true );
    resistance_edit->setReadOnly( true );

    QPushButton *stepForward = new QPushButton( "Step +" );
    connect( stepForward, SIGNAL( pressed() ), this, SLOT( sendStepForward() )  );
    QPushButton *stepBackward = new QPushButton( "Step -" );
    connect( stepBackward, SIGNAL( pressed() ), this, SLOT( sendStepBackward() )  );

    grid_layout->addWidget( force_serialButton, 1, 1, 1, 1 );
    grid_layout->addWidget( rs485_serialButton, 1, 2, 1, 1 );
    grid_layout->addWidget( stepper_serialButton, 1, 3, 1, 1 );
    grid_layout->addWidget( forceEdit, 2, 1, 1, 1 );

    grid_layout->addWidget( voltage_lbl, 2, 4, 1, 1 );
    grid_layout->addWidget( voltageA_edit, 3, 4, 1, 1 );
    grid_layout->addWidget( voltageB_edit, 4, 4, 1, 1 );
    grid_layout->addWidget( voltageC_edit, 5, 4, 1, 1 );
    grid_layout->addWidget( frequency_lbl, 6, 4, 1, 1 );
    grid_layout->addWidget( frequency_edit, 7, 4, 1, 1 );

    grid_layout->addWidget( current_lbl, 2, 5, 1, 1 );
    grid_layout->addWidget( current_edit, 3, 5, 1, 1 );
    grid_layout->addWidget( resistance_lbl, 2, 6, 1, 1 );
    grid_layout->addWidget( resistance_edit, 3, 6, 1, 1 );


    grid_layout->addWidget( stepForward, 3, 1, 1, 1 );
    grid_layout->addWidget( stepBackward, 3, 2, 1, 1 );

    grid_layout->setHorizontalSpacing( 10 );
    grid_layout->setVerticalSpacing( 10 );

    QWidget* mainWidget = new QWidget;
    mainWidget->setLayout( grid_layout );
    setCentralWidget( mainWidget );
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
    voltagePhaseA = rs485_serial->voltagePhaseA;
    voltagePhaseB = rs485_serial->voltagePhaseB;
    voltagePhaseC = rs485_serial->voltagePhaseC;
    frequency = rs485_serial->frequency;

    voltageA_edit->setText( QString::number(voltagePhaseA) );
    voltageB_edit->setText( QString::number(voltagePhaseB) );
    voltageC_edit->setText( QString::number(voltagePhaseC) );
    frequency_edit->setText( QString::number(frequency) );

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

