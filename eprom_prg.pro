TEMPLATE = app
TARGET = eprom_prg
DESTDIR = .

QT += core gui widgets serialport

DEPENDPATH  += . ./GeneratedFiles
MOC_DIR     += ./GeneratedFiles
UI_DIR      += ./GeneratedFiles
RCC_DIR     += ./GeneratedFiles

CONFIG += qt warn_off thread sdk_no_version_check c++11
CONFIG(debug, debug|release) {
    DEFINES += _DEBUG
    OBJECTS_DIR = debug
    CONFIG -= release
    DESTDIR = debug
}
CONFIG(release, debug|release) {
    DEFINES     += QT_NO_WARNING_OUTPUT
    OBJECTS_DIR = release
    CONFIG -= debug
    DESTDIR = release
}

DEFINES += QT_GUI_LIB QT_WIDGETS_LIB

INCLUDEPATH += . \
    ./GeneratedFiles \
    $(QTDIR64)/include \
    $(QTDIR64)/include/QtCore \
    $(QTDIR64)/include/QtGui \
    $(QTDIR64)/include/QtSerialPort \
    $(QTDIR64)/include/QtWidgets

HEADERS += \
    hexFile.h \
    guiMainWindow.h \
    initThread.h \
    qLedWidget.h \
    readThread.h \
    E8755Thread.h \
    E2708Thread.h \
    E2716Thread.h \
    TMS2716Thread.h \
    E2532Thread.h \
	E2732Thread.h

SOURCES += \
    hexFile.cpp \
    guiMainWindow.cpp \
    initThread.cpp \
    main.cpp \
    qLedWidget.cpp \
    readThread.cpp \
    E8755Thread.cpp \
    E2708Thread.cpp \
    E2716Thread.cpp \
    TMS2716Thread.cpp \
    E2532Thread.cpp \
    E2732Thread.cpp

FORMS += \
    guiMainWindow.ui

ICON = chip.icns
RC_ICONS = chip.ico
