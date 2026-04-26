#ifndef CAMERAPLUGININTERFACE_H
#define CAMERAPLUGININTERFACE_H


#include "../CameraInterface/CameraInterface.h"  // 相机抽象接口（插件创建的相机对象类型）
#include <QStringList>
#include <QVector>

// ============================================================
// ============================================================
class CameraPluginInterface {
public:
    virtual ~CameraPluginInterface() = default;  // 虚析构（接口基类必须有）

    virtual QString pluginId() const = 0;

    virtual int interfaceVersion() const = 0;

    virtual QStringList supportedVendors() const = 0;

    // 返回所有相机的元信息列表（序列号、厂商名等）
    virtual uint32_t enumerateCameras(QVector<CameraMetaInfo>& cameras) = 0;

    virtual CameraInterface* createCamera(const CameraMetaInfo& info) = 0;

    virtual void destroyCamera(CameraInterface* camera) = 0;
};

// ============================================================
// Qt 插件系统声明
// ============================================================
#define CameraPluginInterface_iid "com.vcs.camera.CameraPluginInterface/1.0"
Q_DECLARE_INTERFACE(CameraPluginInterface, CameraPluginInterface_iid)

#endif // CAMERAPLUGININTERFACE_H
