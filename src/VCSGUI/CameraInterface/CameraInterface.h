#ifndef CAMERAINTERFACE_H
#define CAMERAINTERFACE_H


#include "CMCameraMetaInfo.h" // 相机元信息（序列号、用户名、厂商名）
#include "CMCameraParam.h" // 统一参数模型（六种参数类型）
#include "CameraError.h" // 错误码定义
#include "CameraImageQueue.h" // 线程安全图像队列
#include "opencv2/core.hpp" // OpenCV 核心（cv::Mat）
#include <QtPlugin> // Qt 插件宏支持

// ============================================================
// CameraInterface：相机抽象接口类
// ============================================================
class CameraInterface {
// TODO: 补充更多虚函数接口
public:
    // 注意：构造时不连接相机，只保存标识信息
    CameraInterface(const CameraMetaInfo& info)
    {
        m_cameraInfo = info;
    }

    virtual ~CameraInterface() { }

    virtual QString UserName()
    {
        return m_cameraInfo.UserDefineID;
    }

    virtual QString Serial()
    {
        return m_cameraInfo.Serial;
    }

    // ---------- [1] 参数接口组 ----------
    // 注意：这里只获取参数"有哪些"，不读取参数当前值
    virtual uint32_t getParamList(QVector<CameraParam>& paramList) = 0;

    // ---------- [2] 状态查询接口组 ----------
    virtual bool isConnect() = 0;

    virtual bool isGrabbing() = 0;

    // ---------- [3] 生命周期接口组 ----------
    virtual uint32_t acquire() = 0;

    virtual uint32_t release() = 0;

    virtual uint32_t connect() = 0;

    // 断开连接（关闭设备），与 connect 配对
    virtual uint32_t disconnect() = 0;

    // ---------- [4] 拉流控制接口组 ----------
    // 创建拉流资源（注册回调函数、分配图像缓冲区）
    virtual uint32_t creatStream() = 0;

    virtual uint32_t destroyStream() = 0;

    virtual uint32_t startGrabbing() = 0;

    virtual uint32_t stopGrabbing() = 0;

    // ---------- [5] 配置文件接口组 ----------
    virtual uint32_t loadConfig(const QString path) = 0;

    // 导出相机配置文件（将相机当前参数保存到文件）
    virtual uint32_t saveConfig(const QString path) = 0;

    virtual QString configFormat() = 0;

    // ---------- [6] 参数读写接口组 ----------
    virtual uint32_t readParam(CameraParam& param) = 0;

    // 写入相机单个参数（将参数值下发到相机硬件）
    virtual uint32_t writeParam(CameraParam& param) = 0;

    // ---------- [7] 图像获取接口组 ----------
    virtual uint32_t getImageLast(cv::Mat& image) = 0;

    virtual CameraImageQueue& ImageQueue()
    {
        return m_imageQueue;
    }

protected:
    CameraImageQueue m_imageQueue;

    QVector<CameraParam> m_cameraParams;

    CameraMetaInfo m_cameraInfo;
};

#endif // CAMERAINTERFACE_H
