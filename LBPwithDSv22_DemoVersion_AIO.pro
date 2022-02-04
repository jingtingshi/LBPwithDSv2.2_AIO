#-------------------------------------------------
#
# Project created by QtCreator 2020-05-30T17:20:58
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = LBPwithDSv22_DemoVersion_AIO
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    searchpattern.cpp \
    qsvm.cpp

HEADERS += \
        mainwindow.h \
    searchpattern.h \
    qsvm.h \
    datastructure.h

FORMS += \
        mainwindow.ui

INCLUDEPATH += C:\openCvMGW\include\opencv\
               C:\openCvMGW\include\opencv2\
               C:\openCvMGW\include

LIBS += C:\openCvMGW\lib\libopencv_core2413.dll.a\
        C:\openCvMGW\lib\libopencv_highgui2413.dll.a\
        C:\openCvMGW\lib\libopencv_video2413.dll.a\
        C:\OpenCvMGW\lib\libopencv_imgproc2413.dll.a\
        C:\OpenCvMGW\lib\libopencv_legacy2413.dll.a\
        C:\OpenCvMGW\lib\libopencv_ml2413.dll.a

#INCLUDEPATH += /usr/local/include\
#               /usr/local/include/opencv2\
#               /usr/local/include/opencv
#LIBS += -L/usr/local/lib -lopencv_imgproc -lopencv_core -lopencv_highgui -lopencv_video


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
