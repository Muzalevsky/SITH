#include "forcewindow.h"

#include <QSerialPort>
#include <QDebug>
ForceWindow::ForceWindow( Port* ext_port, QWidget* parent ) : QMainWindow( parent )
{
    port = ext_port;
    gridLayout = new QGridLayout();
    m_settingsDialog = new SettingsDialog(this);
    PortNameBox = new QComboBox();

    QPushButton *optionsButton = new QPushButton( "Options" );
    connect( optionsButton, &QPushButton::clicked, m_settingsDialog, &QDialog::show);
    gridLayout->addWidget( optionsButton, 3,1,1,1);

    gridLayout->addWidget(new QLabel("Force Port: "),1,1,1,1);
    gridLayout->addWidget(PortNameBox,1,2,1,1);

    QPushButton *setButton = new QPushButton( "Set" );
    connect( setButton, &QPushButton::clicked, this, &ForceWindow::saveSettings );
    gridLayout->addWidget( setButton, 4,1,1,1);

    QPushButton *openButton = new QPushButton( "Open" );
    connect( openButton, &QPushButton::clicked, port, &Port::openPort );
    gridLayout->addWidget( openButton, 4,2,1,1);

    QPushButton *searchButton = new QPushButton( "Search" );
    connect( searchButton, &QPushButton::clicked, this, &ForceWindow::searchPorts );
    gridLayout->addWidget( searchButton, 4,3,1,1);

    QWidget* mainWidget = new QWidget;
    mainWidget->setLayout( gridLayout );
    setCentralWidget( mainWidget );
    setWindowTitle( "Port settings" );

    connect(port, SIGNAL(outPort(QString)), this, SLOT(setForceValue(QString)));


}

void ForceWindow::searchPorts()
{
    PortNameBox->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        PortNameBox->addItem(info.portName());
    }
    qDebug() << "Force port search.";

}

void ForceWindow::saveSettings()
{
    port->setPortSettings( PortNameBox->currentText(),
                           m_settingsDialog->settings().baud,
                           m_settingsDialog->settings().dataBits,
                           m_settingsDialog->settings().parity,
                           m_settingsDialog->settings().stopBits,
                           m_settingsDialog->settings().flow );

    qDebug() << "New force port settings saved.";
}

void ForceWindow::setForceValue( QString str )
{
    QString end_str = ")";
    QString start_str = "=";

    if ( str == start_str ) {
        force_str.clear();
    }

    if ( str == end_str ) {
        force_str.append( str );
        emit updateForce( force_str );
    } else if ( str != end_str && str != start_str ) {
        force_str.append( str );
    }
}
