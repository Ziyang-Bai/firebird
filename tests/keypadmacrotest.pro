QT += core testlib
CONFIG += console c++11 testcase
CONFIG -= app_bundle
TEMPLATE = app
TARGET = keypadmacrotest

INCLUDEPATH += ..
SOURCES += keypadmacrotest.cpp \
    ../keypadmacro.cpp
HEADERS += ../keypadmacro.h
