#-------------------------------------------------
#
# Project created by QtCreator 2012-03-29T13:24:46
#
#-------------------------------------------------

QT       += core gui

TARGET = qtab
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    x11helper.cpp

HEADERS  += mainwindow.h \
    qtab.h \
    json.h \
    x11helper.h

CONFIG += debug
FORMS    += mainwindow.ui

QMAKE_CXXFLAGS += -std=c++0x
