#ifndef STEPPERCONTROL_H
#define STEPPERCONTROL_H

#include <QMainWindow>
#include <QGridLayout>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>


#include <cstdint>

enum  CMD_TYPE {
    CODE_CMD_REQUEST = 0x00,
    CODE_CMD_RESPONSE = 0x01,
    CODE_CMD_POWERSTEP01 = 0x02,
    CODE_CMD_POWERSTEP01_W_MEM0 = 0x03,
    CODE_CMD_POWERSTEP01_W_MEM1 = 0x04,
    CODE_CMD_POWERSTEP01_W_MEM2 = 0x05,
    CODE_CMD_POWERSTEP01_W_MEM3 = 0x06,
    CODE_CMD_POWERSTEP01_R_MEM0 = 0x07,
    CODE_CMD_POWERSTEP01_R_MEM1 = 0x08,
    CODE_CMD_POWERSTEP01_R_MEM2 = 0x09,
    CODE_CMD_POWERSTEP01_R_MEM3 = 0x0A,
    CODE_CMD_CONFIG_SET = 0x0B,
    CODE_CMD_CONFIG_GET = 0x0C,
    CODE_CMD_PASSWORD_SET = 0x0D,
    CODE_CMD_ERROR_GET = 0x0E
};

enum CMD_PowerSTEP {
    CMD_PowerSTEP01_END = 0x00,
    CMD_PowerSTEP01_GET_SPEED = 0x01,
    CMD_PowerSTEP01_STATUS_IN_EVENT = 0x02,
    CMD_PowerSTEP01_SET_MODE = 0x03,
    CMD_PowerSTEP01_GET_MODE = 0x04,
    CMD_PowerSTEP01_SET_MIN_SPEED = 0x05,
    CMD_PowerSTEP01_SET_MAX_SPEED = 0x06,
    CMD_PowerSTEP01_SET_ACC = 0x07,
    CMD_PowerSTEP01_SET_DEC = 0x08,
    CMD_PowerSTEP01_SET_FS_SPEED = 0x09,
    CMD_PowerSTEP01_SET_MASK_EVENT = 0x0A,
    CMD_PowerSTEP01_GET_ABS_POS = 0x0B,
    CMD_PowerSTEP01_GET_EL_POS = 0x0C,
    CMD_PowerSTEP01_GET_STATUS_AND_CLR = 0x0D,
    CMD_PowerSTEP01_RUN_F = 0x0E,
    CMD_PowerSTEP01_RUN_R = 0x0F,
    CMD_PowerSTEP01_MOVE_F = 0x10,
    CMD_PowerSTEP01_MOVE_R = 0x11,
    CMD_PowerSTEP01_GO_TO_F = 0x12,
    CMD_PowerSTEP01_GO_TO_R = 0x13,
    CMD_PowerSTEP01_GO_UNTIL_F = 0x14,
    CMD_PowerSTEP01_GO_UNTIL_R = 0x15,
    CMD_PowerSTEP01_SCAN_ZERO_F = 0x16,
    CMD_PowerSTEP01_SCAN_ZERO_R = 0x17,
    CMD_PowerSTEP01_SCAN_LABEL_F = 0x18,
    CMD_PowerSTEP01_SCAN_LABEL_R = 0x19,
    CMD_PowerSTEP01_GO_ZERO = 0x1A,
    CMD_PowerSTEP01_GO_LABEL = 0x1B,
    CMD_PowerSTEP01_GO_TO = 0x1C,
    CMD_PowerSTEP01_RESET_POS = 0x1D,
    CMD_PowerSTEP01_RESET_POWERSTEP01 = 0x1E,
    CMD_PowerSTEP01_SOFT_STOP = 0x1F,
    CMD_PowerSTEP01_HARD_STOP = 0x20,
    CMD_PowerSTEP01_SOFT_HI_Z = 0x21,
    CMD_PowerSTEP01_HARD_HI_Z = 0x22,
    CMD_PowerSTEP01_SET_WAIT = 0x23,
    CMD_PowerSTEP01_SET_RELE = 0x24,
    CMD_PowerSTEP01_CLR_RELE = 0x25,
    CMD_PowerSTEP01_GET_RELE = 0x26,
    CMD_PowerSTEP01_WAIT_IN0 = 0x27,
    CMD_PowerSTEP01_WAIT_IN1 = 0x28,
    CMD_PowerSTEP01_GOTO_PROGRAM = 0x29,
    CMD_PowerSTEP01_GOTO_PROGRAM_IF_IN0 = 0x2A,
    CMD_PowerSTEP01_GOTO_PROGRAM_IF_IN1 = 0x2B,
    CMD_PowerSTEP01_LOOP_PROGRAM = 0x2C,
    CMD_PowerSTEP01_CALL_PROGRAM = 0x2D,
    CMD_PowerSTEP01_RETURN_PROGRAM = 0x2E,
    CMD_PowerSTEP01_START_PROGRAM_MEM0 = 0x2F,
    CMD_PowerSTEP01_START_PROGRAM_MEM1 = 0x30,
    CMD_PowerSTEP01_START_PROGRAM_MEM2 = 0x31,
    CMD_PowerSTEP01_START_PROGRAM_MEM3 = 0x32,
    CMD_PowerSTEP01_STOP_PROGRAM_MEM = 0x33,
    CMD_PowerSTEP01_STEP_CLOCK = 0x34,
    CMD_PowerSTEP01_STOP_USB = 0x35,
    CMD_PowerSTEP01_GET_MIN_SPEED = 0x36,
    CMD_PowerSTEP01_GET_MAX_SPEED = 0x37,
    CMD_PowerSTEP01_GET_STACK = 0x38
} ;

typedef struct
{
    uint8_t XOR_SUM;
    uint8_t Ver;
    uint8_t CMD_TYPE;
    uint8_t CMD_IDENTIFICATION;
    uint16_t LENGTH_DATA;
    uint8_t *DATA;
}LAN_COMMAND_Type;

typedef struct
{
    uint32_t RESERVE;
    uint32_t ACTION;
    uint32_t COMMAND;
    uint32_t DATA;
}SMSD_CMD_Type;

typedef struct {
    uint16_t HiZ;
    uint16_t BUSY;
    uint16_t SW_F;
    uint16_t SW_EVN;
    uint16_t DIR;
    uint16_t MOT_STATUS;
    uint16_t CMD_ERROR;
    uint16_t RESERVE;
} powerSTEP_STATUS_TypeDef;

typedef struct
{
    powerSTEP_STATUS_TypeDef STATUS_POWERSTEP01;
    uint8_t ERROR_OR_COMMAND;
    uint32_t RETURN_DATA;
}COMMANDS_RETURN_DATA_Type;


class StepperControl : public QMainWindow
{
public:
    explicit StepperControl( QWidget* parent );
    uint8_t xor_sum(uint8_t *data,uint16_t length);
    void    sendPassword();

    QPushButton *setButton;
    QPushButton *openButton;


private:
    QGridLayout     *gridLayout;
    QLineEdit       *speedEdit;
    QLineEdit       *stepNumberEdit;
    QLineEdit       *stepSizeEdit;
    QPushButton     *initBtn;
    QPushButton     *sendBtn;

};

#endif // STEPPERCONTROL_H
