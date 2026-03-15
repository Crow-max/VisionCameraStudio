#ifndef VIRTUALCAMERA_H
#define VIRTUALCAMERA_H


#include "../CameraInterface/CameraInterface.h" // 相机抽象接口
#include <QHash>
#include <QObject>
#include <atomic> // 原子变量（多线程安全的标志位）
#include <thread> // C++11 线程库

// ============================================================
// VirtualCamera：虚拟相机实现类
// ============================================================
class VirtualCamera
    : public CameraInterface {
public:
    // 虚拟相机的固定标识（枚举时用）
    static const QString VIRTUAL_CAMERA_NAME;
    static const QString VIRTUAL_CAMERA_SERIAL; // 序列号 "Vir123456"
    static const QString VIRTUAL_CAMERA_VENDER; // 厂商名 "Virtual"

    explicit VirtualCamera(const CameraMetaInfo& info);
    ~VirtualCamera(); // 析构时自动停止拉流

    // ---------- 静态函数 ----------
    static uint32_t EnumCamera(QVector<CameraMetaInfo>& cameraInfos);

    // ---------- 接口实现 ----------
    uint32_t getParamList(QVector<CameraParam>& paramList) override; // 从 JSON 资源加载参数表
    bool isConnect() override; // 是否已连接（查内部标志）
    bool isGrabbing() override; // 是否正在拉流（查原子标志）
    uint32_t acquire() override;
    uint32_t release() override; // 释放（停止拉流）
    uint32_t connect() override; // 连接（设置标志为 true）
    uint32_t disconnect() override;
    uint32_t creatStream() override; // 创建拉流资源（不需要）
    uint32_t destroyStream() override; // 销毁拉流资源（不需要）
    uint32_t startGrabbing() override;
    uint32_t stopGrabbing() override;
    uint32_t loadConfig(const QString path) override; // 从 Ini 文件加载参数
    uint32_t saveConfig(const QString path) override; // 保存参数到 Ini 文件
    QString configFormat() override; // 配置文件格式 "ini"
    uint32_t readParam(CameraParam& param) override; // 读取参数（默认值+配置覆盖）
    uint32_t writeParam(CameraParam& param) override;
    uint32_t getImageLast(cv::Mat& image) override; // 从队列取一帧（拷贝返回）

    // 获取图像队列引用（拉流线程里用）
    CameraImageQueue& getImageQueue()
    {
        return m_imageQueue;
    }

private:
    bool m_connect = false; // 连接状态标志
    std::atomic_bool m_startGrabbing { false }; // 拉流状态原子标志（多线程安全）
    std::thread m_grabThread; // 拉流模拟线程
    int m_cameraIndex = 1;
    QHash<QString, QString> m_configValues;
};

#endif // VIRTUALCAMERA_H
