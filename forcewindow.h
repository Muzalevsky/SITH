#ifndef FORCEWINDOW_H
#define FORCEWINDOW_H

#include <QMainWindow>
#include <QGridLayout>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <port.h>

class ForceWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit ForceWindow( Port* port, QWidget* parent );
    QPushButton *setButton;
    QPushButton *openButton;


private:
    Port            *port;
    QGridLayout     *gridLayout;
    QComboBox       *PortNameBox;
    QComboBox       *BaudRateBox;
    QComboBox       *DataBitsBox;
    QComboBox       *ParityBox;
    QComboBox       *StopBitsBox;
    QComboBox       *FlowControlBox;

public slots:
    void saveSettings();
    void searchPorts();
};

#endif // FORCEWINDOW_H
