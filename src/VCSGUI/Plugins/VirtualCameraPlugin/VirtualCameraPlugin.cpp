#include "VirtualCameraPlugin.h"
#include "../../CameraFactory/VirtualCamera.h"


QString VirtualCameraPlugin::pluginId() const
{
    return "com.vcs.camera.virtual";
}

int VirtualCameraPlugin::interfaceVersion() const
{
    return 1;
}

// 只支持 "Virtual" 厂商
QStringList VirtualCameraPlugin::supportedVendors() const
{
    return { VirtualCamera::VIRTUAL_CAMERA_VENDER };
}

uint32_t VirtualCameraPlugin::enumerateCameras(QVector<CameraMetaInfo>& cameras)
{
    return VirtualCamera::EnumCamera(cameras);
}

// 创建 VirtualCamera 对象
CameraInterface* VirtualCameraPlugin::createCamera(const CameraMetaInfo& info)
{
    if (!supportedVendors().contains(info.VenderName, Qt::CaseInsensitive)) {
        return nullptr;
    }
    return new VirtualCamera(info);
}

// 销毁 VirtualCamera 对象
void VirtualCameraPlugin::destroyCamera(CameraInterface* camera)
{
    delete camera;
}
