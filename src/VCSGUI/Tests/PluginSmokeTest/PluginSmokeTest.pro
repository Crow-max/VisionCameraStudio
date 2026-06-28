# 【小白导读】

QT += core

TEMPLATE = app
TARGET = PluginSmokeTest
CONFIG += console c++17
CONFIG -= app_bundle

SOURCES += \
    main.cpp \
    ../../CameraPlugin/CameraPluginManager.cpp

# 头文件
HEADERS += \
    ../../CameraInterface/CMCameraMetaInfo.h \
    ../../CameraInterface/CMCameraParam.h \
    ../../CameraInterface/CameraError.h \
    ../../CameraInterface/CameraImageQueue.h \
    ../../CameraInterface/CameraInterface.h \
    ../../CameraPlugin/CameraPluginInterface.h \
    ../../CameraPlugin/CameraPluginManager.h

# 输出目录：bin
DESTDIR = $$PWD/../../../../bin

# OpenCV 头文件和库
INCLUDEPATH += D:/Study/OpenCV/opencv-cuda-4.10.0/install/include
CONFIG(debug, debug|release) {
    LIBS += -lD:/Study/OpenCV/opencv-cuda-4.10.0/install/x64/vc17/lib/opencv_world4100d
} else {
    LIBS += -lD:/Study/OpenCV/opencv-cuda-4.10.0/install/x64/vc17/lib/opencv_world4100
}
