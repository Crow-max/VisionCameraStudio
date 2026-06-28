#include "../../CameraInterface/CameraError.h"
#include "../../CameraPlugin/CameraPluginManager.h"
#include <QCoreApplication>
#include <QDebug>
#include <algorithm>


namespace {
// 辅助函数：打印失败信息并返回 1
int fail(const QString& message)
{
    qCritical().noquote() << "FAIL:" << message;
    return 1;
}
}

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    CameraPluginManager& manager = CameraPluginManager::instance();  // 获取插件管理器单例

    // 1. 加载所有插件
    if (!manager.loadPlugins()) {
        return fail(manager.diagnostics().join('\n'));
    }
    // 2. 验证虚拟相机插件已加载
    if (!manager.loadedPluginIds().contains("com.vcs.camera.virtual")) {
        return fail("Virtual camera plugin was not discovered.");
    }

    // 3. 枚举相机，验证至少有2台虚拟相机
    QVector<CameraMetaInfo> cameras;
    manager.enumerateCameras(cameras);
    const auto virtualCameraCount = std::count_if(cameras.cbegin(), cameras.cend(), [](const CameraMetaInfo& info) {
        return info.VenderName.compare("Virtual", Qt::CaseInsensitive) == 0;
    });
    if (virtualCameraCount < 2) {
        return fail(QString("Expected two virtual cameras, found %1.").arg(virtualCameraCount));
    }
    // 找到第一台虚拟相机
    auto virtualCameraInfo = std::find_if(cameras.cbegin(), cameras.cend(), [](const CameraMetaInfo& info) {
        return info.VenderName.compare("Virtual", Qt::CaseInsensitive) == 0;
    });
    if (virtualCameraInfo == cameras.cend()) {
        return fail("Virtual camera was not enumerated.");
    }

    // 4. 创建虚拟相机实例
    CameraInterface* camera = manager.createCamera(*virtualCameraInfo);
    if (!camera) {
        return fail("Virtual camera instance was not created.");
    }

    // 5. 完整生命周期测试
    bool success = camera->acquire() == CHONGMING_OK        // 创建句柄
        && camera->connect() == CHONGMING_OK                  // 打开设备
        && camera->startGrabbing() == CHONGMING_OK;           // 开始拉流
    cv::Mat image;
    // 取一帧图像，验证不为空
    success = success && camera->getImageLast(image) == CHONGMING_OK && !image.empty();
    // 停止拉流 + 断开 + 释放
    success = success && camera->stopGrabbing() == CHONGMING_OK
        && camera->disconnect() == CHONGMING_OK
        && camera->release() == CHONGMING_OK;

    // 6. 销毁相机实例
    manager.destroyCamera(camera);

    if (!success) {
        return fail("Virtual camera lifecycle or image acquisition failed.");
    }
    // 7. 卸载插件
    if (!manager.unloadPlugins()) {
        return fail(manager.diagnostics().join('\n'));
    }

    // 全部通过
    qInfo().noquote() << "PASS: plugin discovery, camera creation, lifecycle and image acquisition";
    return 0;
}
