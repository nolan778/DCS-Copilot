#-------------------------------------------------
#
# Project created by QtCreator 2016-10-29T02:24:23
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = DCS_Copilot
TEMPLATE = app

MAJOR = 0
MINOR = 1
VERSION_HEADER = $$PWD/version.h

versiontarget.target = $$VERSION_HEADER
versiontarget.commands = $$PWD/version.exe $$MAJOR $$MINOR $$VERSION_HEADER
versiontarget.depends = FORCE

PRE_TARGETDEPS += $$VERSION_HEADER
QMAKE_EXTRA_TARGETS += versiontarget

DESTDIR = $$PWD/../bin

SOURCES += main.cpp\
    banlistwindow.cpp \
        mainwindow.cpp \
    NetworkLocal.cpp \
    settingswindow.cpp \
    serverstart.cpp \
    connectionwindow.cpp \
    aboutwindow.cpp \
    clickableimage.cpp \
    Network.cpp

HEADERS  += mainwindow.h \
    NetworkLocal.h \
    NetworkTypes.h \
    banlistwindow.h \
    settingswindow.h \
    serverstart.h \
    connectionwindow.h \
    aboutwindow.h \
    version.h \
    clickableimage.h \
    Network.h

INCLUDEPATH +=$$PWD/../3rdparty/RakNet/Source
INCLUDEPATH += $$PWD/.
DEPENDPATH += $$PWD/.

FORMS    += mainwindow.ui \
    banlistwindow.ui \
    settingswindow.ui \
    serverstart.ui \
    connectionwindow.ui \
    aboutwindow.ui


CONFIG += static
LIBS += Ws2_32.lib
CONFIG(release, debug|release): {
    LIBS += -L$$PWD/../3rdparty/RakNet/Lib/ -lRakNetStatic_x64
    PRE_TARGETDEPS += $$PWD/../3rdparty/RakNet/Lib/RakNetStatic_x64.lib
    message("64-bit STATIC release")
}
else:CONFIG(debug, debug|release): {
    LIBS += -L$$PWD/../3rdparty/RakNet/Lib/ -lRakNetStatic_x64d
    PRE_TARGETDEPS += $$PWD/../3rdparty/RakNet/Lib/RakNetStatic_x64d.lib
    message("64-bit STATIC debug")
}

RESOURCES += \
    resource.qrc




