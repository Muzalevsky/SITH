#include "forcewindow.h"

#include <QSerialPort>
#include <QDebug>

// Force terminal message settings
const QString end_str = "(";
const QString start_str = "=";

#define WATCHDOG_TIMEOUT 1300
#define FORCE_TERMINAL_WORD_SIZE 7

ForceWindow::ForceWindow(QObject *parent ) : QObject( parent ),
    isPortAlive(false)
{
    port = new Port();
    m_settingsDialog = new SettingsDialog();
    connect( port, &Port::outPort, this, &ForceWindow::updateForce );
    connect( this, &ForceWindow::lostConnection, port, &Port::reconnectPort );

    QTimer *watchDogTimer = new QTimer(this);
    watchDogTimer->setInterval(WATCHDOG_TIMEOUT);
    connect(watchDogTimer, &QTimer::timeout, this, &ForceWindow::gotTimeout);
    connect(this, SIGNAL(resetWatchDog()), watchDogTimer, SLOT(start()));

}

void ForceWindow::saveSettings()
{
    port->setPortSettings( portName,
                           m_settingsDialog->settings().baud,
                           m_settingsDialog->settings().dataBits,
                           m_settingsDialog->settings().parity,
                           m_settingsDialog->settings().stopBits,
                           m_settingsDialog->settings().flow );

    qDebug() << "New force port settings saved.";
}

// TODO need to check if this is
// appropriate and correct and working way to handle messages
void ForceWindow::updateForce( QString str )
{
//    qDebug() << "force" << str;

    if ( str == start_str ) {
        force_str.clear();
        symbols_left = FORCE_TERMINAL_WORD_SIZE;
    } else {
        force_str.append(str);
        symbols_left--;
    }

    if ( symbols_left == 0 ) {
        updateForceValue(force_str);
//        qDebug() << "force" << force_kg;

        emit setForceValue( force_kg );
        resetWatchDog();
        isPortAlive = true;
    }
}

bool ForceWindow::isAlive()
{
    return isPortAlive;
}

void ForceWindow::gotTimeout()
{
    qDebug() << "watchDog force";
    isPortAlive = false;
    emit lostConnection();
}

void ForceWindow::updateForceValue( QString str )
{
    bool ok = false;
    float force_tmp = str.toDouble(&ok);
    if (ok) {
        force_kg = force_tmp;
    }
}
