TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

include($$PWD/../common/common.pri)

SOURCES += \
        main.cpp \
    kabstractclienthandler.cpp \
    ktcpclienthandler.cpp \
    kudpclienthandler.cpp

HEADERS += \
    kabstractclienthandler.h \
    ktcpclienthandler.h \
    kudpclienthandler.h
