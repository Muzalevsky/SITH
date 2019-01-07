#ifndef STEPPERCONTROL_H
#define STEPPERCONTROL_H

#include <QMainWindow>
#include <QGridLayout>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>

#include <port.h>
#include <smsd_header.h>
#include <settingsdialog.h>
#include <cstdint>

class StepperControl : public QMainWindow
{
public:
    explicit StepperControl( Port* ext_port, QWidget* parent );
    uint8_t xor_sum(uint8_t *data,uint16_t length);
    void    sendPassword();
    void    request();
    void    response();
    void    powerStep01();

    QPushButton *setButton;
    QPushButton *openButton;

private:
    QComboBox       *PortNameBox;

    Port            *port;
    QGridLayout     *gridLayout;
    QLineEdit       *speedEdit;
    QLineEdit       *stepNumberEdit;
    QLineEdit       *stepSizeEdit;
    QPushButton     *initBtn;
    QPushButton     *sendBtn;
    SettingsDialog  *m_settingsDialog;
public slots:
    void saveSettings();
    void searchPorts();
};

#endif // STEPPERCONTROL_H
