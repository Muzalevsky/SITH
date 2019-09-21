#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QThread>
#include <QLabel>
#include <QCoreApplication>
#include <QSpinBox>

#include <QInputDialog>
#include <QDir>

/***********************************************/
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow( parent ),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->tabWidget->setCurrentIndex(0);

    setWindowTitle( tr("Программа испытаний соленоида") );
    setWindowIcon( QIcon(":/images/pipes.png") );
    force_serial    = new Port();
    force_ui        = new ForceWindow( force_serial, this );
    rs485_serial    = new ModbusListener();
    stepper_serial  = new Port();
    stepper_ui      = new StepperControl( stepper_serial, this);
    encoder_port    = new Port();
    encoder_ui      = new EncoderControl( encoder_port, this );

    // Initialization
    stop_flag           = false;
    voltagePhaseA       = 0;
    voltagePhaseB       = 0;
    voltagePhaseC       = 0;
    currentPhaseA       = 0;
    currentPhaseB       = 0;
    currentPhaseC       = 0;
    frequency           = 0;
    resistance          = 0;
    manual_step_number  = 0;
    auto_step_number    = 0;
    stepNumber          = 0;
    force_kg            = 0;

    // Mode choosing
    ui->modeComboBox->addItem( "Ручной" );
    ui->modeComboBox->addItem( "Полуавтомат" );
    ui->modeComboBox->addItem( "Автомат" );
    connect( ui->modeComboBox, SIGNAL( currentIndexChanged(int) ), this, SLOT( modeChanged(int) ) );
    ui->modeComboBox->setCurrentIndex( 2 );

    connect( ui->autoStopButton, &QPushButton::clicked, this, &MainWindow::updateStopFlag );
    ui->autoStopButton->setChecked(false);

    connect( ui->setZeroPositionButton, &QPushButton::clicked, encoder_ui, &EncoderControl::setZeroPosition );
    connect( ui->setZeroForceButton, &QPushButton::clicked, this, &MainWindow::setZeroForce );

//    connect( ui->lineSwitchButton, &QPushButton::clicked, this, &MainWindow::changeLineSwitch );
    // Settings saving
    connect( ui->saveSettingsButton, &QPushButton::clicked, this, &MainWindow::writeSettings );
    connect( ui->loadSettingsButton, &QPushButton::clicked, this, &MainWindow::loadSettings );


    // Auto mode calculation
    connect( ui->workingStrokeBox, SIGNAL( valueChanged(double) ), this,
             SLOT( calculateStepNumber(double) ) );
    connect( ui->stepSizeBox, SIGNAL( valueChanged(double) ), this,
             SLOT( calculateStepNumber(double) ) );

    // Manual mode calculation
    connect( ui->manualStepBox, SIGNAL( valueChanged(double) ), this,
             SLOT( updateManualStep(double) ) );

    // Current time and date ticker
    update_timer = new QTimer();
    update_timer->setInterval( 500 );
    connect( update_timer, SIGNAL( timeout() ), this, SLOT( updateTime() ) );
    update_timer->start();

//    settings = new QSettings(  ORGANIZATION_NAME, APPLICATION_NAME );
    settings = new QSettings( "settings.ini", QSettings::IniFormat );

    connect( ui->searchSerialButton, &QPushButton::clicked, this, &MainWindow::searchSerialPorts );
    connect( ui->forceConnectButton, &QPushButton::clicked, this, &MainWindow::connect_clicked );
    connect( ui->stepperConnectButton, &QPushButton::clicked, this, &MainWindow::connect_clicked );
    connect( ui->modbusConnectButton, &QPushButton::clicked, this, &MainWindow::connect_clicked );
    connect( ui->encoderConnectButton, &QPushButton::clicked, this, &MainWindow::connect_clicked );

    connect( encoder_port, &Port::isConnected, this, &MainWindow::changeButtonText );
    connect( force_serial, &Port::isConnected, this, &MainWindow::changeButtonText );
    connect( stepper_serial, &Port::isConnected, this, &MainWindow::changeButtonText );
    connect( rs485_serial, &ModbusListener::isConnected, this, &MainWindow::changeButtonText );

    connect( ui->modbusSlaveBox, SIGNAL( valueChanged(int) ), rs485_serial, SLOT( updateSlaveNumber(int) ) );

    // Settings dialogs
    connect( ui->forceOptionsButton, &QPushButton::clicked,
              force_ui->m_settingsDialog, &QDialog::show);
    connect( ui->stepperOptionsButton, &QPushButton::clicked,
           stepper_ui->m_settingsDialog, &QDialog::show);    
    connect( ui->modbusSettingsButton, &QPushButton::clicked,
            rs485_serial->m_settingsDialog, &QDialog::show);
    connect( ui->encoderSettingsButton, &QPushButton::clicked,
            encoder_ui->m_settingsDialog, &QDialog::show);

    // Work block
    connect( ui->stepForwardButton, SIGNAL( pressed() ), stepper_ui, SLOT( stepForward() ) );
    connect( ui->stepBackwardButton, SIGNAL( pressed() ), stepper_ui, SLOT( stepBackward() ) );
    connect( this, SIGNAL( doStepForward() ), stepper_ui, SLOT( stepForward() ) );
    connect( this, SIGNAL( doStepBackward() ), stepper_ui, SLOT( stepBackward() ) );
    connect( ui->autoStartButton, SIGNAL( pressed() ), this, SLOT( startAuto() ) );
//    connect(ui->stepperspeedEdit, SIGNAL( textChanged(QString) ), this, SLOT( updateSpeedLimit(QString)));
    connect( ui->stepSizeBox, SIGNAL( valueChanged(double) ),
             stepper_ui, SLOT( updateStepNumber(double) ) );
    connect( ui->manualStepBox, SIGNAL( valueChanged(double) ),
             stepper_ui, SLOT( updateStepNumber(double) ) );
    connect( encoder_ui, &EncoderControl::updateUpperLevelPosition, this, &MainWindow::updatePosition );
    connect( force_ui, &ForceWindow::setForceValue, this, &MainWindow::updateForceValue );

    connect( ui->lineSwitchButton, SIGNAL( clicked(bool)), stepper_ui, SLOT( lineSwitchClicked(bool) ) );
    connect( stepper_ui, &StepperControl::isLineSwitchOn, this, &MainWindow::changeButtonText );

    //Force value thread
    QThread *thread_Force = new QThread;
    force_serial->moveToThread(thread_Force);
    connect(force_serial, SIGNAL(finished_Port()), thread_Force, SLOT(deleteLater()));
    connect(force_serial, SIGNAL(finished_Port()), thread_Force, SLOT(quit()));
    connect(thread_Force, SIGNAL(started()), force_serial, SLOT(process_Port()));//Переназначения метода run
    connect(thread_Force, SIGNAL(finished()), force_serial, SLOT(deleteLater()));//Удалить к чертям поток
    thread_Force->start();

    //Encoder value thread
    QThread *thread_Encoder = new QThread;
    encoder_port->moveToThread(thread_Encoder);
    connect(encoder_port, SIGNAL(finished_Port()), thread_Encoder, SLOT(deleteLater()));
    connect(encoder_port, SIGNAL(finished_Port()), thread_Encoder, SLOT(quit()));
    connect(thread_Encoder, SIGNAL(started()), encoder_port, SLOT(process_Port()));//Переназначения метода run
    connect(thread_Encoder, SIGNAL(finished()), encoder_port, SLOT(deleteLater()));//Удалить к чертям поток
    thread_Encoder->start();

    //Modbus thread
    QThread *thread_Modbus = new QThread;
    rs485_serial->moveToThread(thread_Modbus);
    connect( rs485_serial, &ModbusListener::getReply, this, &MainWindow::updateElectricParameters);
    connect( rs485_serial, &ModbusListener::getTemperature, this, &MainWindow::updateTemperature);

    connect(thread_Modbus, SIGNAL(finished()), rs485_serial, SLOT(deleteLater()));//Удалить к чертям поток
    thread_Modbus->start();

    //Stepper thread
    QThread *thread_Stepper = new QThread;
    stepper_serial->moveToThread(thread_Stepper);
    connect(stepper_serial, SIGNAL(finished_Port()), thread_Force, SLOT(deleteLater()));
    connect(stepper_serial, SIGNAL(finished_Port()), thread_Force, SLOT(quit()));
    connect(thread_Stepper, SIGNAL(started()), stepper_serial, SLOT(process_Port()));//Переназначения метода run
    connect(thread_Stepper, SIGNAL(finished()), stepper_serial, SLOT(deleteLater()));//Удалить к чертям поток
    thread_Stepper->start();


    /*
     *
     *
     * DEBUG
     */
    connect( ui->relayOnButton, &QPushButton::clicked, stepper_ui, &StepperControl::relayOn );
    connect( ui->relayOffButton, &QPushButton::clicked, stepper_ui, &StepperControl::relayOff );

    /*
     * Authorization
     */
    bool ok;
    QString userName = QInputDialog::getText(this, tr("Авторизация"),
                                         tr("Имя:"), QLineEdit::Normal,
                                         nullptr, &ok);
    if (ok && !userName.isEmpty()){
        qDebug() << "Successful login" << userName;
        ui->operatorNameEdit->setText(userName);
    } else {
        qDebug() << "Failed to login";
        exit(EXIT_FAILURE);
    }

}

/***********************************************/
MainWindow::~MainWindow()
{
    delete ui;
}

/****************************/
/*     Updates block        */
/****************************/

void MainWindow::setZeroForce()
{
    force_kg = 0;
}

void MainWindow::updateForceValue( QString str )
{
    ui->forceEdit->setText( str );
}

/***********************************************/
void MainWindow::updatePosition()
{
    currentPosition = encoder_ui->final_position_mm;

    if ( ui->currentPositionEdit->text().toFloat() != currentPosition ) {
        ui->positionEdit->setText( QString::number(currentPosition, 'f') );
        ui->currentPositionEdit->setText( QString::number(currentPosition, 'f') );
    }
}

/***********************************************/
void MainWindow::updateElectricParameters()
{
    voltagePhaseA   = rs485_serial->voltagePhaseA;
    voltagePhaseB   = rs485_serial->voltagePhaseB;
    voltagePhaseC   = rs485_serial->voltagePhaseC;
    currentPhaseA   = rs485_serial->currentPhaseA;
    currentPhaseB   = rs485_serial->currentPhaseB;
    currentPhaseC   = rs485_serial->currentPhaseC;

    frequency       = rs485_serial->frequency;
    resistance      = voltagePhaseA / currentPhaseA;

    ui->voltageAEdit->setText( QString::number( voltagePhaseA ) );
    ui->voltageBEdit->setText( QString::number( voltagePhaseB ) );
    ui->voltageCEdit->setText( QString::number( voltagePhaseC ) );
    ui->currentAEdit->setText( QString::number( currentPhaseA ) );
//    ui->currentBEdit->setText( QString::number( currentPhaseB ) );
//    ui->currentCEdit->setText( QString::number( currentPhaseC ) );
    ui->frequencyEdit->setText( QString::number( frequency ) );
    ui->resistanceEdit->setText( QString::number( resistance ) );
}

void MainWindow::updateTemperature()
{
    temperature = rs485_serial->temperature;
    ui->temperatureEdit->setText( QString::number(temperature) );
}

/***********************************************/
void MainWindow::connect_clicked()
{
    if ( sender() == ui->forceConnectButton ) {
        force_ui->portName = ui->forceComboBox->currentText();
        force_ui->saveSettings();
        qDebug() << "force port " << force_ui->portName;
        force_serial->connect_clicked();
    } else if ( sender() == ui->stepperConnectButton ) {
        stepper_ui->portName = ui->stepperComboBox->currentText();
        stepper_ui->saveSettings();
        qDebug() << "stepper port " << stepper_ui->portName;
        stepper_serial->connect_clicked();
        stepper_ui->sendPassword();

    } else if ( sender() == ui->modbusConnectButton ) {
        rs485_serial->portName = ui->modbusComboBox->currentText();
        qDebug() << "modbus port " << rs485_serial->portName;
        rs485_serial->on_connectButton_clicked();

    } else if ( sender() == ui->encoderConnectButton ) {
        encoder_ui->portName = ui->encoderBox->currentText();
        encoder_ui->saveSettings();
        qDebug() << "encoder port " << encoder_ui->portName;
        encoder_port->connect_clicked();
    }
}

/***********************************************/
void MainWindow::changeButtonText(bool state)
{
    if ( sender() == force_serial ) {
        if ( state ) {
            ui->forceConnectButton->setText( "Отключиться" );
        } else {
            ui->forceConnectButton->setText( "Подлючиться" );
        }
    } else if ( sender() == stepper_serial  ) {
        if ( state ) {
            ui->stepperConnectButton->setText( "Отключиться" );
        } else {
            ui->stepperConnectButton->setText( "Подлючиться" );
        }
    } else if ( sender() == stepper_ui  ) {
        if ( state ) {
            ui->lineSwitchButton->setText( "Снять напряжение" );
        } else {
            ui->lineSwitchButton->setText( "Подать напряжение" );
        }
    } else if ( sender() == rs485_serial ) {
        if ( state ) {
            ui->modbusConnectButton->setText( "Отключиться" );
        } else {
            ui->modbusConnectButton->setText( "Подлючиться" );
        }
    } else if ( sender() == encoder_port ) {
        if ( state ) {
            ui->encoderConnectButton->setText( "Отключиться" );
        } else {
            ui->encoderConnectButton->setText( "Подлючиться" );
        }
    }
}

/***********************************************/
void MainWindow::updateTime()
{
    QString str;
    QDate date;
    QTime time;
    str.append( date.currentDate().toString() );
    str.append(" " );
    str.append( time.currentTime().toString() );
    ui->dateTimeEdit->setText( str );
}

/***********************************************/
void MainWindow::updateStopFlag( bool checked )
{
    stop_flag = checked;
    qDebug() << "STOP state: " << stop_flag;
}

/***********************************************/
void MainWindow::setOperatorName(QString name)
{
    ui->operatorNameEdit->setText(name);
}

/***********************************************/




void MainWindow::modeChanged( int mode )
{
    // 0 manual, 1 semiautomated, 2 automatic
    switch ( mode ) {
        case 0:
        ui->autoStartButton->setEnabled( false );
        ui->reverseCheckBox->setEnabled( false );
        ui->stepSizeBox->setEnabled( false );
        ui->workingStrokeBox->setEnabled( false );
        ui->manualGroupBox->setEnabled( true );
        break;
    case 1:
        break;
    case 2:
    default:
        ui->autoStartButton->setEnabled( true );
        ui->reverseCheckBox->setEnabled( true );
        ui->stepSizeBox->setEnabled( true );
        ui->workingStrokeBox->setEnabled( true );
        ui->manualGroupBox->setEnabled( false );


        break;
    }
}

/***********************************************/
void MainWindow::searchSerialPorts()
{
    ui->forceComboBox->clear();
    ui->modbusComboBox->clear();
    ui->stepperComboBox->clear();
    ui->encoderBox->clear();

    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        ui->forceComboBox->addItem(info.portName());
        ui->modbusComboBox->addItem(info.portName());
        ui->stepperComboBox->addItem(info.portName());
        ui->encoderBox->addItem(info.portName());
    }
    qDebug() << "Port search.";

}


void MainWindow::setSerialSettings()
{

}

/***********************************************/


/*
 * Here starts automatic mode
*/
void MainWindow::startAuto()
{
    int i = 0;
    ui->pointsLeftEdit->setText( QString::number( stepNumber - i ) );
    stepper_ui->step_number = auto_step_number;

    file = new QFile(ui->protocolEdit->text() + ".txt" );
    file->open(QIODevice::ReadWrite | QIODevice::Text);
    bool dir_flag = ui->reverseCheckBox->isChecked();

    qDebug() << "Number of points: " << stepNumber;
    printProtocolHeader();
    printString();
    // Zero position
    for ( i = 0; i <= stepNumber; i++ )
    {
        // Если нет СТОПа, то делаем шаг
        if ( !stop_flag ) {
            if ( dir_flag ) {
                emit doStepForward(); // вверх
            } else {
                emit doStepBackward(); // вниз
            }
            printString();
            ui->pointsLeftEdit->setText( QString::number( stepNumber - i ) );
            spec_delay();
        }
    }
    file->close();
}

/***********************************************/
void MainWindow::printProtocolHeader()
{
    QString str;
    QDate date;
    QTime time;

    QTextStream stream(file);
    stream << date.currentDate().toString()
           << " " << time.currentTime().toString()
           << "  \n"
           << " " << "Operator name \t" << ui->operatorNameEdit->text()
           << "  \n"
           << " " << "Solenoid type \t" << ui->solenoidTypeEdit->text()
           << "  \n"
           << " " << "Step size \t" << QString::number( ui->stepSizeBox->value() )
           << "  \n"
           << " " << "Working stroke \t" << QString::number( ui->workingStrokeBox->value() )
           << "  \n";
}

/***********************************************/
void MainWindow::printString()
{
    QTime time;
    QTextStream stream(file);

    stream << " " << time.currentTime().toString()
           << " \t" << ui->forceEdit->text()
           << " \t" << ui->currentPositionEdit->text()
           << " \t" << currentPhaseA
           << "  \n";
}

/***********************************************/
void MainWindow::spec_delay()
{
    QTime dieTime= QTime::currentTime().addSecs(2);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

/***********************************************/
void MainWindow::calculateStepNumber( double val )
{
    if ( ui->stepSizeBox->value() != 0 ) {
        stepNumber = ui->workingStrokeBox->value() /
            ui->stepSizeBox->value();
    }

    ui->stepNumberEdit->setText( QString::number( stepNumber ) );
    double steps_n = ui->stepSizeBox->value() * stepper_ui->step_per_mm;
    auto_step_number = static_cast<uint32_t>( ceil( steps_n ) );
}

/*
 * Settings
*/
void MainWindow::writeSettings()
{
    settings->setValue("app/pos", pos());
    settings->setValue("app/size", size());

    settings->setValue( "modbus/port", ui->modbusComboBox->currentText() );
    settings->setValue( "modbus/baud", QString::number( rs485_serial->m_settingsDialog->settings().baud ) );

    settings->setValue( "stepper/port", ui->stepperComboBox->currentText() );
    settings->setValue( "stepper/baud", QString::number( stepper_ui->m_settingsDialog->settings().baud ) );

    settings->setValue( "force/port", ui->forceComboBox->currentText() );
    settings->setValue( "force/baud", QString::number( force_ui->m_settingsDialog->settings().baud ) );

    settings->setValue( "encoder/port", ui->encoderBox->currentText() );
    settings->setValue( "encoder/baud", QString::number( encoder_ui->m_settingsDialog->settings().baud ) );

    settings->sync();
    qDebug() << "Настройки сохранены";

}

/***********************************************/
void MainWindow::loadSettings()
{
    //Application
    QPoint pos = settings->value("app/pos", QPoint(100, 100)).toPoint();
    QSize size = settings->value("app/size", QSize(1000, 400)).toSize();
    resize(size);
    move(pos);

    searchSerialPorts();

    //Modbus
    rs485_serial->portName = settings->value( "modbus/port" ).toString();
    int modbus_index = ui->modbusComboBox->findText( rs485_serial->portName );
    ui->modbusComboBox->setCurrentIndex( modbus_index );
    int modbus_baud = settings->value( "modbus/baud" ).toInt();
    rs485_serial->m_settingsDialog->setBaud( modbus_baud );
    qDebug() << "modbus" << rs485_serial->portName << " /baud " << modbus_baud;


    //Stepper
    stepper_serial->SettingsPort.name = settings->value( "stepper/port" ).toString();
    int stepper_index = ui->stepperComboBox->findText( stepper_serial->SettingsPort.name );
    ui->stepperComboBox->setCurrentIndex( stepper_index );
    int stepper_baud = settings->value( "stepper/baud" ).toInt();
    stepper_ui->m_settingsDialog->setBaud( stepper_baud );
    qDebug() << "stepper" << stepper_serial->SettingsPort.name << " /baud " << stepper_baud;


    //Force
//    force_ui->portName = settings->value( "force/port" ).toString();
    force_serial->SettingsPort.name = settings->value( "force/port" ).toString();
    int force_index = ui->forceComboBox->findText( force_serial->SettingsPort.name );
    ui->forceComboBox->setCurrentIndex( force_index );
    int force_baud = settings->value( "force/baud" ).toInt();
    force_ui->m_settingsDialog->setBaud( force_baud );
    qDebug() << "force" << force_serial->SettingsPort.name << " /baud " << force_baud;


    //Encoder
//    encoder_ui->portName = settings->value( "encoder/port" ).toString();
    encoder_port->SettingsPort.name = settings->value( "encoder/port" ).toString();
    int encoder_index = ui->encoderBox->findText( encoder_port->SettingsPort.name );
    ui->encoderBox->setCurrentIndex( encoder_index );
    int encoder_baud = settings->value( "encoder/baud" ).toInt();
    encoder_ui->m_settingsDialog->setBaud( encoder_baud );
    qDebug() << "encoder" << encoder_port->SettingsPort.name << " /baud " << encoder_baud;


    qDebug() << "Настройки считаны";
//    qDebug() << "modbus port" << rs485_serial->portName;
//    qDebug() << "stepper port" << stepper_serial->SettingsPort.name;
//    qDebug() << "force port" << force_serial->SettingsPort.name;
//    qDebug() << "force port" << force_serial->SettingsPort.name;


}


/*********************************************/

/*
 * Manual mode
*/
void MainWindow::updateManualStep( double step_mm )
{
    double steps_n = step_mm * stepper_ui->step_per_mm;
    manual_step_number = static_cast<uint32_t>( ceil( steps_n ) );
    stepper_ui->step_number = manual_step_number;
    qDebug() << "Manual step update:" << stepper_ui->step_number;
}

/***********************************************/




//void MainWindow::changeLineSwitch()
//{
//    if ( ui->lineSwitchButton->isChecked() ) {
//        ui->lineSwitchButton->setText( tr("Снять напряжение") );
//        stepper_ui->slotSetRele();
//    } else {
//        ui->lineSwitchButton->setText( tr("Подать напряжение") );
//        stepper_ui->slotClearRele();
//    }
//}

