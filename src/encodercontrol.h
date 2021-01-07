#ifndef ENCODERCONTROL_H
#define ENCODERCONTROL_H

#include <QObject>

#include <port.h>
#include <settingsdialog.h>

class EncoderControl : public QObject
{
    Q_OBJECT
public:
    explicit EncoderControl( QObject* parent = nullptr );
    QString         position_str;
    SettingsDialog  *m_settingsDialog;
    Port            *port;
    QString         portName;
    uint8_t         word_cnt;
    int             zero_mark_number;
    float           position_mm;
    float           final_position_mm;
    float           prev_final_position_mm;
    int             prev_position_raw;
    int             position_raw;
    int             delta;
    int8_t          speed_dir;
    float           zero_position_offset;
    int             overfloat_position;

    bool isAlive();

private:
    bool hasConnection;

public slots:
    void saveSettings();
    void setZeroPosition();
    void gotTimeout();
private slots:
    void updatePosition(QString);
    void analyzePosition(QString);

signals:
    void updateUpperLevelPosition(float);
    void positionChanged(QString);
    void resetWatchDog();
};

#endif // ENCODERCONTROL_H
