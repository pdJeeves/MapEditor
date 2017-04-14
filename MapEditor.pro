#-------------------------------------------------
#
# Project created by QtCreator 2017-01-03T12:15:19
#
#-------------------------------------------------

QT       += core gui

INCLUDEPATH += src
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
INCLUDEPATH += \
	/home/anyuser/Downloads/squish-1.11/ \
	/home/anyuser/Developer/Kreatures/Engine/src/ \
	/home/anyuser/Developer/Kreatures/libFreetures/include/

LIBS += -L/home/anyuser/Downloads/squish-1.11/ -lsquish -ldrm
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
    src/scaleimages.cpp \
    src/verticiesfromimage.cpp \
    src/casttoedges.cpp \
    src/roommesh.cpp \
    src/palette16.cpp \
    src/airfromrooms.cpp \
    src/linkrooms.cpp \
    src/airfromroomsnp.cpp \
    src/joinroomsvertically.cpp

HEADERS  += mainwindow.h \
    src/mainwindow.h \
    src/viewwidget.h \
    src/byteswap.h \
    src/room.h \
    src/roomsfromimage.h \
    src/verticies.h \
    src/commandlist.h \
    src/workerthread.h \
    src/diffusionmap.h \
    src/verticiesfromimage.h \
    src/casttoedges.h \
    src/roommesh.h \
    src/palette16.h \
    src/airfromrooms.h \
    src/linkrooms.h \
    src/airfromroomsnp.h \
    src/joinroomsvertically.h

FORMS    += mainwindow.ui
