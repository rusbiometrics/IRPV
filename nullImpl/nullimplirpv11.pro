#-------------------------------------------------
#
# Project created by QtCreator 2017-06-16T14:15:51
#
#-------------------------------------------------
CONFIG -= qt

CONFIG += c++11

TARGET = irpv_11_null_0_cpu

TEMPLATE = lib

DEFINES += BUILD_SHARED_LIBRARY

SOURCES += nullimplirpv11.cpp

HEADERS += nullimplirpv11.h
           $${PWD}/../irpv.h

INCLUDEPATH += $${PWD}/..

# Installation paths
win32 {
    win32-msvc2013: COMPILER = vc12
    win32-msvc2015: COMPILER = vc14
    win32-msvc2017: COMPILER = vc16
    win32-g++:      COMPILER = mingw
    win32:contains(QMAKE_TARGET.arch, x86_64){
        ARCHITECTURE = x64
    } else {
        ARCHITECTURE = x86
    }
    DESTDIR = $${PWD}/../API_bin/$${TARGET}/$${ARCHITECTURE}/$${COMPILER}
}

linux {
    DEFINES += Q_OS_LINUX
    DESTDIR = $${PWD}/../API_bin/$${TARGET}
}
