#include "forcewindow.h"

#include <QSerialPort>
#include <QDebug>

// Force terminal message settings
const QString end_str = ")";
const QString start_str = "=";

#define WATCHDOG_TIMEOUT 5000

ForceWindow::ForceWindow( Port* ext_port, QWidget* parent ) : QMainWindow( parent ),
    hasConnection(false)
{
    port = ext_port;
    m_settingsDialog = new SettingsDialog(this);
    connect( port, &Port::outPort, this, &ForceWindow::updateForce );


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
    if ( str == start_str ) {
//        qDebug() << "force_str :" << force_str;
        emit setForceValue( force_str );
        force_str.clear();
    }
    if ( str == end_str ) {
        force_str.append( str );
    } else if ( str != end_str && str != start_str ) {
        force_str.append( str );
    }

    resetWatchDog();
    hasConnection = true;
}

bool ForceWindow::isAlive()
{
    return hasConnection;
}

void ForceWindow::gotTimeout()
{
    if ( port->isOpened() ) {
        qDebug() << "watchDog force";
        hasConnection = false;
        emit lostConnection();
    }
}
