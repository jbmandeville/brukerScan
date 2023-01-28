#-------------------------------------------------
#
# Project created by QtCreator 2021-10-21T10:52:46
#
#-------------------------------------------------
DEFINES += QT_NO_DEBUG_OUTPUT

QT       += core gui concurrent widgets

LIBS += -L"$$_PRO_FILE_PWD_/3rdparty/fftw-3.3.8/lib" -lfftw3
INCLUDEPATH += 3rdparty/fftw-3.3.8/include

TARGET = MRIProcess
TEMPLATE = app

SOURCES += main.cpp\
        ImageIO.cpp \
    MRIProcess.cpp \
        anatomy.cpp \
        download.cpp \
        function.cpp \
        utilio.cpp \
    clean.cpp

HEADERS  += \
    ImageIO.h \
    MRIProcess.h \
    io.h \
    nifti1.h

DISTFILES +=

RESOURCES += \
    MyResources.qrc
