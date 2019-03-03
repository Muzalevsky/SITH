#include "forcewindow.h"

#include <QSerialPort>
#include <QDebug>

// Force terminal message settings
const QString end_str = ")";
const QString start_str = "=";

ForceWindow::ForceWindow( Port* ext_port, QWidget* parent ) : QMainWindow( parent )
{
    port = ext_port;
    m_settingsDialog = new SettingsDialog(this);

    connect( port, &Port::outPort, this, &ForceWindow::updateForce );

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

void ForceWindow::updateForce( QString str )
{
    if ( str == start_str ) {
        qDebug() << "force_str :" << force_str;
        emit setForceValue( force_str );
        force_str.clear();
    }
    if ( str == end_str ) {
        force_str.append( str );
    } else if ( str != end_str && str != start_str ) {
        force_str.append( str );
    }

}
