#include "forcewindow.h"

#include <QSerialPort>
#include <QDebug>
ForceWindow::ForceWindow( Port* ext_port, QWidget* parent ) : QMainWindow( parent )
{
    port = ext_port;
    gridLayout = new QGridLayout();

    PortNameBox = new QComboBox();
    BaudRateBox = new QComboBox();
    DataBitsBox = new QComboBox();
    ParityBox = new QComboBox();
    StopBitsBox = new QComboBox();
    FlowControlBox = new QComboBox();


    gridLayout->addWidget(new QLabel("Port: "),1,1,1,1);
    gridLayout->addWidget(new QLabel("Baud rate: "),2,1,1,1);
    gridLayout->addWidget(new QLabel("Data bits: "),3,1,1,1);
    gridLayout->addWidget(new QLabel("Parity: "),4,1,1,1);
    gridLayout->addWidget(new QLabel("Stop bits: "),5,1,1,1);
    gridLayout->addWidget(new QLabel("Flow control: "),6,1,1,1);

    gridLayout->addWidget(PortNameBox,1,2,1,1);
    gridLayout->addWidget(BaudRateBox,2,2,1,1);
    gridLayout->addWidget(DataBitsBox,3,2,1,1);
    gridLayout->addWidget(ParityBox,4,2,1,1);
    gridLayout->addWidget(StopBitsBox,5,2,1,1);
    gridLayout->addWidget(FlowControlBox,6,2,1,1);

    setButton = new QPushButton( "Set" );
    connect( setButton, SIGNAL(pressed() ), this, SLOT( saveSettings() ));
    gridLayout->addWidget( setButton, 7,1,1,1);

    openButton = new QPushButton( "Open" );
    connect( openButton, SIGNAL(pressed() ), port, SLOT( openPort() ));
    gridLayout->addWidget( openButton, 8,1,1,1);

    QPushButton *searchButton = new QPushButton( "Search" );
    connect( searchButton, SIGNAL(pressed() ), this, SLOT( searchPorts() ));
    gridLayout->addWidget( searchButton, 7,2,1,1);


    BaudRateBox->addItem(QLatin1String("9600"), QSerialPort::Baud9600);
    BaudRateBox->addItem(QLatin1String("19200"), QSerialPort::Baud19200);
    BaudRateBox->addItem(QLatin1String("38400"), QSerialPort::Baud38400);
    BaudRateBox->addItem(QLatin1String("115200"), QSerialPort::Baud115200);
   // fill data bits
    DataBitsBox->addItem(QLatin1String("5"), QSerialPort::Data5);
    DataBitsBox->addItem(QLatin1String("6"), QSerialPort::Data6);
    DataBitsBox->addItem(QLatin1String("7"), QSerialPort::Data7);
    DataBitsBox->addItem(QLatin1String("8"), QSerialPort::Data8);
    DataBitsBox->setCurrentIndex(3);
   // fill parity
    ParityBox->addItem(QLatin1String("None"), QSerialPort::NoParity);
    ParityBox->addItem(QLatin1String("Even"), QSerialPort::EvenParity);
    ParityBox->addItem(QLatin1String("Odd"), QSerialPort::OddParity);
    ParityBox->addItem(QLatin1String("Mark"), QSerialPort::MarkParity);
    ParityBox->addItem(QLatin1String("Space"), QSerialPort::SpaceParity);
   // fill stop bits
    StopBitsBox->addItem(QLatin1String("1"), QSerialPort::OneStop);
    StopBitsBox->addItem(QLatin1String("1.5"), QSerialPort::OneAndHalfStop);
    StopBitsBox->addItem(QLatin1String("2"), QSerialPort::TwoStop);
   // fill flow control
    FlowControlBox->addItem(QLatin1String("None"), QSerialPort::NoFlowControl);
    FlowControlBox->addItem(QLatin1String("RTS/CTS"), QSerialPort::HardwareControl);
    FlowControlBox->addItem(QLatin1String("XON/XOFF"), QSerialPort::SoftwareControl);


    QWidget* mainWidget = new QWidget;
    mainWidget->setLayout( gridLayout );
    setCentralWidget( mainWidget );
    setWindowTitle( "Port settings" );

}

void ForceWindow::searchPorts()
{
    PortNameBox->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        PortNameBox->addItem(info.portName());
    }
}

void ForceWindow::saveSettings()
{
    int baudrate = BaudRateBox->currentData().toInt();
    int databits = DataBitsBox->currentData().toInt();
    int parity = ParityBox->currentData().toInt();
    int stopbits = StopBitsBox->currentData().toInt();
    int flowcontrol = FlowControlBox->currentData().toInt();

    port->setPortSettings( PortNameBox->currentText(), baudrate, databits, parity, stopbits, flowcontrol );

    qDebug() << "click set";
}
