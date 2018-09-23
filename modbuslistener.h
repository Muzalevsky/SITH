#ifndef MODBUSLISTENER_H
#define MODBUSLISTENER_H

#include <QModbusDataUnit>
#include <QMainWindow>
#include <QTimer>

#include <settingsdialog.h>
QT_BEGIN_NAMESPACE

class QModbusClient;
class QModbusReply;

namespace Ui {
class ModbusListener;
class SettingsDialog;
}

class ModbusListener : public QMainWindow
{
    Q_OBJECT
public:
    explicit ModbusListener(QWidget *parent = nullptr);
    ~ModbusListener();
    QModbusDataUnit readRequest() const;
    const QModbusDataUnit *unit;
    QStringList *strList;
    double voltagePhaseA;
    double voltagePhaseB;
    double voltagePhaseC;
    double frequency;


private:
    QTimer              *timer;

    void initActions();
    Ui::ModbusListener *ui;
    QModbusReply *lastRequest;
    QModbusClient *modbusDevice;
    SettingsDialog *m_settingsDialog;

private slots:
    void on_connectButton_clicked();
    void onStateChanged(int state);
    void on_readButton_clicked();
    void readReady();
    void on_connectType_currentIndexChanged(int);

signals:
    void getReply();
};

#endif // MODBUSLISTENER_H
