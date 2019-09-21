#ifndef ENCODERCONTROL_H
#define ENCODERCONTROL_H

#include <QWidget>
#include <QObject>
#include <QMainWindow>

#include <port.h>
#include <settingsdialog.h>

class EncoderControl : public QMainWindow
{
    Q_OBJECT
public:
    explicit EncoderControl( Port* ext_port, QWidget* parent );
    QString         position_str;
    SettingsDialog  *m_settingsDialog;
    Port            *port;
    QString         portName;
    uint8_t         word_cnt;
    int             zero_mark_number;
    float           position_mm;
    float           final_position_mm;
    int             prev_position_raw;
    int             position_raw;
    int             delta;
    int8_t          speed_dir;
    float           zero_position_offset;
    int             overfloat_position;

private:

public slots:
    void saveSettings();
    void updatePosition(QString);
    void analyzePosition(QString);
    void setZeroPosition();
signals:
    void updateUpperLevelPosition();
    void positionChanged(QString);

};

#endif // ENCODERCONTROL_H
