QT += core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MyNotepad
TEMPLATE = app
CONFIG += c++17

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    aboutdialog.cpp \
    finddialog.cpp \
    replacedialog.cpp \
    codeeditor.cpp

HEADERS += \
    mainwindow.h \
    aboutdialog.h \
    finddialog.h \
    replacedialog.h \
    codeeditor.h

FORMS += mainwindow.ui

RESOURCES += res.qrc
