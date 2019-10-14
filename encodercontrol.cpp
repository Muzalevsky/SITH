#include "encodercontrol.h"

#include <QDebug>

const QString enc_end_str = "\r";
const QString enc_start_str = "\r";
//Mechanical parameters
const int encoder_pulse_per_rev = 4000;
const int encoder_pulse_limit = 64000;
const int rev_number_per_overfloat = encoder_pulse_limit / encoder_pulse_per_rev;

const int screw_step_mm = 5;

#define WATCHDOG_TIMEOUT 5000

EncoderControl::EncoderControl( Port* ext_port, QWidget* parent ) : QMainWindow( parent )
{
    port = ext_port;
    m_settingsDialog = new SettingsDialog(this);
    word_cnt = 0;
    zero_mark_number = 0;
    position_mm = 0;
    final_position_mm = 0;
    prev_position_raw = 0;
    position_raw = 0;
    delta = 0;
    speed_dir = 0;
    zero_position_offset = 0;
    overfloat_position = 0;

    connect( port, SIGNAL(outPort(QString) ), this, SLOT(updatePosition(QString) ) );
    connect( this, &EncoderControl::positionChanged, this, &EncoderControl::analyzePosition );

    QTimer *watchDogTimer = new QTimer(this);
    watchDogTimer->setInterval(WATCHDOG_TIMEOUT);
    connect(watchDogTimer, &QTimer::timeout, this, &EncoderControl::gotTimeout);
    connect(this, SIGNAL(resetWatchDog()), watchDogTimer, SLOT(start()));

}

void EncoderControl::updatePosition( QString str )
{
//    qDebug() << "Clear position from port" << str;
    if ( str == enc_start_str ) {
        position_str.clear();
        word_cnt = 5;
    } else {
        position_str.append(str);
        word_cnt--;
    }

    if ( word_cnt == 0 ) {
//        qDebug() << "position" << position_str;
        emit positionChanged( position_str );
    }

    resetWatchDog();
    hasConnection = true;
}

void EncoderControl::saveSettings()
{
    port->setPortSettings( portName,
                           m_settingsDialog->settings().baud,
                           m_settingsDialog->settings().dataBits,
                           m_settingsDialog->settings().parity,
                           m_settingsDialog->settings().stopBits,
                           m_settingsDialog->settings().flow );

    qDebug() << "New position port settings saved.";
}

void EncoderControl::analyzePosition( QString str )
{
    prev_position_raw = position_raw;
    position_raw = str.toInt();

//    qDebug() << "остаток: " << position_raw % encoder_pulse_per_rev;

    delta = (position_raw - prev_position_raw);
    // Определение направления вращения при переполнении
    if ( delta > encoder_pulse_limit / 2 ) {
        overfloat_position -= 1;
    }

    if ( delta < -encoder_pulse_limit / 2 ) {
        overfloat_position += 1;
    }

    position_mm = /*zero_position_offset +*/ overfloat_position * rev_number_per_overfloat * screw_step_mm +
            (static_cast<float>(position_raw) *
             screw_step_mm / encoder_pulse_per_rev);

    final_position_mm = zero_position_offset + position_mm;

    if ( delta != 0 ) {
        qDebug() << "prev:" << prev_position_raw
                 << "current:" << position_raw
                 << "delta: " << delta
                 << "overfloat_position:" << overfloat_position
                 << "position_mm:" << position_mm;
    }

    emit updateUpperLevelPosition();
}

void EncoderControl::setZeroPosition()
{
    zero_position_offset = -position_mm;
}

bool EncoderControl::isAlive()
{
    return hasConnection;
}

void EncoderControl::gotTimeout()
{
    if ( port->isOpened() ) {
        qDebug() << "watchDog encoder";
        hasConnection = false;
        emit lostConnection();
    }
}

