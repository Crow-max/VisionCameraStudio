#include "CameraContext.h"
#include "../CameraPlugin/CameraPluginManager.h"
#include "../Utils/ImageConver.h"
#include "CMCameraMetaInfo.h"
#include "CMCameraParam.h"
#include "CameraImageQueue.h"
#include "CameraInterface.h"
#include <QApplication>
#include <QDebug>
#include <QDir>


// 单例静态成员定义（必须在类外定义，分配内存）
CameraContext* CameraContext::m_pContext = Q_NULLPTR;

// ============================================================
// Instance：获取单例指针（懒汉式）
//       但本项目在主线程初始化，所以没问题
// ============================================================
CameraContext* CameraContext::Instance()
{
    if (Q_NULLPTR == m_pContext) {
        m_pContext = new CameraContext();
    }
    return m_pContext;
}

// ============================================================
// Release：释放单例
// 程序退出时调用，delete 实例并置空
// ============================================================
void CameraContext::Release()
{
    if (Q_NULLPTR != m_pContext) {
        delete m_pContext;
        m_pContext = Q_NULLPTR;
    }
}

CameraContext::CameraContext
    // TODO: 完善错误处理()
{
}

// 析构函数：清理所有相机实例 + 卸载插件
CameraContext::~CameraContext
    // TODO: 排查析构时偶发崩溃问题()
{
    clearCameras();
    CameraPluginManager::instance().unloadPlugins();
}

// ============================================================
// clearCameras：清空所有相机实例
// ============================================================
void CameraContext::clearCameras()
{
    // 遍历映射表中的所有相机指针
    for (CameraInterface* camera : m_serialCamMap) {
        // 如果正在拉流，先停止
        if (camera->isGrabbing()) {
            camera->ImageQueue().Stop();   // 先停止图像队列，唤醒等待线程
            camera->stopGrabbing();         // 停止相机拉流
            camera->destroyStream();        // 销毁拉流资源
        }
        // 如果已连接，断开连接
        if (camera->isConnect()) {
            camera->disconnect();
        }
        // 释放相机资源（销毁 SDK 句柄）
        camera->release();
        // 通过插件管理器销毁相机对象（谁创建谁销毁）
        CameraPluginManager::instance().destroyCamera(camera);
    }
    m_serialCamMap.clear();  // 清空映射表
}

// ============================================================
// 流程：
//   1. 清空旧的相机实例
//   2. 通过插件管理器加载所有相机插件（.dll）
//   3. 让每个插件枚举它支持的相机
//   4. 为每个新相机创建实例，存入映射表
// ============================================================
uint32_t CameraContext::EnumerationCamera(QVector<CameraMetaInfo>& cameraInfos)
{
    clearCameras();  // 先清空旧实例，重新枚举

    // 获取插件管理器单例，加载所有相机插件
    CameraPluginManager& pluginManager = CameraPluginManager::instance();
    const bool pluginsLoaded = pluginManager.loadPlugins();
    qInfo() << "Camera plugin directory:" << pluginManager.pluginDirectory();
    qInfo() << "Loaded camera plugins:" << pluginManager.loadedPluginIds();
    if (!pluginManager.diagnostics().isEmpty()) {
        qWarning() << "Camera plugin diagnostics:" << pluginManager.diagnostics();
    }
    if (!pluginsLoaded) {
        return NOCAMERA_ERROR;  // 没有可用插件
    }

    // 让所有插件枚举相机，汇总到 infos
    QVector<CameraMetaInfo> infos;
    const uint32_t enumerationResult = pluginManager.enumerateCameras(infos);

    // 为每个新枚举到的相机创建实例
    for (auto info : infos) {
        // 检查是否已经在 cameraInfos 中（去重）
        QVector<CameraMetaInfo>::iterator it = std::find(cameraInfos.begin(), cameraInfos.end(), info);
        if (it == cameraInfos.end()) {
            // 新相机：加入列表
            cameraInfos.push_back(info);
            QString serial = info.Serial;
            CameraInterface* camera = pluginManager.createCamera(info);
            if (camera) {
                // 处理重复序列号的极端情况
                if (m_serialCamMap.contains(info.Serial)) {
                    qWarning() << "Duplicate camera serial, replacing previous instance:" << info.Serial;
                    CameraInterface* previous = m_serialCamMap.take(info.Serial);
                    // 清理旧实例
                    if (previous->isGrabbing()) {
                        previous->ImageQueue().Stop();
                        previous->stopGrabbing();
                    }
                    if (previous->isConnect()) {
                        previous->disconnect();
                    }
                    previous->release();
                    pluginManager.destroyCamera(previous);
                }
                // 存入映射表：序列号 → 相机指针
                m_serialCamMap[info.Serial] = camera;
                qDebug() << "创建相机成功:" << info.VenderName << info.Serial;
            } else {
                qWarning() << "创建相机失败，不支持的厂商:" << info.VenderName;
            }
        }
    }
    return enumerationResult;
}

// ============================================================
// getParamList：获取指定相机的参数列表
// ============================================================
uint32_t CameraContext::getParamList(const QString serial, QVector<CameraParam>& paramList)
{
    if (m_serialCamMap.find(serial) == m_serialCamMap.end())
        return NOCAMERA_ERROR;

    QVector<CameraParam> paramListTemp;
    auto camera = m_serialCamMap[serial];
    camera->getParamList(paramListTemp);  // 获取参数元信息列表

    // 逐个读取当前值
    for (auto var : paramListTemp) {
        camera->readParam(var);  // 从相机硬件读取当前值
        paramList.push_back(var);
    }

    return CHONGMING_OK;
}

// ---------- 状态查询（简单转发） ----------
uint32_t CameraContext::isConnect(const QString serial, bool& state)
{
    if (m_serialCamMap.find(serial) == m_serialCamMap.end())
        return NOCAMERA_ERROR;
    auto camera = m_serialCamMap[serial];
    state = camera->isConnect();
    return CHONGMING_OK;
}

uint32_t CameraContext::isGrabbing(const QString serial, bool& state)
{
    if (m_serialCamMap.find(serial) == m_serialCamMap.end())
        return NOCAMERA_ERROR;
    auto camera = m_serialCamMap[serial];
    state = camera->isGrabbing();
    return CHONGMING_OK;
}

// ============================================================
// connect：连接相机
// ============================================================
uint32_t CameraContext::connect(const QString serial)
{
    if (m_serialCamMap.find(serial) == m_serialCamMap.end())
        return NOCAMERA_ERROR;

    auto camera = m_serialCamMap[serial];
    // 第一步：初始化相机（创建 SDK 句柄）
    auto ret = camera->acquire();
    if (ret != CHONGMING_OK)
        return ret;
    // 第二步：连接相机（打开设备）
    ret = camera->connect();
    if (ret != CHONGMING_OK) {
        camera->release();
        return ret;
    }

    return CHONGMING_OK;
}

// ============================================================
// disconnect：断开连接
// 与 connect 逆序
// ============================================================
uint32_t CameraContext::disconnect(const QString serial)
{
    if (m_serialCamMap.find(serial) == m_serialCamMap.end())
        return NOCAMERA_ERROR;

    auto camera = m_serialCamMap[serial];
    auto ret = camera->disconnect();  // 关闭设备
    if (ret != CHONGMING_OK)
        return ret;

    ret = camera->release();  // 销毁句柄
    if (ret != CHONGMING_OK)
        return ret;

    return CHONGMING_OK;
}

// ============================================================
// startGrabbing：开启拉流
// 前置条件：相机已连接
// ============================================================
uint32_t CameraContext::startGrabbing(const QString serial)
{
    if (m_serialCamMap.find(serial) == m_serialCamMap.end())
        return NOCAMERA_ERROR;

    auto camera = m_serialCamMap[serial];

    // 前置检查：必须已连接
    if (camera->isConnect() != true)
        return CAMERA_NOT_CONNECTED;

    // 已经在拉流了，直接返回成功（幂等）
    if (camera->isGrabbing() == true)
        return CHONGMING_OK;

    camera->ImageQueue().Start();

    // 创建拉流资源（注册图像回调函数、分配图像缓冲区）
    auto ret = camera->creatStream();
    if (ret != CHONGMING_OK)
        return ret;

    // 开启拉流（相机开始持续输出图像）
    ret = camera->startGrabbing();
    if (ret != CHONGMING_OK)
    {
        // 回滚：开启失败要清理已创建的资源
        camera->ImageQueue().Stop();
        camera->destroyStream();
        return ret;
    }

    return CHONGMING_OK;
}

// ============================================================
// stopGrabbing：停止拉流
// 与 startGrabbing 逆序
// ============================================================
uint32_t CameraContext::stopGrabbing(const QString serial)
{
    if (m_serialCamMap.find(serial) == m_serialCamMap.end())
        return NOCAMERA_ERROR;

    auto camera = m_serialCamMap[serial];

    // 没连接或没在拉流，直接返回（幂等）
    if (camera->isConnect() == false || camera->isGrabbing() == false)
        return CHONGMING_OK;

    camera->ImageQueue().Stop();  // 停止队列，唤醒等待中的取图线程

    auto ret = camera->stopGrabbing();  // 停止相机拉流
    if (ret != CHONGMING_OK)
        return ret;

    ret = camera->destroyStream();  // 销毁拉流资源
    if (ret != CHONGMING_OK)
        return ret;

    return CHONGMING_OK;
}

// ---------- 配置文件导入导出 ----------
uint32_t CameraContext::loadConfig(const QString serial, const QString path)
{
    if (m_serialCamMap.find(serial) == m_serialCamMap.end())
        return NOCAMERA_ERROR;

    auto camera = m_serialCamMap[serial];
    if (camera->isConnect() == false || camera->isGrabbing() == true)
        return CAMERA_NOT_CONNECTED;

    auto ret = camera->loadConfig(path);
    if (ret != CHONGMING_OK)
        return ret;

    return CHONGMING_OK;
}

uint32_t CameraContext::saveConfig(const QString serial, const QString path)
{
    if (m_serialCamMap.find(serial) == m_serialCamMap.end())
        return NOCAMERA_ERROR;

    auto camera = m_serialCamMap[serial];
    if (camera->isConnect() == false || camera->isGrabbing() == true)
        return CAMERA_NOT_CONNECTED;

    auto ret = camera->saveConfig(path);
    if (ret != CHONGMING_OK)
        return ret;

    return CHONGMING_OK;
}

QString CameraContext::getConfigFormat(const QString serial)
{
    if (m_serialCamMap.find(serial) == m_serialCamMap.end())
        return "";

    CameraInterface* camera = m_serialCamMap[serial];
    QString format = camera->configFormat();

    return QString(format.data());
}

// ---------- 参数读写（简单转发） ----------
uint32_t CameraContext::readParam(const QString serial, CameraParam& param)
{
    if (m_serialCamMap.find(serial) == m_serialCamMap.end())
        return NOCAMERA_ERROR;

    CameraInterface* camera = m_serialCamMap[serial];
    auto ret = camera->readParam(param);
    if (ret != CHONGMING_OK)
        return ret;

    return CHONGMING_OK;
}

uint32_t CameraContext::writeParam(const QString serial, CameraParam& param)
{
    if (m_serialCamMap.find(serial) == m_serialCamMap.end())
        return NOCAMERA_ERROR;

    CameraInterface* camera = m_serialCamMap[serial];
    auto ret = camera->writeParam(param);
    if (ret != CHONGMING_OK)
        return ret;

    return CHONGMING_OK;
}

// ============================================================
// getImageLast：获取最新一帧图像
// 注意：这是阻塞调用，队列空时会等待最多 5 秒
// ============================================================
uint32_t CameraContext::getImageLast(const QString serial, QImage& image)
{
    if (m_serialCamMap.find(serial) == m_serialCamMap.end())
        return NOCAMERA_ERROR;

    CameraInterface* camera = m_serialCamMap[serial];
    cv::Mat cvImage;
    auto ret = camera->getImageLast(cvImage);
    if (ret != CHONGMING_OK)
        return GETIAMGE_TIMEOUT;

    image = ImageConver::cvMat2QImage(cvImage);

    return CHONGMING_OK;
}

// ============================================================
// wakeImageWait：唤醒正在等待图像的线程
// ============================================================
void CameraContext::wakeImageWait(const QString serial)
{
    auto camera = m_serialCamMap.value(serial, nullptr);
    if (camera) {
        camera->ImageQueue().Stop();
    }
}
