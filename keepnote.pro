QT += core gui qml quick quickcontrols2 quickdialogs2
CONFIG += c++17 release
TARGET = keepnote
TEMPLATE = app
HEADERS += src/backend.h src/syntaxhighlighter.h
SOURCES += src/main.cpp src/backend.cpp src/syntaxhighlighter.cpp
RESOURCES += src/resources.qrc

macx {
    ICON = macos/keepnote.icns
    QMAKE_TARGET_BUNDLE_PREFIX = io.keepnote
}

win32 {
    CONFIG -= debug_and_release debug_and_release_target
    RC_ICONS = windows/keepnote.ico
    QMAKE_TARGET_DESCRIPTION = Keepnote
    QMAKE_TARGET_PRODUCT = Keepnote
}
msvc: QMAKE_CXXFLAGS += /utf-8
