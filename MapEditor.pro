#-------------------------------------------------
#
# Project created by QtCreator 2017-01-03T12:15:19
#
#-------------------------------------------------

QT       += core gui

INCLUDEPATH += src
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MapEditor
TEMPLATE = app


SOURCES += main.cpp \
    src/mainwindow.cpp \
    src/viewwidget.cpp \
    src/room.cpp \
    src/importer.cpp \
    src/save.cpp \
    src/roomsfromimage.cpp \
    src/verticies.cpp \
    src/load.cpp \
    src/commandlist.cpp \
    src/workerthread.cpp \
    src/super_xbr.cpp \
    src/scaleimages.cpp

HEADERS  += mainwindow.h \
    src/mainwindow.h \
    src/viewwidget.h \
    src/byteswap.h \
    src/room.h \
    src/roomsfromimage.h \
    src/verticies.h \
    src/commandlist.h \
    src/workerthread.h

FORMS    += mainwindow.ui
