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
    singlemod.cpp \
    settings.cpp \
    modlistmodel.cpp \
    merger.cpp \
    modtableview.cpp \
    modinstaller.cpp \
    pausemessagebox.cpp

HEADERS  += mainwindow.h \
    metadatastore.h \
    bundle.h \
    unpacker.h \
    libs/lz4.h \
    singlemod.h \
    settings.h \
    modlistmodel.h \
    merger.h \
    modtableview.h \
    modinstaller.h \
    pausemessagebox.h

FORMS    += mainwindow.ui \
    settings.ui

RESOURCES += \
    resources.qrc

RC_ICONS = icons\wolfhead.ico

win32 {
    WINSDK_DIR = c:/Program Files/Microsoft SDKs/Windows/v7.1
    WIN_PWD = $$replace(PWD, /, \\)
    OUT_PWD_WIN = $$replace(OUT_PWD, /, \\)
    QMAKE_POST_LINK = "$$WINSDK_DIR/bin/x64/mt.exe -manifest $$quote($$WIN_PWD\\$$basename(TARGET).manifest) -outputresource:$$quote($$OUT_PWD_WIN\\${DESTDIR_TARGET};1)"
}

TRANSLATIONS += \
    translations\W3ModMerger_ru_RU.ts

#LIBS += -Lc:\Libs\boost_1_61_0\build\lib
#LIBS += -lboost_zlib-mgw51-mt-1_61
