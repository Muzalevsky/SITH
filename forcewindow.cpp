#include "forcewindow.h"

#include <QSerialPort>
#include <QDebug>

ForceWindow::ForceWindow( Port* ext_port, QWidget* parent ) : QMainWindow( parent )
{
    port = ext_port;
    m_settingsDialog = new SettingsDialog(this);

}

void ForceWindow::saveSettings()
{
    port->setPortSettings( port_name,
                           m_settingsDialog->settings().baud,
                           m_settingsDialog->settings().dataBits,
                           m_settingsDialog->settings().parity,
                           m_settingsDialog->settings().stopBits,
                           m_settingsDialog->settings().flow );

    qDebug() << "New force port settings saved.";
}

