#ifndef FORCEWINDOW_H
#define FORCEWINDOW_H

#include <QMainWindow>
#include <QGridLayout>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>

#include <port.h>
#include <settingsdialog.h>

class ForceWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit ForceWindow( Port* port, QWidget* parent );

private:
    Port            *port;
    QGridLayout     *gridLayout;
    QComboBox       *PortNameBox;
    SettingsDialog  *m_settingsDialog;

public slots:
    void saveSettings();
    void searchPorts();
};

#endif // FORCEWINDOW_H
