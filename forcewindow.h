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
    QString         force_str;
    SettingsDialog  *m_settingsDialog;
    Port            *port;
    QString         portName;
    void            updateForce( QString);

private:

public slots:
    void saveSettings();

signals:
    void setForceValue( QString );

};

#endif // FORCEWINDOW_H
