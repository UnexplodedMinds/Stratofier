#-------------------------------------------------
#
# RoscoPi
# Copyright 2019 Unexploded Minds
#
#-------------------------------------------------

QT += core gui websockets widgets network

TARGET = RoscoPi
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += ./include \
               ../include \
               ../RoscoPi/include \
			   ../RoscoPi/uic \
			   ../RoscoPi/rcc


VPATH += ./include \
         ../include \
         ../RoscoPi/ui \
         ../RoscoPi/include

DESTDIR = ../RoscoPi/bin
OBJECTS_DIR = ../RoscoPi/obj

UI_DIR = ../RoscoPi/uic
MOC_DIR = ../RoscoPi/moc
RCC_DIR = ../RoscoPi/rcc

SOURCES += main.cpp \
           StreamReader.cpp \
           AHRSCanvas.cpp \
           AHRSMainWin.cpp \
           BugSelector.cpp \
           Keypad.cpp \
           TrafficMath.cpp \
           Canvas.cpp \
           MenuDialog.cpp \
           Builder.cpp

HEADERS += StratuxStreams.h \
           StreamReader.h \
           AHRSCanvas.h \
           AHRSMainWin.h \
           BugSelector.h \
           Keypad.h \
           TrafficMath.h \
           Canvas.h \
           MenuDialog.h \
           Builder.h

FORMS += AHRSMainWin.ui \
         BugSelector.ui \
         Keypad.ui \
         MenuDialog.ui

RESOURCES += AHRSResources.qrc
