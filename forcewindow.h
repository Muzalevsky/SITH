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
    float           force_kg;
    int             symbols_left;

    SettingsDialog  *m_settingsDialog;
    Port            *port;
    QString         portName;
    void            updateForce( QString);

    bool            isAlive();
    void            updateForceValue( QString str );

private:
    bool hasConnection;

public slots:
    void saveSettings();
//    void reconnectPort();

private slots:
    void gotTimeout();
signals:
    void resetWatchDog();
    void lostConnection();

//    void setForceValue( QString );
    void setForceValue( float force_kg );
};

#endif // FORCEWINDOW_H
