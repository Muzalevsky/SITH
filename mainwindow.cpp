#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QCoreApplication>
#include <QDir>
#include <QInputDialog>
#include <QMessageBox>
#include <QTextCodec>
#include <QThread>

#define MSEC_BETWEEN_STEPS 1500
#define STATEOBSERVER_INTERVAL_MSEC 300

//#define DISABLE_ENCODER_CHECKING

/***********************************************/
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow( parent ),
    ui(new Ui::MainWindow),
    manual_step_number(0),
    auto_step_number(0),
    voltagePhaseA(0),
    voltagePhaseB(0),
    voltagePhaseC(0),
    currentPhaseA(0),
    currentPhaseB(0),
    currentPhaseC(0),
    resistance(0),
    frequency(0),
    temperature(0),
    force_kg(0),
    stepNumber(0),
    stepDoneNumber(0),
    isBusy(false)
{
    ui->setupUi(this);
    ui->tabWidget->setCurrentIndex(0);
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QTextCodec::setCodecForLocale(codec);

    // Authorization
    bool ok;
    QString userName = QInputDialog::getText(this, tr("Вход"),
                                         tr("Имя:"), QLineEdit::Normal,
                                         nullptr, &ok);
    if (ok && !userName.isEmpty()){
        qDebug() << "Successful login" << userName;
        ui->operatorNameEdit->setText(userName);
    } else {
        qDebug() << "Failed to login";
        exit(EXIT_FAILURE);
    }

    // Initialization
    ui->currentPositionEdit->setText(QString::number(0.000000, 'f'));
    ui->motorPositionEdit->setText(QString::number(0.000000, 'f'));


    force_ui            = new ForceWindow();
    rs485_serial        = new ModbusListener();
    stepper_ui          = new StepperControl();
    encoder_ui          = new EncoderControl();

    // Mode choosing
    ui->modeComboBox->addItem( "Ручной" );
    ui->modeComboBox->addItem( "Полуавтомат" );
    ui->modeComboBox->addItem( "Автомат" );
    connect( ui->modeComboBox, SIGNAL( currentIndexChanged(int) ), this, SLOT( modeChanged(int) ) );
    ui->modeComboBox->setCurrentIndex( 1 );


//    connect(ui->connectAllDevicesButton, &QPushButton::clicked, this, &MainWindow::connectToAllDevices);


    // Settings saving
    connect( ui->saveSettingsButton, &QPushButton::clicked, this, &MainWindow::writeSettings );
    connect( ui->loadSettingsButton, &QPushButton::clicked, this, &MainWindow::loadSettings );
    //    settings = new QSettings(  ORGANIZATION_NAME, APPLICATION_NAME );
    settings = new QSettings( "settings.ini", QSettings::IniFormat );


    /*
     * AUTOSTOP callback
     */
    ui->autoStopButton->setChecked(true);            
    stop_flag           = ui->autoStopButton->isChecked();
    connect( ui->autoStopButton, &QPushButton::clicked, this, &MainWindow::updateStopFlag );
    updateStopFlag(stop_flag);


    /*
     * Zero all positions
     */
    connect( ui->setZeroPositionButton, &QPushButton::clicked,
             encoder_ui, &EncoderControl::setZeroPosition, Qt::QueuedConnection );

    connect( ui->setZeroPositionButton, &QPushButton::clicked,
             stepper_ui, &StepperControl::resetMotorPosition, Qt::QueuedConnection );


    /*
     * Auto mode parameters
     */
    connect( ui->workingStrokeBox, SIGNAL( valueChanged(double) ),
             this, SLOT( calculateStepNumber(double) ) );
    connect( ui->stepSizeBox, SIGNAL( valueChanged(double) ),
             this, SLOT( calculateStepNumber(double) ) );


    /*
     * Manual mode parameters
     */
    connect( ui->manualStepBox, SIGNAL( valueChanged(double) ),
             this, SLOT( updateManualStep(double) ) );

    /*
     * Work block
     */
    // Local callback with STOP checking
    connect( ui->stepForwardButton, SIGNAL( pressed() ), this, SLOT( gotStepForward() ) );
    connect( ui->stepBackwardButton, SIGNAL( pressed() ), this, SLOT( gotStepBackward() ) );

    // Remote slot for real motor move
    connect( this, SIGNAL( doStepForward() ),
             stepper_ui, SLOT( stepForward() ), Qt::QueuedConnection );
    connect( this, SIGNAL( doStepBackward() ),
             stepper_ui, SLOT( stepBackward() ), Qt::QueuedConnection );

    // Reset motor supply so we can move it manually
    connect( this, SIGNAL( resetStepperSupply() ),
             stepper_ui, SLOT( resetMotorSupply() ), Qt::QueuedConnection );

    //TODO This thing doesn't work. Think about closeEvent()
    connect( this, SIGNAL( goingToClose() ),
             stepper_ui, SLOT( disableElectricity() ), Qt::QueuedConnection );

    /*
     * Measurement mode
     */
    // Start measurements
    connect( ui->autoStartButton, SIGNAL( pressed() ), this, SLOT( startAuto() ) );

    /*
     * BUG: if somebody sets auto step value
     * then update manual step size - we measure
     * with manual step
     */
    connect( ui->stepSizeBox, SIGNAL( valueChanged(double) ),
             stepper_ui, SLOT( updateStepNumber(double) ), Qt::QueuedConnection );
    connect( ui->manualStepBox, SIGNAL( valueChanged(double) ),
             stepper_ui, SLOT( updateStepNumber(double) ), Qt::QueuedConnection );


    connect( stepper_ui, SIGNAL( updatePos(QString)),
             ui->motorPositionEdit, SLOT(setText(QString)), Qt::QueuedConnection );



    connect( encoder_ui, &EncoderControl::updateUpperLevelPosition,
             this, &MainWindow::updatePosition, Qt::QueuedConnection );
    connect( force_ui, &ForceWindow::setForceValue,
             this, &MainWindow::updateForceValue, Qt::QueuedConnection );
    connect( ui->lineSwitchButton, &QPushButton::clicked, this, &MainWindow::lineSwitchClicked );
    connect( stepper_ui, &StepperControl::isLineSwitchOn,
             this, &MainWindow::changeButtonText, Qt::QueuedConnection );

    // Current time and date ticker
    measuringTimeoutTimer = new QTimer(this);
    measuringTimeoutTimer->setInterval(MSEC_BETWEEN_STEPS);
    connect( measuringTimeoutTimer, SIGNAL( timeout() ), this, SLOT( measureNextPoint() ) );

    update_timer = new QTimer(this);
    update_timer->setInterval(STATEOBSERVER_INTERVAL_MSEC);
    connect( update_timer, SIGNAL( timeout() ), this, SLOT( updateTime() ) );
    connect( update_timer, SIGNAL( timeout() ), this, SLOT( updateState() ) );

//    motorSupplyOffTimer = new QTimer(this);
//    connect( overForceTimer, SIGNAL( timeout() ), stepper_ui, SLOT( resetMotorSupply()) );
//    motorSupplyOffTimer->singleShot(2000, stepper_ui, SLOT( resetMotorSupply()));

    connect(this, &MainWindow::tooMuchForce,
            stepper_ui, &StepperControl::resetMotorSupply, Qt::QueuedConnection);


    // Serials buttons
    connect( ui->searchSerialButton, &QPushButton::clicked, this, &MainWindow::searchSerialPorts );
    connect( ui->forceConnectButton, &QPushButton::clicked, this, &MainWindow::connect_clicked );
    connect( ui->stepperConnectButton, &QPushButton::clicked, this, &MainWindow::connect_clicked );
    connect( ui->modbusConnectButton, &QPushButton::clicked, this, &MainWindow::connect_clicked );
    connect( ui->encoderConnectButton, &QPushButton::clicked, this, &MainWindow::connect_clicked );

    connect( encoder_ui->port, &Port::connectionStateChanged,
             this, &MainWindow::changeButtonText, Qt::QueuedConnection );
    connect( force_ui->port, &Port::connectionStateChanged,
             this, &MainWindow::changeButtonText, Qt::QueuedConnection );
    connect( stepper_ui->port, &Port::connectionStateChanged,
             this, &MainWindow::changeButtonText, Qt::QueuedConnection );
    connect( rs485_serial, &ModbusListener::isConnected,
             this, &MainWindow::changeButtonText, Qt::QueuedConnection );

    connect( ui->modbusSlaveBox, SIGNAL( valueChanged(int) ), rs485_serial, SLOT( updateSlaveNumber(int) ) );

    // Settings dialogs
    connect( ui->forceOptionsButton, &QPushButton::clicked,
              force_ui->m_settingsDialog, &QDialog::show, Qt::QueuedConnection);
    connect( ui->stepperOptionsButton, &QPushButton::clicked,
           stepper_ui->m_settingsDialog, &QDialog::show, Qt::QueuedConnection);
    connect( ui->modbusSettingsButton, &QPushButton::clicked,
            rs485_serial->m_settingsDialog, &QDialog::show, Qt::QueuedConnection);
    connect( ui->encoderSettingsButton, &QPushButton::clicked,
            encoder_ui->m_settingsDialog, &QDialog::show, Qt::QueuedConnection);

    /*
     * The main idea is to process ports in different threads
     */
    //Force value thread
    QThread *thread_Force = new QThread;
    force_ui->moveToThread(thread_Force);
    connect(force_ui->port, SIGNAL(finished_Port()), thread_Force, SLOT(deleteLater()));
    connect(force_ui->port, SIGNAL(finished_Port()), thread_Force, SLOT(quit()));
    connect(thread_Force, SIGNAL(started()), force_ui->port, SLOT(process_Port()));//Переназначения метода run
    connect(thread_Force, SIGNAL(finished()), force_ui->port, SLOT(deleteLater()));//Удалить к чертям поток
    thread_Force->start();

    //Encoder value thread
    QThread *thread_Encoder = new QThread;
    encoder_ui->moveToThread(thread_Encoder);
    connect(encoder_ui->port, SIGNAL(finished_Port()), thread_Encoder, SLOT(deleteLater()));
    connect(encoder_ui->port, SIGNAL(finished_Port()), thread_Encoder, SLOT(quit()));
    connect(thread_Encoder, SIGNAL(started()), encoder_ui->port, SLOT(process_Port()));//Переназначения метода run
    connect(thread_Encoder, SIGNAL(finished()), encoder_ui->port, SLOT(deleteLater()));//Удалить к чертям поток
    thread_Encoder->start();

    //Modbus IS IN CURRENT thread
    connect( rs485_serial, &ModbusListener::getReply, this, &MainWindow::updateElectricParameters);
    connect( rs485_serial, &ModbusListener::getTemperature, this, &MainWindow::updateTemperature);

    //Stepper thread
    QThread *thread_Stepper = new QThread;
    stepper_ui->moveToThread(thread_Stepper);
    connect(stepper_ui->port, SIGNAL(finished_Port()), thread_Force, SLOT(deleteLater()));
    connect(stepper_ui->port, SIGNAL(finished_Port()), thread_Force, SLOT(quit()));
    connect(thread_Stepper, SIGNAL(started()), stepper_ui->port, SLOT(process_Port()));//Переназначения метода run
    connect(thread_Stepper, SIGNAL(finished()), stepper_ui->port, SLOT(deleteLater()));//Удалить к чертям поток
    thread_Stepper->start();

    /*
     * DEBUG
     */
    connect( ui->relayOnButton, &QPushButton::clicked,
             stepper_ui, &StepperControl::relayOn, Qt::QueuedConnection );
    connect( ui->relayOffButton, &QPushButton::clicked,
             stepper_ui, &StepperControl::relayOff, Qt::QueuedConnection );

    connect( this, &MainWindow::reconnectForceWindow,
             force_ui->port, &Port::reconnectPort, Qt::QueuedConnection );
    connect( this, &MainWindow::reconnectModbus, rs485_serial,
             &ModbusListener::reconnectModbus );
    connect( this, &MainWindow::reconnectStepper,
             stepper_ui, &StepperControl::reconnectStepper, Qt::QueuedConnection );

#ifndef DISABLE_ENCODER_CHECKING
    connect( this, &MainWindow::reconnectEncoder,
             encoder_ui->port, &Port::reconnectPort, Qt::QueuedConnection );
#endif

    /*
     * Here we restore serial devices and window geometry
     */
    loadSettings();

    update_timer->start();

    qDebug() << "Force thread" << thread_Force;
    qDebug() << "Encoder thread" << thread_Encoder;
    qDebug() << "Stepper thread" << thread_Stepper;


//    connectToAllDevices();
}

/***********************************************/
MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    writeSettings();
    emit goingToClose();

    event->accept();
}

/****************************/
/*     Updates block        */
/****************************/
void MainWindow::updateForceValue( float forceValue )
{
    force_kg = forceValue;

//    qDebug() << "got updateForceValue()" << force_kg;

    if ( abs(force_kg) > static_cast<float>(ui->forceLimitBox->value()) ) {
//        emit tooMuchForce();
        setStateButtonColor( Qt::yellow );
    }

    ui->forceEdit->setText( QString::number(force_kg));
}

/***********************************************/
void MainWindow::updatePosition(float new_position)
{
    currentPosition = new_position;
    ui->currentPositionEdit->setText( QString::number(new_position, 'f') );

    qDebug() << "got updatePosition()" << currentPosition;
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

    power           = rs485_serial->power;

    ui->voltageAEdit->setText( QString::number( voltagePhaseA ) );
    ui->voltageBEdit->setText( QString::number( voltagePhaseB ) );
    ui->voltageCEdit->setText( QString::number( voltagePhaseC ) );
    ui->currentAEdit->setText( QString::number( currentPhaseA ) );
    ui->currentBEdit->setText( QString::number( currentPhaseB ) );
    ui->currentCEdit->setText( QString::number( currentPhaseC ) );
    ui->frequencyEdit->setText( QString::number( frequency ) );
    ui->resistanceEdit->setText( QString::number( resistance ) );
    ui->powerEdit->setText( QString::number( power ) );

}

/***********************************************/
void MainWindow::updateTemperature()
{
    temperature = rs485_serial->temperature;
    ui->temperatureEdit->setText( QString::number(temperature) );
}

/***********************************************/
void MainWindow::connect_clicked()
{
    if ( sender() == ui->forceConnectButton )
    {
            force_ui->portName = ui->forceComboBox->currentText();
            force_ui->saveSettings();
            qDebug() << "force port " << force_ui->portName;
        force_ui->port->connect_clicked();
    }
    else if ( sender() == ui->stepperConnectButton )
    {
        stepper_ui->portName = ui->stepperComboBox->currentText();
        stepper_ui->saveSettings();
        qDebug() << "stepper port " << stepper_ui->portName;
        stepper_ui->port->connect_clicked();
        stepper_ui->sendPassword();
        stepper_ui->getRelayState();

    }
    else if ( sender() == ui->modbusConnectButton )
    {
        rs485_serial->portName = ui->modbusComboBox->currentText();
        qDebug() << "modbus port " << rs485_serial->portName;
        rs485_serial->on_connectButton_clicked();

    }
    else if ( sender() == ui->encoderConnectButton )
    {
        encoder_ui->portName = ui->encoderBox->currentText();
        encoder_ui->saveSettings();
        qDebug() << "encoder port " << encoder_ui->portName;
        encoder_ui->port->connect_clicked();
    }
}

/***********************************************/
void MainWindow::changeButtonText(bool state)
{
    if ( sender() == force_ui->port )
    {
        if ( state ) {
            ui->forceConnectButton->setText( "Отключить" );
        } else {
            ui->forceConnectButton->setText( "Подключить" );
        }
    }
    else if ( sender() == stepper_ui->port  )
    {
        if ( state ) {
            ui->stepperConnectButton->setText( "Отключить" );
        } else {
            ui->stepperConnectButton->setText( "Подключить" );
        }
    }
    else if ( sender() == rs485_serial )
    {
        if ( state ) {
            ui->modbusConnectButton->setText( "Отключить" );
        } else {
            ui->modbusConnectButton->setText( "Подключить" );
        }
    }
    else if ( sender() == encoder_ui->port )
    {
        if ( state ) {
            ui->encoderConnectButton->setText( "Отключить" );
        } else {
            ui->encoderConnectButton->setText( "Подключить" );
        }
    }
    else if ( sender() == stepper_ui  )
    {
            if ( state ) {
                ui->lineSwitchButton->setText( "Снять напряжение" );
            } else {
                ui->lineSwitchButton->setText( "Подать напряжение" );
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
/*
 * Every STATEOBSERVER_INTERVAL_MSEC milliseconds we
 * are checking state of our stand.
 * If somesthing's wrong - setColor!
*/
void MainWindow::updateState()
{
    /*
     * Firstly check if all the ports are opened
     */
    // Always check without MessageBox, but setting color
    int res = checkConnectionStatesMuted();
    if ( res != 0 ) {
        setStateButtonColor( Qt::red );
        qDebug() << "checkConnectionStatesMuted" << res;
        return;
    }

    /*
     * Then if they are OPEN we check for data in them
     */
    // Checking if all are sending something, if no - reconnect
    res = isSerialAlive();
    if ( res != 0 ) {
        setStateButtonColor( Qt::blue );
        qDebug() << "isSerialAlive" << res;
        return;
    }

    /*
     * If ports are OPEN and HAS DATA we do the SAFETY check
     */
    if ( abs(force_kg) > static_cast<float>(ui->forceLimitBox->value()) ) {
        setStateButtonColor( Qt::yellow );
        return;
    }

    //Finally everything's OK
    setStateButtonColor( Qt::green );
}

/***********************************************/
void MainWindow::updateStopFlag( bool checked )
{
    stop_flag = checked;
    qDebug() << "STOP state: " << stop_flag;
    if (stop_flag) {
        ui->autoStopButton->setStyleSheet( "background-color: red;");


        ui->autoStartButton->setEnabled(false);
        ui->stepBackwardButton->setEnabled(false);
        ui->stepForwardButton->setEnabled(false);
        emit resetStepperSupply();
    } else {
        ui->autoStopButton->setStyleSheet( "background-color: green;");

        ui->autoStartButton->setEnabled(true);
        ui->stepBackwardButton->setEnabled(true);
        ui->stepForwardButton->setEnabled(true);
    }
}

/***********************************************/
void MainWindow::modeChanged( int mode )
{
    // 0 manual, 1 semiautomated, 2 automatic
    switch ( mode ) {
        case WorkingModes::MANUAL_MODE:
        ui->autoStartButton->setEnabled( false );
        ui->reverseCheckBox->setEnabled( false );
        ui->stepSizeBox->setEnabled( false );
        ui->workingStrokeBox->setEnabled( false );

        ui->manualStepBox->setEnabled(true);
        ui->stepForwardButton->setEnabled(true);
        ui->stepBackwardButton->setEnabled(true);

        ui->manualStepBox->setVisible(true);
        ui->manualModeLabel->setVisible(true);
        ui->stepSizeLabel->setVisible(true);
        ui->stepForwardButton->setVisible(true);
        ui->stepBackwardButton->setVisible(true);

        break;
    case WorkingModes::SEMIAUTO_MODE :
        ui->manualStepBox->setEnabled(true);
        ui->stepForwardButton->setEnabled(true);
        ui->stepBackwardButton->setEnabled(true);
        ui->autoStartButton->setEnabled( true );
        ui->reverseCheckBox->setEnabled( true );
        ui->stepSizeBox->setEnabled( true );
        ui->workingStrokeBox->setEnabled( true );

        ui->manualStepBox->setVisible(true);
        ui->manualModeLabel->setVisible(true);
        ui->stepSizeLabel->setVisible(true);
        ui->stepForwardButton->setVisible(true);
        ui->stepBackwardButton->setVisible(true);
        break;
    case WorkingModes::FULLAUTO_MODE :
    default:
        ui->autoStartButton->setEnabled( true );
        ui->reverseCheckBox->setEnabled( true );
        ui->stepSizeBox->setEnabled( true );
        ui->workingStrokeBox->setEnabled( true );

        ui->manualStepBox->setEnabled(false);
        ui->stepForwardButton->setEnabled(false);
        ui->stepBackwardButton->setEnabled(false);

        ui->manualStepBox->setVisible(false);
        ui->manualModeLabel->setVisible(false);
        ui->stepSizeLabel->setVisible(false);
        ui->stepForwardButton->setVisible(false);
        ui->stepBackwardButton->setVisible(false);
        break;
    }
}


/***********************************************/
void MainWindow::setOperatorName(QString name)
{
    ui->operatorNameEdit->setText(name);
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
/***********************************************/

/****************************/
/*   Manual mode block      */
/****************************/
void MainWindow::updateManualStep( double step_mm )
{
    double steps_n = step_mm * stepper_ui->step_per_mm;
    manual_step_number = static_cast<uint32_t>( ceil( steps_n ) );
    stepper_ui->step_number = manual_step_number;
    qDebug() << "Manual step update:" << stepper_ui->step_number;
}

void MainWindow::gotStepBackward()
{
    if (stop_flag)
        return;

    emit doStepBackward();
}

void MainWindow::gotStepForward()
{
    if (stop_flag)
        return;

    emit doStepForward();
}


/****************************/
/*   Automatic mode block   */
/****************************/
void MainWindow::startAuto()
{
    if ( checkProtocolHeader() != 0 ) {
        qDebug() << "Invalid protocol header";
        return;
    }

    if ( checkConnectionStates() != 0 ) {
        qDebug() << "Invalid connection states";
        return;
    }

    ui->pointsLeftEdit->setText( QString::number( stepNumber ) );
    stepper_ui->step_number = auto_step_number;

    QString protoDirPath = QApplication::applicationDirPath() + "/protocols";
    QDir protoDir;
    if ( !protoDir.exists(protoDirPath) )
        protoDir.mkpath(protoDirPath);

    QString protoFilePath;
    protoFilePath = protoDirPath + "/" + ui->protocolNameEdit->text();

    protocolCsvFile = new QFile( protoFilePath + ".csv"  );
    protocolCsvFile->open(QIODevice::ReadWrite | QIODevice::Text);

    protocolFile = new QFile(protoFilePath + ".txt" );
    protocolFile->open(QIODevice::ReadWrite | QIODevice::Text);

    qDebug() << "Number of points: " << stepNumber;
    printProtocolHeader();
    printCsvHeader();

    stepDoneNumber = 0;

    measuringTimeoutTimer->start();
    isBusy = true;
}

/***********************************************/
void MainWindow::measureNextPoint()
{
    bool dir_flag = ui->reverseCheckBox->isChecked();
    /*
     * Защита от перегрузки в автоматическом режиме
     */
    if ( abs(force_kg) > ui->forceLimitBox->value() )
    {
        setStateButtonColor( Qt::yellow );

        if ( dir_flag ) {
            emit doStepBackward(); // Отступить на шаг вниз
        } else {
            emit doStepForward(); // Отступить на шаг вверх
        }

        endMeasuring();
        QMessageBox msgBox;
        msgBox.setText("Превышен предел силы!");
        msgBox.exec();
        return;
    }

    /*
     * Если на предыдущем шаге мы закончили,
     * то подождав 2 секунды, мы выйдем здесь
     */
    if (stepDoneNumber == stepNumber) {
        endMeasuring();
        return;
    }

    // Если нет СТОПа, то делаем шаг
    if ( !stop_flag ) {
        printString();
        printCsvString();

        if ( dir_flag ) {
            emit doStepForward(); // вверх
        } else {
            emit doStepBackward(); // вниз
        }

        stepDoneNumber++;
        ui->pointsLeftEdit->setText( QString::number( stepNumber - stepDoneNumber ) );
        setStateButtonColor( Qt::green );
    }
    else
    {
        /*
         * STOP
         */
        endMeasuring();
        return;
    }

    qDebug() << "stepDoneNumber/stepNumber" << stepDoneNumber << "/" << stepNumber;

    // Снова взводим таймер для следующего шага
    if (stepDoneNumber < stepNumber)  {
        measuringTimeoutTimer->start();
    }

    return;
}

/***********************************************/
void MainWindow::endMeasuring()
{
    qDebug() << "got end measuring";
    measuringTimeoutTimer->stop();

    ui->autoStopButton->setChecked(true);
    ui->autoStopButton->setStyleSheet( "background-color: red;");

    updateStopFlag(true);

    if (protocolFile) {
        protocolFile->close();
        delete protocolFile;
    }

    if (protocolCsvFile) {
        protocolCsvFile->close();
        delete protocolCsvFile;
    }

    /*
     * After we have finished auto or pressed STOP
     * we restore manual step instead of auto
     */
    stepper_ui->step_number = manual_step_number;

    isBusy = false;
}

/***********************************************/
void MainWindow::calculateStepNumber( double val )
{
    Q_UNUSED(val);

    if ( ui->stepSizeBox->value() != 0 ) {
        stepNumber = static_cast<int>(ui->workingStrokeBox->value() / ui->stepSizeBox->value() );
        ui->stepNumberEdit->setText( QString::number( stepNumber ) );
    }

    double steps_n = ui->stepSizeBox->value() * stepper_ui->step_per_mm;
    auto_step_number = static_cast<uint32_t>( ceil( steps_n ) );
}

/****************************/
/*     Protocol block       */
/****************************/
int MainWindow::checkProtocolHeader()
{
    if ( ui->protocolNameEdit->text().isEmpty() ) {
        QMessageBox msgBox;
        msgBox.setText(tr("Введите название протокола"));
        msgBox.exec();
        return -1;
    } else if ( ui->operatorNameEdit->text().isEmpty() ) {
        QMessageBox msgBox;
        msgBox.setText(tr("Введите имя оператора"));
        msgBox.exec();
        return -2;
    } else if ( ui->solenoidTypeEdit->text().isEmpty() ) {
        QMessageBox msgBox;
        msgBox.setText(tr("Введите тип соленоида"));
        msgBox.exec();
        return -3;
    } else if ( ui->stepSizeBox->value() == 0 ) {
       QMessageBox msgBox;
       msgBox.setText(tr("Введите шаг испытаний"));
       msgBox.exec();
       return -4;
    } else if ( ui->workingStrokeBox->value() == 0 ) {
       QMessageBox msgBox;
       msgBox.setText(tr("Введите величину рабочего хода"));
       msgBox.exec();
       return -5;
    } else {
        return 0;
    }
}

/***********************************************/
/*
 * If all devices are connected than we'll get OK
*/
int MainWindow::checkConnectionStatesMuted()
{
    if ( !force_ui->port->isOpened() ) {
        emit reconnectForceWindow();
        return -1;
    }

    if ( !rs485_serial->isModbusConnected() ) {
        emit reconnectModbus();
        return -3;
    }

#ifndef DISABLE_ENCODER_CHECKING
    if ( !encoder_ui->port->isOpened() ) {
       emit reconnectEncoder();
       return -4;
    }
#endif
    if ( !stepper_ui->port->isOpened() ) {
        emit reconnectStepper();
        return -2;
    }

    return 0;
}

int MainWindow::checkConnectionStates()
{
    if ( !force_ui->port->isOpened() ) {
       QMessageBox msgBox;
       msgBox.setText("Отсутствует связь с весовым терминалом");
       msgBox.exec();
       return -1;
    }

    if ( !stepper_ui->port->isOpened() ) {
       QMessageBox msgBox;
       msgBox.setText("Отсутствует связь с драйвером двигателя");
       msgBox.exec();
       return -2;
    }

    if ( !rs485_serial->isModbusConnected() ) {
       QMessageBox msgBox;
       msgBox.setText("Отсутствует интерфейс RS485");
       msgBox.exec();
       return -3;
    }

#ifndef DISABLE_ENCODER_CHECKING
    if ( !encoder_ui->port->isOpened() ) {
       QMessageBox msgBox;
       msgBox.setText("Отсутствует связь с энкодером");
       msgBox.exec();
       return -4;
    }
#endif

    return 0;
}

/***********************************************/
int MainWindow::isSerialAlive()
{
    if ( !force_ui->isAlive() ) {
        qDebug() << "Отсутствует связь с весовым терминалом";
        emit reconnectForceWindow();
        return -1;
    }

    if ( !rs485_serial->isModbusAlive() ) {
        qDebug() << "Отсутствует связь RS485";
        emit reconnectModbus();
        return -3;
    }

#ifndef DISABLE_ENCODER_CHECKING
    if ( !encoder_ui->isAlive() ) {
        qDebug() << "Отсутствует связь с энкодером";
        emit reconnectEncoder();
        return -4;
    }
#endif

    if ( !stepper_ui->isAuthorized() ) {
        qDebug() << "Отсутствует связь с драйвером ШД";
        emit reconnectStepper();
        return -2;
    }

    return 0;
}

void MainWindow::setStateButtonColor(int colorCode)
{
    switch (colorCode) {
        default:
        case Qt::red:
            ui->connectionStateButton->setStyleSheet( "background-color: red;");
            break;

        case Qt::yellow:
            ui->connectionStateButton->setStyleSheet( "background-color: yellow;");
            break;

        case Qt::green:
            ui->connectionStateButton->setStyleSheet( "background-color: green;");
            break;
        case Qt::blue:
            ui->connectionStateButton->setStyleSheet( "background-color: blue;");
            break;
    }
}

/***********************************************/
void MainWindow::printProtocolHeader()
{
    QDate date;
    QTime time;
    QTextStream stream(protocolFile);
    stream << date.currentDate().toString()
           << " " << time.currentTime().toString() << "  \n"
           << " " << QString(tr("Оператор")).leftJustified(20) << ui->operatorNameEdit->text() << "  \n"
           << " " << QString(tr("Тип соленоида")).leftJustified(20) << ui->solenoidTypeEdit->text() << "  \n"
           << " " << QString(tr("Шаг испытаний, мм")).leftJustified(20) << QString::number( ui->stepSizeBox->value() ) << "  \n"
           << " " << QString(tr("Рабочий ход, мм")).leftJustified(20) << QString::number( ui->workingStrokeBox->value() ) << "  \n";

    stream << "-----------------------------------------" << "\n";

    //Table headers
    stream << tr("Время     ") << tr("Положение, мм  ")
           << tr("Усилие, кг     ") << tr("Ток фазы А, А  ")
           << tr("t, °C   ") << "\n";
}

/***********************************************/
void MainWindow::printString()
{
    QTime time;
    QTextStream stream(protocolFile);

    stream << time.currentTime().toString()
           << "  " << QString::number(currentPosition).leftJustified(9)
           << "      " << QString::number(force_kg).leftJustified(9)
           << "      " << QString::number(currentPhaseA).leftJustified(9)
           << "      " << QString::number(temperature).leftJustified(9) << "\n";
}


/***********************************************/
void MainWindow::printCsvHeader()
{
    QDate date;
    QTime time;

    QTextStream stream(protocolCsvFile);
    stream.setCodec("system");
    stream << date.currentDate().toString()
           << " " << time.currentTime().toString() << "  \n"
           << " " << tr("Оператор;") << ui->operatorNameEdit->text() << "  \n"
           << " " << tr("Тип соленоида;") << ui->solenoidTypeEdit->text() << "  \n"
           << " " << tr("Шаг испытаний;") << QString::number( ui->stepSizeBox->value() ) << "  \n"
           << " " << tr("Рабочий ход;") << QString::number( ui->workingStrokeBox->value() ) << "  \n";

    //Table headers
    stream << " " << tr("Время;") << tr("Положение, мм;")
           << tr("Тяговое усилие, кг;") << tr("Ток фазы А, А;")
           << tr("Температура, градус") << "\n";
}

/***********************************************/
void MainWindow::printCsvString()
{
    QTime time;
    QTextStream stream(protocolCsvFile);
    stream.setCodec("UTF-8");

    stream << " " << time.currentTime().toString()
           << ";" << currentPosition
           << ";" << force_kg
           << ";" << currentPhaseA
           << ";" << temperature << "\n";
}

/****************************/
/*  Serial setting block    */
/****************************/
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
    stepper_ui->port->SettingsPort.name = settings->value( "stepper/port" ).toString();
    int stepper_index = ui->stepperComboBox->findText( stepper_ui->port->SettingsPort.name );
    ui->stepperComboBox->setCurrentIndex( stepper_index );
    int stepper_baud = settings->value( "stepper/baud" ).toInt();
    stepper_ui->m_settingsDialog->setBaud( stepper_baud );
    qDebug() << "stepper" << stepper_ui->port->SettingsPort.name << " /baud " << stepper_baud;

    //Force
    force_ui->port->SettingsPort.name = settings->value( "force/port" ).toString();
    int force_index = ui->forceComboBox->findText( force_ui->port->SettingsPort.name );
    ui->forceComboBox->setCurrentIndex( force_index );
    int force_baud = settings->value( "force/baud" ).toInt();
    force_ui->m_settingsDialog->setBaud( force_baud );
    qDebug() << "force" << force_ui->port->SettingsPort.name << " /baud " << force_baud;

    //Encoder
    encoder_ui->port->SettingsPort.name = settings->value( "encoder/port" ).toString();
    int encoder_index = ui->encoderBox->findText( encoder_ui->port->SettingsPort.name );
    ui->encoderBox->setCurrentIndex( encoder_index );
    int encoder_baud = settings->value( "encoder/baud" ).toInt();
    encoder_ui->m_settingsDialog->setBaud( encoder_baud );
    qDebug() << "encoder" << encoder_ui->port->SettingsPort.name << " /baud " << encoder_baud;
    qDebug() << "Настройки считаны";


    force_ui->portName = ui->forceComboBox->currentText();
    force_ui->saveSettings();

    stepper_ui->portName = ui->stepperComboBox->currentText();
    stepper_ui->saveSettings();

    rs485_serial->portName = ui->modbusComboBox->currentText();

    encoder_ui->portName = ui->encoderBox->currentText();
    encoder_ui->saveSettings();
}

/*********************************************/
void MainWindow::lineSwitchClicked()
{
    if (ui->lineSwitchButton->isChecked()) {
        stepper_ui->relayOn();
    } else {
        stepper_ui->relayOff();
    }
}

/***********************************************/
void MainWindow::connectToAllDevices()
{

//    force_ui->port->connect_clicked();
//    stepper_ui->port->connect_clicked();
//    stepper_ui->sendPassword();

//    rs485_serial->on_connectButton_clicked();
//    encoder_ui->port->connect_clicked();
}
