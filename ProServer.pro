#-------------------------------------------------
#
# Project created by QtCreator 2019-02-04T10:02:07
#
#-------------------------------------------------

QT       += core gui widgets network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

DESTDIR = ../build_Pro
TARGET = ProServer
TEMPLATE = app
CONFIG += c++11

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
        main.cpp \
        servwgt.cpp \
    network/tcpclient.cpp \
    network/tcpserver.cpp \
    hwconnmgr.cpp

HEADERS += \
        servwgt.h \
    network/tcpclient.h \
    network/tcpserver.h \
    hwconnmgr.h

FORMS += \
        servwgt.ui

INCLUDEPATH += ../headers

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
