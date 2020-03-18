#ifndef FORCEWINDOW_H
#define FORCEWINDOW_H

//#include <QMainWindow>
//#include <QGridLayout>
//#include <QComboBox>
//#include <QLabel>
//#include <QPushButton>

#include <port.h>
#include <settingsdialog.h>

class ForceWindow : public QObject
{
    Q_OBJECT
public:
    explicit ForceWindow( QObject* parent = nullptr );
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
    bool isPortAlive;

public slots:
    void saveSettings();
//    void reconnectPort();

private slots:
    void gotTimeout();
signals:
    void resetWatchDog();
    void lostConnection();

//    void setForceValue( QString );
// QA: why not float& force_kg? Is it declared somewhere in Qt Lib? 
    void setForceValue( float force_kg );
};

#endif // FORCEWINDOW_H
