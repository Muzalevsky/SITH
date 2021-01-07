QT +=  core gui serialport serialbus

#QT +=  core gui


greaterThan(QT_MAJOR_VERSION, 4): QT += widgets


CONFIG += c++11

TARGET = sol_console
#CONFIG += console
#CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    port.cpp \
    forcewindow.cpp \
    steppercontrol.cpp \
    modbuslistener.cpp \
    settingsdialog.cpp \
    mainwindow.cpp \
    encodercontrol.cpp
HEADERS += \
    port.h \
    forcewindow.h \
    steppercontrol.h \
    modbuslistener.h \
    settingsdialog.h \
    smsd_header.h \
    mainwindow.h \
    encodercontrol.h
#INSTALLS += target

FORMS += \
    settingsdialog.ui \
    mainwindow.ui

RESOURCES += \
    icons.qrc
