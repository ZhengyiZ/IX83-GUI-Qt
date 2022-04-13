QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    base/cmdthread.cpp \
    base/klabel.cpp \
    base/main.cpp \
    gui/ifselection.cpp \
    gui/mainwindow.cpp \
    gui/setwindow.cpp

HEADERS += \
    base/DLL.h \
    base/cmd.h \
    base/cmdthread.h \
    base/klabel.h \
    gui/ifselection.h \
    gui/mainwindow.h \
    gui/setwindow.h

FORMS += \
    gui/ifselection.ui \
    gui/mainwindow.ui \
    gui/setwindow.ui

CONFIG += lrelease
CONFIG += embed_translations

RC_ICONS = res/icon.ico

# Copy lib files to build directory
LibFile = $$PWD/lib/*.*
CONFIG(release, debug | release) {
    OutLibFile = $${OUT_PWD}/release/*.*
}else {
    OutLibFile = $${OUT_PWD}/debug/*.*
}
LibFile = $$replace(LibFile, /, \\)
OutLibFile = $$replace(OutLibFile, /, \\)
QMAKE_PRE_LINK += copy $$LibFile $$OutLibFile

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
