#ifndef HIKCAMERAPLUGIN_H
#define HIKCAMERAPLUGIN_H


#include "../../CameraPlugin/CameraPluginInterface.h" // 插件接口
#include <QObject>

// ============================================================
// HikCameraPlugin：海康相机插件入口类
// ============================================================
class HikCameraPlugin final : public QObject, public CameraPluginInterface {
Q_OBJECT // Qt 元对象系统宏（必须）
Q_PLUGIN_METADATA(IID CameraPluginInterface_iid FILE "HikCameraPlugin.json")
    // 声明实现的接口（qobject_cast 能识别）
    Q_INTERFACES(CameraPluginInterface)

        public : QString pluginId() const override; // 插件唯一 ID
    int interfaceVersion() const override; // 接口版本（必须=1才加载）
    QStringList supportedVendors() const override; // 支持的厂商名列表
    uint32_t enumerateCameras(QVector<CameraMetaInfo>& cameras) override; // 枚举海康相机
    CameraInterface* createCamera(const CameraMetaInfo& info) override; // 创建 HikCamera 对象
    void destroyCamera(CameraInterface* camera) override; // 销毁 HikCamera 对象
};

#endif // HIKCAMERAPLUGIN_H
