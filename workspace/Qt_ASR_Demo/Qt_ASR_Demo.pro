QT       += core gui network multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
CONFIG -= console

TARGET = Qt_ASR_Demo
TEMPLATE = app

SOURCES += \
    src/audiocollector.cpp \
    src/main.cpp \
    src/mainwindow.cpp

HEADERS += \
    src/audiocollector.h \
    src/mainwindow.h

FORMS += \
    src/mainwindow.ui

win32 {
    clang: QMAKE_CXXFLAGS += -execution-charset:utf-8 -source-charset:utf-8
    msvc: QMAKE_CXXFLAGS += /utf-8
}
