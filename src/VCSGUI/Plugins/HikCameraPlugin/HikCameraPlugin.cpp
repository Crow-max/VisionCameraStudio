#include "HikCameraPlugin.h"
#include "../../CameraFactory/HikCamera.h"


// 插件唯一 ID（主程序用它去重和识别）
QString HikCameraPlugin::pluginId() const
{
    return "com.vcs.camera.hikrobot";
}

int HikCameraPlugin::interfaceVersion() const
{
    return 1;
}

QStringList HikCameraPlugin::supportedVendors() const
{
    return { HikCamera::VIRTUAL_CAMERA_VENDER, "Hikvision" };
}

uint32_t HikCameraPlugin::enumerateCameras(QVector<CameraMetaInfo>& cameras)
{
    return HikCamera::EnumCamera(cameras);
}

CameraInterface* HikCameraPlugin::createCamera(const CameraMetaInfo& info)
{
    if (!supportedVendors().contains(info.VenderName, Qt::CaseInsensitive)) {
        return nullptr;
    }
    return new HikCamera(info);  // 创建海康相机对象
}

void HikCameraPlugin::destroyCamera(CameraInterface* camera)
{
    delete camera;
}
