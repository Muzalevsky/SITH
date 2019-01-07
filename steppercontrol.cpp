#include "steppercontrol.h"

#include <cstdlib>

StepperControl::StepperControl( QWidget* parent ) : QMainWindow( parent )
{
    gridLayout = new QGridLayout();
    speedEdit       = new QLineEdit();
    stepNumberEdit  = new QLineEdit();
    stepSizeEdit    = new QLineEdit();
    initBtn = new QPushButton("Init", this);
    sendBtn = new QPushButton("Send", this);

    gridLayout->addWidget(new QLabel(tr("Set speed: ")),1,1,1,1);
    gridLayout->addWidget(new QLabel(tr("Set step number: ")),2,1,1,1);
    gridLayout->addWidget(new QLabel(tr("Set step size (mm): ")),3,1,1,1);
    gridLayout->addWidget(speedEdit,1,2,1,1);
    gridLayout->addWidget(stepNumberEdit,2,2,1,1);
    gridLayout->addWidget(stepSizeEdit,3,2,1,1);

    gridLayout->addWidget(initBtn,4,1,1,1);
    gridLayout->addWidget(sendBtn,4,2,1,1);

//    setButton = new QPushButton( "Set" );
//    connect( setButton, SIGNAL(pressed() ), this, SLOT( saveSettings() ));
//    gridLayout->addWidget( setButton, 5,1,1,1);

//    openButton = new QPushButton( "Open" );
//    connect( openButton, SIGNAL(pressed() ), port, SLOT( openPort() ));
//    gridLayout->addWidget( openButton, 5,2,1,1);

//    QPushButton *searchButton = new QPushButton( "Search" );
//    connect( searchButton, SIGNAL(pressed() ), this, SLOT( searchPorts() ));
//    gridLayout->addWidget( searchButton, 5,3,1,1);

    QWidget* mainWidget = new QWidget;
    mainWidget->setLayout( gridLayout );
    setCentralWidget( mainWidget );
    setWindowTitle( "Stepper settings" );
}

uint8_t StepperControl::xor_sum(uint8_t *data,uint16_t length)
{
    uint8_t xor_temp = 0xFF;
    while( length-- )
    {
        xor_temp += *data;
        data++;
    }
    return ( xor_temp ^ 0xFF );
}

void StepperControl::sendPassword()
{
    LAN_COMMAND_Type *cmd;
    cmd = (LAN_COMMAND_Type *)calloc( 1, sizeof( LAN_COMMAND_Type* ) );

    cmd->Ver = 0x02;
    cmd->CMD_TYPE = CODE_CMD_REQUEST;
    cmd->LENGTH_DATA = 0x0008;
//    cmd->DATA = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

    cmd->XOR_SUM=0x00;
    cmd->XOR_SUM=xor_sum((uint8_t*)&cmd->XOR_SUM, sizeof(cmd));

}
