QT += core
QT -= gui

CONFIG += c++11

TARGET = smart_pointer
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp

HEADERS += \
    unique_ptr.h
