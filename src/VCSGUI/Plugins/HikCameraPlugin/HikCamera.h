#ifndef HIKCAMERA_H
#define HIKCAMERA_H


#include "../CameraInterface/CameraInterface.h" // 相机抽象接口（必须继承）
#include "MvCameraControl.h" // 海康 MVS SDK 头文件

// ============================================================
// HikCamera：海康相机实现类
// * 由 HikCameraPlugin 创建和销毁
// ============================================================
class HikCamera
    : public CameraInterface {
public:
    // 海康相机的厂商名常量（插件用它判断是否支持该厂商）
    static const QString VIRTUAL_CAMERA_VENDER;

    // 构造函数：接收相机元信息（序列号、用户名、厂商名）
    HikCamera(const CameraMetaInfo& info);
    ~HikCamera() override;

    // ---------- 静态函数 ----------
    static uint32_t EnumCamera(QVector<CameraMetaInfo>& cameraInfos);

    // ---------- 接口实现（override 表示重写基类纯虚函数） ----------
    uint32_t getParamList(QVector<CameraParam>& paramList) override;
    bool isConnect() override; // 是否已连接（查 SDK 状态）
    bool isGrabbing() override; // 是否正在拉流（查内部标志）
    uint32_t acquire() override;
    uint32_t release() override; // 释放：销毁句柄
    uint32_t connect() override;
    uint32_t disconnect() override; // 断开：取消回调+关闭设备
    uint32_t creatStream() override;
    uint32_t destroyStream() override;
    uint32_t startGrabbing() override; // 开启拉流
    uint32_t stopGrabbing() override; // 停止拉流
    uint32_t loadConfig(const QString path) override;
    uint32_t saveConfig(const QString path) override;
    QString configFormat() override; // 配置文件格式后缀（"mfs"）
    uint32_t readParam(CameraParam& param) override;
    uint32_t writeParam(CameraParam& param) override;
    uint32_t getImageLast(cv::Mat& image) override; // 从图像队列取一帧

    // 获取海康相机句柄（回调函数和像素转换需要用）
    void* CameraHandle()
    {
        return m_cameraHandle;
    }

private:
    uint32_t getFeatureAccessMode(CameraParam& param);

private:
    void* m_cameraHandle = NULL;
    MV_CC_DEVICE_INFO* m_pDeviceInfo = NULL;

    bool isStartGrabbing = false;
};

#endif // HIKCAMERA_H
