# 【小白导读】

QT += core

TEMPLATE = lib
TARGET = VirtualCameraPlugin
CONFIG += plugin c++17
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    VirtualCameraPlugin.cpp \
    ../../CameraFactory/VirtualCamera.cpp \
    ../../CameraInterface/CameraImageQueue.cpp \
    ../../ParseUiJson/ParseUiJson.cpp

# 头文件
HEADERS += \
    VirtualCameraPlugin.h \
    ../../CameraFactory/VirtualCamera.h \
    ../../CameraInterface/CMCameraMetaInfo.h \
    ../../CameraInterface/CMCameraParam.h \
    ../../CameraInterface/CameraError.h \
    ../../CameraInterface/CameraImageQueue.h \
    ../../CameraInterface/CameraInterface.h \
    ../../CameraPlugin/CameraPluginInterface.h \
    ../../ParseUiJson/ParseUiJson.h

RESOURCES += ../../Resource/resource.qrc

DESTDIR = $$PWD/../../../../bin/plugins/cameras

# OpenCV 头文件和库
INCLUDEPATH += D:/Study/OpenCV/opencv-cuda-4.10.0/install/include
CONFIG(debug, debug|release) {
    LIBS += -lD:/Study/OpenCV/opencv-cuda-4.10.0/install/x64/vc17/lib/opencv_world4100d
} else {
    LIBS += -lD:/Study/OpenCV/opencv-cuda-4.10.0/install/x64/vc17/lib/opencv_world4100
}
