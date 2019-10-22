#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QCoreApplication>
#include <QDir>
#include <QInputDialog>
//#include <QLabel>
#include <QMessageBox>
//#include <QSpinBox>
#include <QTextCodec>
#include <QThread>

#define MSEC_BETWEEN_STEPS 2000
#define STATEOBSERVER_INTERVAL_MSEC 500

#define DEBUG_ENCODER

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

    // Initialization
    // TODO Think about the next problem: when starts button is checked, but manual updateStopFlag()is useless
    ui->autoStopButton->setChecked(true);
    stop_flag           = ui->autoStopButton->isChecked();

    force_serial        = new Port();
    force_serial->setPortOpenMode(QIODevice::ReadOnly);
    force_ui            = new ForceWindow( force_serial, this );
    rs485_serial        = new ModbusListener();
    stepper_serial      = new Port();
    stepper_serial->setPortOpenMode(QIODevice::ReadWrite);
    stepper_ui          = new StepperControl( stepper_serial, this);
    encoder_serial      = new Port();
    encoder_serial->setPortOpenMode(QIODevice::ReadOnly);
    encoder_ui          = new EncoderControl( encoder_serial, this );

    // Mode choosing
    ui->modeComboBox->addItem( "Ручной" );
    ui->modeComboBox->addItem( "Полуавтомат" );
    ui->modeComboBox->addItem( "Автомат" );
    connect( ui->modeComboBox, SIGNAL( currentIndexChanged(int) ), this, SLOT( modeChanged(int) ) );
    ui->modeComboBox->setCurrentIndex( 2 );

    // Settings saving
    connect( ui->saveSettingsButton, &QPushButton::clicked, this, &MainWindow::writeSettings );
    connect( ui->loadSettingsButton, &QPushButton::clicked, this, &MainWindow::loadSettings );
    //    settings = new QSettings(  ORGANIZATION_NAME, APPLICATION_NAME );
    settings = new QSettings( "settings.ini", QSettings::IniFormat );

    // Working cycle callbacks
    connect( ui->autoStopButton, &QPushButton::clicked, this, &MainWindow::updateStopFlag );
    updateStopFlag(ui->autoStopButton->isChecked());
    connect( ui->setZeroPositionButton, &QPushButton::clicked, encoder_ui, &EncoderControl::setZeroPosition );

    // Auto mode calculation
    connect( ui->workingStrokeBox, SIGNAL( valueChanged(double) ), this,
             SLOT( calculateStepNumber(double) ) );
    connect( ui->stepSizeBox, SIGNAL( valueChanged(double) ), this,
             SLOT( calculateStepNumber(double) ) );
    // Manual mode calculation
    connect( ui->manualStepBox, SIGNAL( valueChanged(double) ), this,
             SLOT( updateManualStep(double) ) );

    // Work block
    connect( ui->stepForwardButton, SIGNAL( pressed() ), this, SLOT( gotStepForward() ) );
    connect( ui->stepBackwardButton, SIGNAL( pressed() ), this, SLOT( gotStepBackward() ) );
    connect( this, SIGNAL( doStepForward() ), stepper_ui, SLOT( stepForward() ) );
    connect( this, SIGNAL( doStepBackward() ), stepper_ui, SLOT( stepBackward() ) );
    connect( this, SIGNAL( resetStepperSupply() ), stepper_ui, SLOT( resetMotorSupply() ) );
    connect( this, SIGNAL( goingToClose() ), stepper_ui, SLOT( disableElectricity() ) );
    connect( ui->autoStartButton, SIGNAL( pressed() ), this, SLOT( startAuto() ) );
    connect( ui->stepSizeBox, SIGNAL( valueChanged(double) ), stepper_ui, SLOT( updateStepNumber(double) ) );
    connect( ui->manualStepBox, SIGNAL( valueChanged(double) ), stepper_ui, SLOT( updateStepNumber(double) ) );
    connect( encoder_ui, &EncoderControl::updateUpperLevelPosition, this, &MainWindow::updatePosition );
    connect( force_ui, &ForceWindow::setForceValue, this, &MainWindow::updateForceValue );
    connect( ui->lineSwitchButton, &QPushButton::clicked, this, &MainWindow::lineSwitchClicked );
    connect( stepper_ui, &StepperControl::isLineSwitchOn, this, &MainWindow::changeButtonText );

    // Current time and date ticker
    measuringTimeoutTimer = new QTimer(this);
    measuringTimeoutTimer->setInterval(MSEC_BETWEEN_STEPS);
    connect( measuringTimeoutTimer, SIGNAL( timeout() ), this, SLOT( measureNextPoint() ) );

    update_timer = new QTimer(this);
    update_timer->setInterval(STATEOBSERVER_INTERVAL_MSEC);
    connect( update_timer, SIGNAL( timeout() ), this, SLOT( updateTime() ) );
    connect( update_timer, SIGNAL( timeout() ), this, SLOT( updateState() ) );

    motorSupplyOffTimer = new QTimer(this);
//    connect( overForceTimer, SIGNAL( timeout() ), stepper_ui, SLOT( resetMotorSupply()) );
    motorSupplyOffTimer->singleShot(2000, stepper_ui, SLOT( resetMotorSupply()));


    // Serials buttons
    connect( ui->searchSerialButton, &QPushButton::clicked, this, &MainWindow::searchSerialPorts );
    connect( ui->forceConnectButton, &QPushButton::clicked, this, &MainWindow::connect_clicked );
    connect( ui->stepperConnectButton, &QPushButton::clicked, this, &MainWindow::connect_clicked );
    connect( ui->modbusConnectButton, &QPushButton::clicked, this, &MainWindow::connect_clicked );
    connect( ui->encoderConnectButton, &QPushButton::clicked, this, &MainWindow::connect_clicked );

    connect( encoder_serial, &Port::connectionStateChanged, this, &MainWindow::changeButtonText );
    connect( force_serial, &Port::connectionStateChanged, this, &MainWindow::changeButtonText );
    connect( stepper_serial, &Port::connectionStateChanged, this, &MainWindow::changeButtonText );
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

    /*
     * The main idea is to process ports in different threads
     */
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
    encoder_serial->moveToThread(thread_Encoder);
    connect(encoder_serial, SIGNAL(finished_Port()), thread_Encoder, SLOT(deleteLater()));
    connect(encoder_serial, SIGNAL(finished_Port()), thread_Encoder, SLOT(quit()));
    connect(thread_Encoder, SIGNAL(started()), encoder_serial, SLOT(process_Port()));//Переназначения метода run
    connect(thread_Encoder, SIGNAL(finished()), encoder_serial, SLOT(deleteLater()));//Удалить к чертям поток
    thread_Encoder->start();

    //Modbus thread
//    QThread *thread_Modbus = new QThread;
////    rs485_serial->moveToThread(thread_Modbus);
    connect( rs485_serial, &ModbusListener::getReply, this, &MainWindow::updateElectricParameters);
    connect( rs485_serial, &ModbusListener::getTemperature, this, &MainWindow::updateTemperature);

//    connect(thread_Modbus, SIGNAL(finished()), rs485_serial, SLOT(deleteLater()));//Удалить к чертям поток
//    thread_Modbus->start();

    //Stepper thread
    QThread *thread_Stepper = new QThread;
    stepper_serial->moveToThread(thread_Stepper);
    connect(stepper_serial, SIGNAL(finished_Port()), thread_Force, SLOT(deleteLater()));
    connect(stepper_serial, SIGNAL(finished_Port()), thread_Force, SLOT(quit()));
    connect(thread_Stepper, SIGNAL(started()), stepper_serial, SLOT(process_Port()));//Переназначения метода run
    connect(thread_Stepper, SIGNAL(finished()), stepper_serial, SLOT(deleteLater()));//Удалить к чертям поток
    thread_Stepper->start();


    /*
     * DEBUG
     */
    connect( ui->relayOnButton, &QPushButton::clicked, stepper_ui, &StepperControl::relayOn );
    connect( ui->relayOffButton, &QPushButton::clicked, stepper_ui, &StepperControl::relayOff );

    connect( this, &MainWindow::reconnectForceWindow, force_serial, &Port::reconnectPort );
    connect( this, &MainWindow::reconnectEncoder, encoder_serial, &Port::reconnectPort );

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

    /*
     * Here we restore serial devices and window geometry
     */
    loadSettings();

    update_timer->start();

    qDebug() << "Force thread" << thread_Force;
    qDebug() << "Encoder thread" << thread_Encoder;
    qDebug() << "Stepper thread" << thread_Stepper;

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
void MainWindow::updateForceValue( QString str )
{
    if ( str.contains("=") ) {
        str.remove("=");
    }

    if ( str.contains("(kg)") ) {
        str.remove("(kg)");
    }

    ui->forceEdit->setText( str );
    force_kg = str.toDouble();
}

/***********************************************/
void MainWindow::updatePosition()
{
    currentPosition = encoder_ui->final_position_mm;

    if ( ui->currentPositionEdit->text().toFloat() != currentPosition ) {
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
    ui->currentBEdit->setText( QString::number( currentPhaseB ) );
    ui->currentCEdit->setText( QString::number( currentPhaseC ) );
    ui->frequencyEdit->setText( QString::number( frequency ) );
    ui->resistanceEdit->setText( QString::number( resistance ) );
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
        encoder_serial->connect_clicked();
    }
}

/***********************************************/
void MainWindow::changeButtonText(bool state)
{
    if ( sender() == force_serial ) {
        if ( state ) {
            ui->forceConnectButton->setText( "Отключить" );
        } else {
            ui->forceConnectButton->setText( "Подлючить" );
        }
    } else if ( sender() == stepper_serial  ) {
        if ( state ) {
            ui->stepperConnectButton->setText( "Отключить" );
        } else {
            ui->stepperConnectButton->setText( "Подлючить" );
        }
    } else if ( sender() == rs485_serial ) {
        if ( state ) {
            ui->modbusConnectButton->setText( "Отключить" );
        } else {
            ui->modbusConnectButton->setText( "Подлючить" );
        }
    } else if ( sender() == encoder_serial ) {
        if ( state ) {
            ui->encoderConnectButton->setText( "Отключить" );
        } else {
            ui->encoderConnectButton->setText( "Подлючить" );
        }
    } else if ( sender() == stepper_ui  ) {
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
    // Always check without MessageBox, but setting color
    if ( checkConnectionStatesMuted() != 0 ) {
        setStateButtonColor( Qt::red );
        return;
    }

    if ( abs(force_kg) > ui->forceLimitBox->value() ) {
        setStateButtonColor( Qt::yellow );
        return;
    }

    // Checking if all are sending something, if no - reconnect
    if ( isSerialAlive() != 0 ) {
        setStateButtonColor( Qt::blue );
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
        ui->autoStartButton->setEnabled(false);
        ui->stepBackwardButton->setEnabled(false);
        ui->stepForwardButton->setEnabled(false);
        emit resetStepperSupply();
    } else {
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

#ifndef DEBUG_ENCODER
    if ( checkConnectionStates() != 0 ) {
        qDebug() << "Invalid connection states";
        return;
    }

    int res = isSerialAlive();
    if ( res != 0 ) {
        setStateButtonColor( Qt::blue );
        qDebug() << "Failed to check if device is alive | res:" << res;
        QMessageBox msgBox;
        msgBox.setText("Потеряна связь с устройством!");
        msgBox.exec();
        return;
    }
#endif
    int i = 0;

    ui->pointsLeftEdit->setText( QString::number( stepNumber - i ) );
    stepper_ui->step_number = auto_step_number;

    protocolCsvFile = new QFile(ui->protocolNameEdit->text() + ".csv" );
    protocolCsvFile->open(QIODevice::ReadWrite | QIODevice::Text);

    protocolFile = new QFile(ui->protocolNameEdit->text() + ".txt" );
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
    /* Защита от перегрузки а автоматическом режиме */
    if ( abs(force_kg) > ui->forceLimitBox->value() ) {
        setStateButtonColor( Qt::yellow );
//        emit weAreGoingToBreakSensor();
        motorSupplyOffTimer->start();

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
    } else {
        endMeasuring();
        return;
    }

    qDebug() << "stepDoneNumber/stepNumber" << stepDoneNumber << "/" << stepNumber;

    if (stepDoneNumber < stepNumber)  {
        // Снова взводим таймер для следующего шага
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
    updateStopFlag(true);

    if (protocolFile) {
        protocolFile->close();
        delete protocolFile;
    }

    if (protocolCsvFile) {
        protocolCsvFile->close();
        delete protocolCsvFile;
    }

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
    if ( !force_serial->isOpened() )
       return -1;

    if ( !stepper_serial->isOpened() )
       return -2;

    if ( !rs485_serial->isModbusConnected() )
       return -3;

#ifndef DEBUG_ENCODER
    if ( !encoder_serial->isOpened() )
       return -3;
#endif
    return 0;
}

int MainWindow::checkConnectionStates()
{
    if ( !force_serial->isOpened() ) {
       QMessageBox msgBox;
       msgBox.setText("Отсутствует связь с весовым терминалом");
       msgBox.exec();
       return -1;
    }

    if ( !stepper_serial->isOpened() ) {
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

    if ( !encoder_serial->isOpened() ) {
       QMessageBox msgBox;
       msgBox.setText("Отсутствует связь с энкодером");
       msgBox.exec();
       return -3;
    }

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

//    if ( !encoder_ui->isAlive() ) {
//        qDebug() << "Отсутствует связь с энкодером";
//        emit reconnectEncoder();
//        return -3;
//    }

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
           << " " << tr("Оператор \t") << ui->operatorNameEdit->text() << "  \n"
           << " " << tr("Тип соленоида \t") << ui->solenoidTypeEdit->text() << "  \n"
           << " " << tr("Шаг испытаний \t") << QString::number( ui->stepSizeBox->value() ) << "  \n"
           << " " << tr("Рабочий ход \t") << QString::number( ui->workingStrokeBox->value() ) << "  \n";

    //Table headers
    stream << " " << tr("Время ") << tr("Положение, мм ")
           << tr("Тяговое усилие, кг ") << tr("Ток фазы А, А ")
           << tr("Температура, градус ") << "\n";

}

/***********************************************/
void MainWindow::printString()
{
    QTime time;
    QTextStream stream(protocolFile);
    stream << " " << time.currentTime().toString()
           << " \t" << force_kg
           << " \t" << currentPosition
           << " \t" << currentPhaseA
           << " \t" << temperature << "\n";
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
    encoder_serial->SettingsPort.name = settings->value( "encoder/port" ).toString();
    int encoder_index = ui->encoderBox->findText( encoder_serial->SettingsPort.name );
    ui->encoderBox->setCurrentIndex( encoder_index );
    int encoder_baud = settings->value( "encoder/baud" ).toInt();
    encoder_ui->m_settingsDialog->setBaud( encoder_baud );
    qDebug() << "encoder" << encoder_serial->SettingsPort.name << " /baud " << encoder_baud;
    qDebug() << "Настройки считаны";
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
