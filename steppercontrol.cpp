#include "steppercontrol.h"

#include <cstdlib>

StepperControl::StepperControl()
{

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
