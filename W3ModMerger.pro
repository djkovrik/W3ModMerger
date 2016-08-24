#-------------------------------------------------
#
# Project created by QtCreator 2016-08-03T18:58:35
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = W3ModMerger
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    metadatastore.cpp \
    bundle.cpp \
    unpacker.cpp \
    libs/lz4.cpp \
    libs/doboz/Decompressor.cpp \
    singlemod.cpp \
    settings.cpp \
    modlistmodel.cpp \
    merger.cpp \
    modtableview.cpp

HEADERS  += mainwindow.h \
    metadatastore.h \
    bundle.h \
    unpacker.h \
    libs/lz4.h \
    libs/doboz/Common.h \
    libs/doboz/Decompressor.h \
    singlemod.h \
    settings.h \
    libs/zlib/strict_fstream.hpp \
    libs/zlib/zconf.h \
    libs/zlib/zlib.h \
    libs/zlib/zstr.hpp \
    modlistmodel.h \
    merger.h \
    modtableview.h

FORMS    += mainwindow.ui \
    settings.ui

RESOURCES += \
    resources.qrc

RC_ICONS = icons\wolfhead.ico

LIBS += -Lc:\Libs\boost_1_61_0\build\lib
LIBS += -lboost_zlib-mgw51-mt-1_61
