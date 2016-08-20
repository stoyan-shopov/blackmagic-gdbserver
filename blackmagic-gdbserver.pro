#-------------------------------------------------
#
# Project created by QtCreator 2016-08-20T22:40:15
#
#-------------------------------------------------

QT       += core gui network serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = blackmagic-gdbserver
TEMPLATE = app


SOURCES +=\
        mainwindow.cxx \
    main.cxx \
    gdbpacket.cxx

HEADERS  += mainwindow.hxx \
    gdbpacket.hxx

FORMS    += mainwindow.ui
