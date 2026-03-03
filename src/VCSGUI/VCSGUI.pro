# 【小白导读】
#
#   子项目包括：
#

TEMPLATE = subdirs
CONFIG += ordered

virtual_camera_plugin.file = Plugins/VirtualCameraPlugin/VirtualCameraPlugin.pro
hik_camera_plugin.file = Plugins/HikCameraPlugin/HikCameraPlugin.pro
application.file = VCSGUIApp.pro
plugin_smoke_test.file = Tests/PluginSmokeTest/PluginSmokeTest.pro

application.depends = virtual_camera_plugin hik_camera_plugin
# 测试依赖虚拟相机插件
plugin_smoke_test.depends = virtual_camera_plugin

# 子项目列表（按顺序编译）
SUBDIRS += \
    virtual_camera_plugin \
    hik_camera_plugin \
    application \
    plugin_smoke_test
