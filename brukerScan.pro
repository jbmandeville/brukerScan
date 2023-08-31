#-------------------------------------------------
#
# Project created by QtCreator 2021-10-21T10:52:46
#
#-------------------------------------------------
#DEFINES += QT_NO_DEBUG_OUTPUT

QT       += core gui concurrent widgets

INCLUDEPATH += ../util/
INCLUDEPATH += ../util/3rdparty/nifticlib-2.0.0/include
INCLUDEPATH += ../util/3rdparty/nifticlib-2.0.0/znzlib

VPATH       += ../util
VPATH       += ../util/3rdparty/nifticlib-2.0.0/include

LIBS += -L"../util/3rdparty/nifticlib-2.0.0/znzlib" -lznz -lz

TARGET = brukerScan
TEMPLATE = app

SOURCES += main.cpp\
        brukerScan.cpp \
        scan.cpp \
        clean.cpp \
        ../util/ImageIO.cpp \
        ../util/utilio.cpp

HEADERS  += \
    brukerScan.h \
    ImageIO.h \
    io.h \
    nifti1.h

DISTFILES +=

RESOURCES += \
    MyResources.qrc
