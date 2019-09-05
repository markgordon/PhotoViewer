TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt
INCLUDEPATH += /usr/local/include /usr/local/include/opencv4
# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
LIBS += -L/usr/lib -L/usr/local/lib/      -lopencv_core \
    -lopencv_dnn      -lopencv_features2d  -lopencv_flann \
      -lopencv_highgui  -lopencv_imgcodecs    \
-lopencv_imgproc    -lopencv_ml  -lopencv_objdetect     \
               \
  -lopencv_stitching         \
   -lopencv_videoio  -lopencv_video      \
    -pthread -lboost_thread
LIBS += /usr/local/lib/libboost_filesystem.a -lboost_chrono

SOURCES += \
        main.cpp
