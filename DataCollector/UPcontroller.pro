#-------------------------------------------------
#
# Project created by QtCreator 2016-11-07T14:26:15
#
#-------------------------------------------------

QT       += core gui
QT       += sql
QT       += serialport
QT       += widgets printsupport
QT       += websockets
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport


TARGET = UPcontroller
TEMPLATE = app


SOURCES += main.cpp\
        widget.cpp \
    qcustomplot.cpp \
    syncthread.cpp \
    setting.cpp

HEADERS  += widget.h \
    qcustomplot.h \
    mythread.h \
    syncthread.h \
    setting.h

FORMS    += widget.ui \
    setting.ui

RC_FILE += Biglion.rc

RESOURCES += \
    mediasrc.qrc

QMAKE_LFLAGS_WINDOWS = /SUBSYSTEM:WINDOWS,5.01
