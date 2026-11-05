QT += core gui quick testlib
CONFIG += c++17 testcase
TARGET = keepnote_tests
TEMPLATE = app
INCLUDEPATH += ../src
HEADERS += ../src/backend.h ../src/syntaxhighlighter.h
SOURCES += keepnote_tests.cpp ../src/backend.cpp ../src/syntaxhighlighter.cpp
