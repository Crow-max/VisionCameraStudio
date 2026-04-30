# 【小白导读】
#   主要配置：

QT += core

TEMPLATE = lib
TARGET = HikCameraPlugin
CONFIG += plugin c++17
DEFINES += QT_DEPRECATED_WARNINGS
QMAKE_CXXFLAGS += -wd4828

SOURCES += \
    HikCameraPlugin.cpp \
    ../../CameraFactory/HikCamera.cpp \
    ../../CameraInterface/CameraImageQueue.cpp

# 头文件
HEADERS += \
    HikCameraPlugin.h \
    ../../CameraFactory/HikCamera.h \
    ../../CameraInterface/CMCameraMetaInfo.h \
    ../../CameraInterface/CMCameraParam.h \
    ../../CameraInterface/CameraError.h \
    ../../CameraInterface/CameraImageQueue.h \
    ../../CameraInterface/CameraInterface.h \
    ../../CameraPlugin/CameraPluginInterface.h

DESTDIR = $$PWD/../../../../bin/plugins/cameras

INCLUDEPATH += \
    D:/Study/OpenCV/opencv-cuda-4.10.0/install/include \
    $$PWD/../../../../depends/HikCamera/Includes

CONFIG(debug, debug|release) {
    LIBS += -lD:/Study/OpenCV/opencv-cuda-4.10.0/install/x64/vc17/lib/opencv_world4100d
} else {
    LIBS += -lD:/Study/OpenCV/opencv-cuda-4.10.0/install/x64/vc17/lib/opencv_world4100
}
# 海康 MVS SDK 库
LIBS += -l$$PWD/../../../../depends/HikCamera/Libraries/MvCameraControl
