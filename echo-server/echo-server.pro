TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

include($$PWD/../common/common.pri)

SOURCES += \
        main.cpp \
    kserverhandler.cpp

HEADERS += \
    kserverhandler.h
