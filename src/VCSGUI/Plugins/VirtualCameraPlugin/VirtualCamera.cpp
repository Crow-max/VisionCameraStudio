#include "VirtualCamera.h"
#include "../ParseUiJson/ParseUiJson.h"
#include <QFileInfo>
#include <QSettings>
#include <chrono>
#include <ctime>
#include <thread>


// 虚拟相机的固定标识
const QString VirtualCamera::VIRTUAL_CAMERA_NAME = "VirtualCamera";
const QString VirtualCamera::VIRTUAL_CAMERA_SERIAL = "Vir123456";
const QString VirtualCamera::VIRTUAL_CAMERA_VENDER = "Virtual";

// ---------- 构造/析构 ----------
VirtualCamera::VirtualCamera(const CameraMetaInfo& info)
    : CameraInterface(info)
{
    m_cameraIndex = info.Serial.endsWith("002") ? 2 : 1;
}

VirtualCamera::~VirtualCamera()
{
    stopGrabbing(); // 析构时确保拉流线程已停止
}

// ============================================================
// EnumCamera：枚举两台虚拟相机（静态函数）
// ============================================================
uint32_t VirtualCamera::EnumCamera(QVector<CameraMetaInfo>& cameraInfos)
{
    cameraInfos.push_back(CameraMetaInfo {
        "VIRTUAL-001", "VirtualCamera-1", VIRTUAL_CAMERA_VENDER });
    cameraInfos.push_back(CameraMetaInfo {
        "VIRTUAL-002", "VirtualCamera-2", VIRTUAL_CAMERA_VENDER });
    return CHONGMING_OK;
}

// ============================================================
// getParamList：获取虚拟相机参数列表
// ============================================================
uint32_t VirtualCamera::getParamList(QVector<CameraParam>& paramList)
{
    ParseUiJson* parser = ParseUiJson::instance();
    parser->loadFromFile(":/VirtualCameraParam.json"); // 从资源文件加载
    QList<CameraParamMetaInfo> paramMetaInfoList = parser->getParamList();

    for (auto var : paramMetaInfoList) {
        paramList.push_back(CameraParam(var));
    }

    return CHONGMING_OK;
}

// ---------- 状态查询 ----------
bool VirtualCamera::isConnect()
{
    return m_connect; // 虚拟相机只查内部标志
}

bool VirtualCamera::isGrabbing()
{
    return m_startGrabbing.load(); // 原子读取（多线程安全）
}

// ---------- 生命周期（虚拟相机不需要实际操作，只改标志） ----------
uint32_t VirtualCamera::acquire()
{
    return CHONGMING_OK; // 虚拟相机不需要初始化
}

uint32_t VirtualCamera::release()
{
    stopGrabbing(); // 释放前确保停止拉流
    return CHONGMING_OK;
}

uint32_t VirtualCamera::connect()
{
    m_connect = true; // 设置连接标志
    return CHONGMING_OK;
}

uint32_t VirtualCamera::disconnect()
{
    stopGrabbing(); // 断开前先停止拉流
    m_connect = false;
    return CHONGMING_OK;
}

// ---------- 拉流资源（虚拟相机不需要） ----------
uint32_t VirtualCamera::creatStream()
{
    return CHONGMING_OK;
}

uint32_t VirtualCamera::destroyStream()
{
    return CHONGMING_OK;
}

// ============================================================
// *流程：
// ============================================================
uint32_t VirtualCamera::startGrabbing()
{
    if (m_startGrabbing.exchange(true)) {
        return CHONGMING_OK;
    }

    auto CreateImage = [this]() -> void {
        const cv::Scalar color = m_cameraIndex == 1
            ? cv::Scalar(70, 150, 230)
            : cv::Scalar(180, 90, 70);
        // 循环生成模拟图像，直到拉流停止
        while (this->isGrabbing()) {
            cv::Mat canvas(cv::Size(1920, 1080), CV_8UC3, color);

            // 放入图像队列（模拟相机出图，生产者）
            this->getImageQueue().Put(canvas);
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
        }
    };

    // 启动拉流线程
    m_grabThread = std::thread(CreateImage);

    return CHONGMING_OK;
}

// ============================================================
// stopGrabbing：停止拉流
// * 流程：
// ============================================================
uint32_t VirtualCamera::stopGrabbing()
{
    m_startGrabbing.store(false);
    if (m_grabThread.joinable()) {
        m_grabThread.join();
    }
    return CHONGMING_OK;
}

// ============================================================
// loadConfig：从 Ini 文件加载参数配置
// ============================================================
uint32_t VirtualCamera::loadConfig(const QString path)
{
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        return CAMERA_CONFIG_LOAD_FAILED;
    }

    QSettings settings(path, QSettings::IniFormat); // 以 Ini 格式打开
    settings.beginGroup("Parameters");
    m_configValues.clear();
    // 遍历分组下所有键，存入缓存
    for (const QString& key : settings.childKeys()) {
        m_configValues.insert(key, settings.value(key).toString());
    }
    settings.endGroup();
    return CHONGMING_OK;
}

// ============================================================
// saveConfig：保存参数配置到 Ini 文件
// ============================================================
uint32_t VirtualCamera::saveConfig(const QString path)
{
    if (path.isEmpty()) {
        return CAMERA_CONFIG_SAVE_FAILED;
    }

    QSettings settings(path, QSettings::IniFormat);
    settings.clear(); // 清空旧内容
    // 写入相机基本信息
    settings.beginGroup("Camera");
    settings.setValue("serial", Serial());
    settings.setValue("resolution", "1920x1080");
    settings.setValue("simulatedFps", 25);
    settings.endGroup();
    // 写入所有参数
    settings.beginGroup("Parameters");
    for (auto it = m_configValues.cbegin(); it != m_configValues.cend(); ++it) {
        settings.setValue(it.key(), it.value());
    }
    settings.endGroup();
    settings.sync(); // 立即写入磁盘
    if (settings.status() != QSettings::NoError) {
        return CAMERA_CONFIG_SAVE_FAILED;
    }
    return CHONGMING_OK;
}

QString VirtualCamera::configFormat()
{
    return "ini"; // 虚拟相机配置文件后缀
}

// ============================================================
// * 流程：
// *   1. 标记参数为有效、可读、可写
// ============================================================
uint32_t VirtualCamera::readParam(CameraParam& param)
{
    // 虚拟相机所有参数都标记为可读可写
    param.setValid(true);
    param.setReadable(true);
    param.setWriteable(true);

    // 根据类型给默认值
    if (param.type() == INT) {
        IntParam varParam = param.GetValue().value<IntParam>();
        varParam.value = 30;
        varParam.increment = 1;
        varParam.min = 0;
        varParam.max = 100;
        param.SetValue(QVariant::fromValue(varParam));
    } else if (param.type() == DOUBLE) {
        DoubleParam varParam = param.GetValue().value<DoubleParam>();
        varParam.value = 55.6;
        varParam.min = 0;
        varParam.max = 1000;
        param.SetValue(QVariant::fromValue(varParam));
    } else if (param.type() == ENUM) {
        EnumParam varParam = param.GetValue().value<EnumParam>();
        varParam.value = "item1";
        varParam.valueInt = 0;
        varParam.availableInt = QVector<int> { 0, 1, 2 };
        varParam.availableValue = QVector<QString> { "item1", "item2", "item3" };
        param.SetValue(QVariant::fromValue(varParam));
    } else if (param.type() == BOOL) {
        BoolParam varParam = param.GetValue().value<BoolParam>();
        varParam.value = false;
        param.SetValue(QVariant::fromValue(varParam));
    } else if (param.type() == CMD) {
        // CMD 命令参数不需要读取
    } else if (param.type() == STRING) {
        StringParam varParam = param.GetValue().value<StringParam>();
        varParam.value = "string_value";
        varParam.nMaxLength = 16;
        param.SetValue(QVariant::fromValue(varParam));
    }

    const auto saved = m_configValues.constFind(param.name());
    if (saved != m_configValues.cend()) {
        if (param.type() == INT) {
            auto value = param.GetValue().value<IntParam>();
            value.value = saved.value().toLongLong();
            param.SetValue(QVariant::fromValue(value));
        } else if (param.type() == DOUBLE) {
            auto value = param.GetValue().value<DoubleParam>();
            value.value = saved.value().toDouble();
            param.SetValue(QVariant::fromValue(value));
        } else if (param.type() == BOOL) {
            auto value = param.GetValue().value<BoolParam>();
            // 支持 "true"/"1" 两种表示
            value.value = saved.value().compare("true", Qt::CaseInsensitive) == 0 || saved.value() == "1";
            param.SetValue(QVariant::fromValue(value));
        } else if (param.type() == STRING) {
            auto value = param.GetValue().value<StringParam>();
            value.value = saved.value();
            param.SetValue(QVariant::fromValue(value));
        } else if (param.type() == ENUM) {
            auto value = param.GetValue().value<EnumParam>();
            value.value = saved.value();
            param.SetValue(QVariant::fromValue(value));
        }
    }
    // 把当前值的显示文本回写到缓存，保持最新
    m_configValues.insert(param.name(), param.displayText());

    return CHONGMING_OK;
}

// ============================================================
// ============================================================
uint32_t VirtualCamera::writeParam(CameraParam& param)
{
    if (!param.isValid() || !param.isWriteable()) {
        return WRITE_PARAM_FAILED;
    }
    // 把参数值的显示文本存入缓存
    m_configValues.insert(param.name(), param.displayText());
    return CHONGMING_OK;
}

// ============================================================
// getImageLast：从图像队列取一帧图像
// ============================================================
uint32_t VirtualCamera::getImageLast(cv::Mat& image)
{
    cv::Mat srcImage;
    auto ret = m_imageQueue.Take(srcImage); // 从队列取一帧（阻塞等待）
    if (ret == GETIAMGE_TIMEOUT) {
        return GETIAMGE_TIMEOUT;
    }
    srcImage.copyTo(image);

    return CHONGMING_OK;
}
