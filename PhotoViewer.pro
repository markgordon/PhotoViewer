TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt
INCLUDEPATH += /usr/include/opencv4 /usr/local/include
# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
LIBS += -L/usr/lib -L/usr/local/lib -lopencv_world -pthread
LIBS += -lstdc++fs

SOURCES += \
        main.cpp \
    exif.cpp

HEADERS += \
    exif.h
