QT +=  core gui serialport serialbus

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets


CONFIG += c++11

TARGET = sol_console
#CONFIG += console
#CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    benchwindow.cpp \
    serialwindow.cpp \
    port.cpp \
    forcewindow.cpp \
    steppercontrol.cpp \
    modbuslistener.cpp \
    settingsdialog.cpp
HEADERS += \
    benchwindow.h \
    serialwindow.h \
    port.h \
    forcewindow.h \
    steppercontrol.h \
    modbuslistener.h \
    settingsdialog.h
#INSTALLS += target

FORMS += \
    settingsdialog.ui \
    modbuslistener.ui
