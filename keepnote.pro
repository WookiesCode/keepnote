QT += core gui qml quick quickcontrols2 quickdialogs2
CONFIG += c++17 release
TARGET = keepnote
TEMPLATE = app
HEADERS += src/backend.h src/syntaxhighlighter.h
SOURCES += src/main.cpp src/backend.cpp src/syntaxhighlighter.cpp
RESOURCES += src/resources.qrc
