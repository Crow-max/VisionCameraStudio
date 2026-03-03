# 【小白导读】
#   主要配置：

QT += core gui concurrent widgets

TEMPLATE = app
TARGET = VCSGUI
CONFIG += c++17
DEFINES += QT_DEPRECATED_WARNINGS

# 源文件（.cpp）
SOURCES += \
    AppStyle/AppStyle.cpp \
    CameraInterface/CameraContext.cpp \
    CameraInterface/CameraImageQueue.cpp \
    CameraPlugin/CameraPluginManager.cpp \
    ControlWidget/ControlWidget.cpp \
    Listener.cpp \
    LoadingDialog/LoadingDialog.cpp \
    ParamWidget/CameraParamDelegate.cpp \
    ParamWidget/CameraParamItem.cpp \
    ParamWidget/CameraParamModel.cpp \
    ParamWidget/CustomWidget/BoolCustomWidget.cpp \
    ParamWidget/CustomWidget/CmdCustomWidget.cpp \
    ParamWidget/CustomWidget/DoubleCustomWidget.cpp \
    ParamWidget/CustomWidget/EnumCustomWidget.cpp \
    ParamWidget/CustomWidget/IntCustomWidget.cpp \
    ParamWidget/CustomWidget/StringCustomWidget.cpp \
    ParamWidget/OneCustomWidget.cpp \
    ParamWidget/ParamWidget.cpp \
    ParseUiJson/ParseUiJson.cpp \
    ViewWidget/AcquireImageProcess.cpp \
    ViewWidget/GraphicsView.cpp \
    ViewWidget/ImageItem.cpp \
    ViewWidget/ViewWidget.cpp \
    main.cpp \
    mainwindow.cpp

# 头文件（.h）
HEADERS += \
    AppStyle/AppStyle.h \
    CameraInterface/CMCameraMetaInfo.h \
    CameraInterface/CMCameraParam.h \
    CameraInterface/CameraContext.h \
    CameraInterface/CameraError.h \
    CameraInterface/CameraImageQueue.h \
    CameraInterface/CameraInterface.h \
    CameraPlugin/CameraPluginInterface.h \
    CameraPlugin/CameraPluginManager.h \
    ControlWidget/ControlWidget.h \
    Listener.h \
    LoadingDialog/LoadingDialog.h \
    ParamWidget/CameraParamDelegate.h \
    ParamWidget/CameraParamItem.h \
    ParamWidget/CameraParamModel.h \
    ParamWidget/CustomWidget/BoolCustomWidget.h \
    ParamWidget/CustomWidget/CmdCustomWidget.h \
    ParamWidget/CustomWidget/DoubleCustomWidget.h \
    ParamWidget/CustomWidget/EnumCustomWidget.h \
    ParamWidget/CustomWidget/IntCustomWidget.h \
    ParamWidget/CustomWidget/StringCustomWidget.h \
    ParamWidget/OneCustomWidget.h \
    ParamWidget/ParamWidget.h \
    ParseUiJson/ParseUiJson.h \
    Utils/ImageConver.h \
    ViewWidget/AcquireImageProcess.h \
    ViewWidget/GraphicsView.h \
    ViewWidget/ImageItem.h \
    ViewWidget/ViewWidget.h \
    mainwindow.h

FORMS += \
    ControlWidget/ControlWidget.ui \
    LoadingDialog/LoadingDialog.ui \
    ParamWidget/ParamWidget.ui \
    ViewWidget/ViewWidget.ui \
    mainwindow.ui

RESOURCES += \
    AppStyle/style.qrc \
    Icon/Icon.qrc

DESTDIR = $$PWD/../../bin

# OpenCV 头文件路径
INCLUDEPATH += D:/Study/OpenCV/opencv-cuda-4.10.0/install/include
CONFIG(debug, debug|release) {
    LIBS += -lD:/Study/OpenCV/opencv-cuda-4.10.0/install/x64/vc17/lib/opencv_world4100d
} else {
    LIBS += -lD:/Study/OpenCV/opencv-cuda-4.10.0/install/x64/vc17/lib/opencv_world4100
}

# Windows 程序图标
RC_ICONS = favicon.ico
