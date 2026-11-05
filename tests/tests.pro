QT += core gui quick testlib
CONFIG += c++17 testcase
CONFIG -= app_bundle
TARGET = keepnote_tests
TEMPLATE = app
INCLUDEPATH += ../src
HEADERS += ../src/backend.h ../src/syntaxhighlighter.h
SOURCES += keepnote_tests.cpp ../src/backend.cpp ../src/syntaxhighlighter.cpp

win32: CONFIG -= debug_and_release debug_and_release_target
msvc: QMAKE_CXXFLAGS += /utf-8
