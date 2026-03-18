#include "HikCamera.h"
#include "opencv2/opencv.hpp"
#include <QDebug>


const QString HikCamera::VIRTUAL_CAMERA_VENDER = "Hikrobot";

// ============================================================
// ============================================================
bool IsColor(MvGvspPixelType enType)
{
    switch (enType) {
    case PixelType_Gvsp_BGR8_Packed:
    case PixelType_Gvsp_YUV422_Packed:
    case PixelType_Gvsp_YUV422_YUYV_Packed:
    case PixelType_Gvsp_BayerGR8:
    case PixelType_Gvsp_BayerRG8:
    case PixelType_Gvsp_BayerGB8:
    case PixelType_Gvsp_BayerBG8:
    case PixelType_Gvsp_BayerGB10:
    case PixelType_Gvsp_BayerGB10_Packed:
    case PixelType_Gvsp_BayerBG10:
    case PixelType_Gvsp_BayerBG10_Packed:
    case PixelType_Gvsp_BayerRG10:
    case PixelType_Gvsp_BayerRG10_Packed:
    case PixelType_Gvsp_BayerGR10:
    case PixelType_Gvsp_BayerGR10_Packed:
    case PixelType_Gvsp_BayerGB12:
    case PixelType_Gvsp_BayerGB12_Packed:
    case PixelType_Gvsp_BayerBG12:
    case PixelType_Gvsp_BayerBG12_Packed:
    case PixelType_Gvsp_BayerRG12:
    case PixelType_Gvsp_BayerRG12_Packed:
    case PixelType_Gvsp_BayerGR12:
    case PixelType_Gvsp_BayerGR12_Packed:
    case PixelType_Gvsp_BayerRBGG8:
    case PixelType_Gvsp_BayerGR16:
    case PixelType_Gvsp_BayerRG16:
    case PixelType_Gvsp_BayerGB16:
    case PixelType_Gvsp_BayerBG16:
        return true;
    default:
        return false;
    }
}

// ============================================================
// ============================================================
bool IsMono(MvGvspPixelType enType)
{
    switch (enType) {
    case PixelType_Gvsp_Mono8:
    case PixelType_Gvsp_Mono10:
    case PixelType_Gvsp_Mono10_Packed:
    case PixelType_Gvsp_Mono12:
    case PixelType_Gvsp_Mono12_Packed:
    case PixelType_Gvsp_Mono14:
    case PixelType_Gvsp_Mono16:
        return true;
    default:
        return false;
    }
}

// ============================================================
// *   1. 判断原始像素格式是黑白还是彩色
// ============================================================
bool HikConvert2Mat(void* handle, MV_FRAME_OUT_INFO_EX* pstImageInfo, unsigned char* pData, cv::Mat& dstImage)
{
    if (NULL == pstImageInfo || NULL == pData) {
        printf("NULL info or data.\n");
        return false;
    }
    MvGvspPixelType enDstPixelType = PixelType_Gvsp_Undefined;
    if (IsMono(pstImageInfo->enPixelType)) {
        enDstPixelType = PixelType_Gvsp_Mono8;
    } else if (IsColor(pstImageInfo->enPixelType)) {
        enDstPixelType = PixelType_Gvsp_RGB8_Packed;
    }

    if (enDstPixelType == PixelType_Gvsp_Undefined) {
        qDebug() << "Unsupported pixel format!";
        return false;
    }

    const bool monoImage = IsMono(pstImageInfo->enPixelType);
    if (!monoImage) {
        enDstPixelType = PixelType_Gvsp_BGR8_Packed;
    }
    dstImage.create(pstImageInfo->nHeight, pstImageInfo->nWidth, monoImage ? CV_8UC1 : CV_8UC3);

    // 填充海康像素转换参数结构体
    MV_CC_PIXEL_CONVERT_PARAM convertParam = { 0 };
    convertParam.nWidth = pstImageInfo->nWidth; // 图像宽
    convertParam.nHeight = pstImageInfo->nHeight; // 图像高
    convertParam.pSrcData = pData; // 原始数据指针
    convertParam.nSrcDataLen = pstImageInfo->nFrameLen; // 原始数据长度
    convertParam.enSrcPixelType = pstImageInfo->enPixelType; // 原始像素格式
    convertParam.enDstPixelType = enDstPixelType; // 目标像素格式
    convertParam.pDstBuffer = dstImage.data;
    convertParam.nDstBufferSize = static_cast<unsigned int>(dstImage.total() * dstImage.elemSize()); // 目标缓冲区大小

    // 调用海康 SDK 做像素格式转换
    const auto result = MV_CC_ConvertPixelType(handle, &convertParam);
    if (result != MV_OK) {
        dstImage.release();
        qDebug() << "Convert Pixel Type fail:" << result;
        return false;
    }

    return true;
}

// ============================================================
// * 参数：
// *   pData     - 原始图像数据指针
// ============================================================
void __stdcall ImageCallBack(unsigned char* pData, MV_FRAME_OUT_INFO_EX* pFrameInfo, void* pUser)
{
    if (!pFrameInfo || !pData)
        return;
    HikCamera* pCamera = static_cast<HikCamera*>(pUser);
    if (!pCamera)
        return;

    // 把原始帧数据转换成 cv::Mat
    cv::Mat cvImage;
    if (HikConvert2Mat(pCamera->CameraHandle(), pFrameInfo, pData, cvImage)) {
        // 转换成功，放入图像队列（生产者）
        pCamera->ImageQueue().Put(cvImage);
    }
}

// ---------- 构造/析构 ----------
HikCamera::HikCamera
    qDebug() << "[HikCamera] 相机实例创建";(const CameraMetaInfo& info)
    : CameraInterface(info) // 调用基类构造，保存相机元信息
{
    // 构造时不连接相机，只保存信息
}

HikCamera::~HikCamera()
{
    if (isStartGrabbing) {
        stopGrabbing(); // 正在拉流先停止
    }
    if (m_cameraHandle && MV_CC_IsDeviceConnected(m_cameraHandle)) {
        disconnect(); // 已连接先断开
    }
    release(); // 最后释放句柄
}

// ============================================================
// * 流程：
// *   4. 去重后加入结果列表
// ============================================================
uint32_t HikCamera::EnumCamera(QVector<CameraMetaInfo>& cameraInfos)
{
    MV_CC_DEVICE_INFO_LIST stDeviceList;
    memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
    auto nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &stDeviceList);
    if (MV_OK != nRet) {
        qWarning() << "MV_CC_EnumDevices failed:" << Qt::hex << nRet;
        return NOCAMERA_ERROR;
    }

    qInfo() << "MVS discovered devices:" << stDeviceList.nDeviceNum;
    if (stDeviceList.nDeviceNum == 0) {
        return NOCAMERA_ERROR;
    }

    // 遍历每台枚举到的设备
    for (unsigned int i = 0; i < stDeviceList.nDeviceNum; i++) {
        MV_CC_DEVICE_INFO* cameraInfo = stDeviceList.pDeviceInfo[i];
        if (!cameraInfo) {
            continue;
        }

        CameraMetaInfo info;
        if (cameraInfo->nTLayerType == MV_GIGE_DEVICE) {
            info.Serial = (char*)cameraInfo->SpecialInfo.stGigEInfo.chSerialNumber;
            info.UserDefineID = (char*)cameraInfo->SpecialInfo.stGigEInfo.chUserDefinedName;
            info.VenderName = (char*)cameraInfo->SpecialInfo.stGigEInfo.chManufacturerName;
        } else if (cameraInfo->nTLayerType == MV_USB_DEVICE) {
            info.Serial = (char*)cameraInfo->SpecialInfo.stUsb3VInfo.chSerialNumber;
            info.UserDefineID = (char*)cameraInfo->SpecialInfo.stUsb3VInfo.chUserDefinedName;
            info.VenderName = (char*)cameraInfo->SpecialInfo.stUsb3VInfo.chManufacturerName;
        } else {
            qWarning() << "Unsupported MVS transport layer:" << cameraInfo->nTLayerType;
            continue;
        }

        // 序列号为空的设备跳过（异常设备）
        if (info.Serial.isEmpty()) {
            qWarning() << "MVS device has empty serial number at index" << i;
            continue;
        }

        qInfo() << "MVS camera" << i << info.VenderName
                << info.UserDefineID << info.Serial;
        // 去重：同一台相机不重复加入
        if (!cameraInfos.contains(info)) {
            cameraInfos.push_back(info);
        }
    }

    return cameraInfos.isEmpty() ? NOCAMERA_ERROR : CHONGMING_OK;
}

// ============================================================
// ============================================================
uint32_t HikCamera::getParamList(QVector<CameraParam>& paramList)
{
    static QList<CameraParamMetaInfo> paramMetaInfoList = {
        { "DeviceControl", "DeviceVendorName", STRING, "", QStringLiteral("设备制造商名称") },
        { "DeviceControl", "DeviceUserID", STRING, "", QStringLiteral("设备名称，默认为空，可自行设置") },
        { "DeviceControl", "DeviceSerialNumber", STRING, "", QStringLiteral("设备序列号") },
        { "ImageFormatControl", "WidthMax", INT, "", QStringLiteral("最大宽度") },
        { "ImageFormatControl", "HeightMax", INT, "", QStringLiteral("最大高度") },
        { "ImageFormatControl", "Width", INT, "", QStringLiteral("ROI 区域横向的分辨率") },
        { "ImageFormatControl", "Height", INT, "", QStringLiteral("ROI 区域纵向的分辨率") },
        { "ImageFormatControl", "OffsetX", INT, "", QStringLiteral("ROI 区域左上角起点位置的横坐标") },
        { "ImageFormatControl", "OffsetY", INT, "", QStringLiteral("ROI 区域左上角起点位置的纵坐标") },
        { "ImageFormatControl", "ReverseX", BOOL, "", QStringLiteral("相机图像左右翻转") },
        { "ImageFormatControl", "ReverseY", BOOL, "", QStringLiteral("相机图像上下翻转") },
        { "ImageFormatControl", "PixelFormat", ENUM, "", QStringLiteral("相机支持多种像素格式，用户可根据需要自行设置像素格式") },
        { "AcquisitionControl", "AcquisitionMode", ENUM, "", QStringLiteral("采集模式") },
        { "AcquisitionControl", "AcquisitionStart", CMD, "", QStringLiteral("开始采集") },
        { "AcquisitionControl", "AcquisitionStop", CMD, "", QStringLiteral("停止采集") },
        { "AcquisitionControl", "AcquisitionFrameRateEnable", BOOL, "", QStringLiteral("帧率使能") },
        { "AcquisitionControl", "AcquisitionFrameRate", DOUBLE, "", QStringLiteral("需求帧率") },
        { "AcquisitionControl", "TriggerSelector", ENUM, "", QStringLiteral("触发选项") },
        { "AcquisitionControl", "TriggerMode", ENUM, "", QStringLiteral("触发模式") },
        { "AcquisitionControl", "TriggerSoftware", CMD, "", QStringLiteral("软触发") },
        { "AcquisitionControl", "TriggerSource", ENUM, "", QStringLiteral("触发源设置") },
        { "AcquisitionControl", "ExposureTime", DOUBLE, "", QStringLiteral("曝光设置") },
        { "AnalogControl", "Gain", DOUBLE, "", QStringLiteral("增益设置") },
        { "AnalogControl", "BlackLevel", INT, "", QStringLiteral("黑电平设置") },
        { "AnalogControl", "BlackLevelEnable", BOOL, "", QStringLiteral("黑电平设置使能") },
        { "AnalogControl", "BalanceWhiteAuto", BOOL, "", QStringLiteral("自动白平衡") },
        { "AnalogControl", "Gamma", DOUBLE, "", QStringLiteral("Gamma校正") },
        { "AnalogControl", "GammaSelector", ENUM, "", QStringLiteral("Gamma校正") },
        { "AnalogControl", "GammaEnable", BOOL, "", QStringLiteral("Gamma校正使能") },
        { "AnalogControl", "Sharpness", INT, "", QStringLiteral("锐度设置") },
        { "AnalogControl", "SharpnessEnable", BOOL, "", QStringLiteral("锐度设置使能") },
        { "AnalogControl", "ContrastRatio", DOUBLE, "", QStringLiteral("对比度设置") },
        { "AnalogControl", "ContrastRatioEnable", BOOL, "", QStringLiteral("对比度设置使能") },
        { "UserSetControl ", "UserSetSelector", ENUM, "", QStringLiteral("用户参数组选择") },
        { "UserSetControl ", "UserSetLoad", CMD, "", QStringLiteral("参数组加载") },
        { "UserSetControl ", "UserSetSave", CMD, "", QStringLiteral("参数组保存") },
        { "UserSetControl ", "UserSetDefault", ENUM, "", QStringLiteral("默认用户参数组设置") }
    };

    // 把元信息列表转成 CameraParam 对象列表
    for (auto var : paramMetaInfoList) {
        paramList.push_back(CameraParam(var));
    }

    return CHONGMING_OK;
}

// ---------- 状态查询 ----------
bool HikCamera::isConnect()
{
    if (m_cameraHandle == NULL) {
        return false;
    }
    // 调用海康 SDK 查询设备连接状态
    return MV_CC_IsDeviceConnected(m_cameraHandle);
}

bool HikCamera::isGrabbing()
{
    if (m_cameraHandle == NULL) {
        return false;
    }
    return isStartGrabbing;
}

// ============================================================
// acquire：初始化相机（创建 SDK 句柄）
// * 流程：
// ============================================================
uint32_t HikCamera::acquire()
{
    // 枚举设备，找到和当前相机序列号对应的相机信息
    MV_CC_DEVICE_INFO_LIST stDeviceList;
    memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
    auto nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &stDeviceList);
    if (MV_OK != nRet) {
        return CAMERA_ACQUIRE_FAILED;
    }

    // 遍历枚举结果，按序列号匹配
    m_pDeviceInfo = NULL;
    for (unsigned int i = 0; i < stDeviceList.nDeviceNum; i++) {
        MV_CC_DEVICE_INFO* cameraInfo = stDeviceList.pDeviceInfo[i];
        if (!cameraInfo) {
            continue;
        }

        QString serial;
        if (cameraInfo->nTLayerType == MV_GIGE_DEVICE) {
            serial = QString::fromLocal8Bit(
                reinterpret_cast<const char*>(cameraInfo->SpecialInfo.stGigEInfo.chSerialNumber));
        } else if (cameraInfo->nTLayerType == MV_USB_DEVICE) {
            serial = QString::fromLocal8Bit(
                reinterpret_cast<const char*>(cameraInfo->SpecialInfo.stUsb3VInfo.chSerialNumber));
        }
        if (serial == Serial()) {
            m_pDeviceInfo = cameraInfo; // 找到匹配的设备信息
            break;
        }
    }

    if (m_pDeviceInfo == NULL)
        return CAMERA_ACQUIRE_FAILED; // 没找到对应序列号的相机

    nRet = MV_CC_CreateHandle(&m_cameraHandle, m_pDeviceInfo);
    if (MV_OK != nRet) {
        return INVALID_CAMERA_HANDLE;
    }

    return CHONGMING_OK;
}

// ============================================================
// release：释放相机（销毁 SDK 句柄）
// ============================================================
uint32_t HikCamera::release()
{
    if (m_cameraHandle == NULL) {
        return CHONGMING_OK; // 已经释放了，幂等返回
    }
    // 销毁相机句柄
    MV_CC_DestroyHandle(m_cameraHandle);
    m_cameraHandle = NULL;

    m_pDeviceInfo = NULL;

    return CHONGMING_OK;
}

// ============================================================
// * 流程：
// ============================================================
uint32_t HikCamera::connect()
{
    if (m_cameraHandle == NULL) {
        return INVALID_CAMERA_HANDLE;
    }
    if (MV_CC_IsDeviceAccessible(m_pDeviceInfo, MV_ACCESS_Exclusive) == false) {
        return DEVICE_NOT_ACCESSIBLE;
    }

    // 打开设备
    auto nRet = MV_CC_OpenDevice(m_cameraHandle);
    if (MV_OK != nRet)
        return CONNECT_ERROR;

    nRet = MV_CC_RegisterImageCallBackEx(m_cameraHandle, ImageCallBack, this);
    if (MV_OK != nRet)
        return CONNECT_ERROR;

    return CHONGMING_OK;
}

// ============================================================
// ============================================================
uint32_t HikCamera::disconnect()
{
    if (m_cameraHandle == NULL) {
        return INVALID_CAMERA_HANDLE;
    }

    // 取消回调函数注册（传 NULL 表示取消）
    auto nRet = MV_CC_RegisterImageCallBackEx(m_cameraHandle, NULL, NULL);
    if (MV_OK != nRet)
        return DISCONNECT_ERROR;

    // 关闭设备
    nRet = MV_CC_CloseDevice(m_cameraHandle);
    if (MV_OK != nRet)
        return DISCONNECT_ERROR;

    return CHONGMING_OK;
}

// ============================================================
// ============================================================
uint32_t HikCamera::creatStream()
{
    if (m_cameraHandle == NULL) {
        return INVALID_CAMERA_HANDLE;
    }

    auto nRet = MV_CC_SetGrabStrategy(m_cameraHandle, MV_GRAB_STRATEGY::MV_GrabStrategy_OneByOne);
    if (MV_OK != nRet) {
        qWarning() << "MV_CC_SetGrabStrategy failed:" << Qt::hex << nRet;
        return STARTGRAB_ERROR;
    }
    nRet = MV_CC_SetImageNodeNum(m_cameraHandle, ImageQueueSize);
    if (MV_OK != nRet) {
        qWarning() << "MV_CC_SetImageNodeNum failed:" << Qt::hex << nRet;
        return STARTGRAB_ERROR;
    }

    return CHONGMING_OK;
}

uint32_t HikCamera::destroyStream()
{
    return CHONGMING_OK;
}

// ---------- 拉流控制 ----------
uint32_t HikCamera::startGrabbing()
{
    if (m_cameraHandle == NULL) {
        return INVALID_CAMERA_HANDLE;
    }

    auto nRet = MV_CC_StartGrabbing(m_cameraHandle);
    if (MV_OK != nRet) {
        return STARTGRAB_ERROR;
    }
    isStartGrabbing = true; // 更新内部状态标志

    return CHONGMING_OK;
}

uint32_t HikCamera::stopGrabbing()
{
    if (m_cameraHandle == NULL) {
        return INVALID_CAMERA_HANDLE;
    }

    // 停止拉流
    auto nRet = MV_CC_StopGrabbing(m_cameraHandle);
    if (MV_OK != nRet) {
        return STOPGRAB_ERROR;
    }
    isStartGrabbing = false; // 更新内部状态标志

    return CHONGMING_OK;
}

// ---------- 配置文件导入导出 ----------
uint32_t HikCamera::loadConfig(const QString path)
{
    if (m_cameraHandle == NULL) {
        return INVALID_CAMERA_HANDLE;
    }
    auto nRet = MV_CC_FeatureLoad(m_cameraHandle, path.toLocal8Bit().data());
    if (MV_OK != nRet) {
        return CAMERA_CONFIG_LOAD_FAILED;
    }

    return CHONGMING_OK;
}

uint32_t HikCamera::saveConfig(const QString path)
{
    if (m_cameraHandle == NULL) {
        return INVALID_CAMERA_HANDLE;
    }
    // 把相机当前所有参数保存到文件
    auto nRet = MV_CC_FeatureSave(m_cameraHandle, path.toLocal8Bit().data());
    if (MV_OK != nRet) {
        return CAMERA_CONFIG_SAVE_FAILED;
    }

    return CHONGMING_OK;
}

QString HikCamera::configFormat()
{
    return "mfs"; // 海康配置文件后缀
}

// ============================================================
// readParam：读取相机单个参数的当前值
// *流程：
// ============================================================
uint32_t HikCamera::readParam(CameraParam& param)
{
    // 先获取参数的访问模式（有些参数不可读，如纯写命令）
    getFeatureAccessMode(param);
    if (param.isReadable() == false) {
        return CHONGMING_OK; // 不可读，直接返回（不算错误）
    }

    // 根据参数类型调用不同的 SDK 读取函数
    if (param.type() == INT) {
        QString name = param.name();
        MVCC_INTVALUE value { };
        auto nRet = MV_CC_GetIntValue(m_cameraHandle, name.toLocal8Bit().data(), &value);
        if (nRet != MV_OK)
            return WRITE_PARAM_FAILED;

        // 把 SDK 返回的值填入 IntParam 结构体
        IntParam varParam = param.GetValue().value<IntParam>();
        varParam.value = value.nCurValue; // 当前值
        varParam.increment = value.nInc; // 步进
        varParam.min = value.nMin; // 最小值
        varParam.max = value.nMax; // 最大值
        param.SetValue(QVariant::fromValue(varParam)); // 存回 QVariant
    } else if (param.type() == DOUBLE) {
        QString name = param.name();
        MVCC_FLOATVALUE value { };
        auto nRet = MV_CC_GetFloatValue(m_cameraHandle, name.toLocal8Bit().data(), &value);
        if (nRet != MV_OK)
            return WRITE_PARAM_FAILED;

        DoubleParam varParam = param.GetValue().value<DoubleParam>();
        varParam.value = value.fCurValue;
        varParam.min = value.fMin;
        varParam.max = value.fMax;
        param.SetValue(QVariant::fromValue(varParam));
    } else if (param.type() == ENUM) {
        QString name = param.name();

        // 先获取枚举参数的当前值和支持的条目数
        MVCC_ENUMVALUE value { };
        memset(&value, 0, sizeof(MVCC_ENUMVALUE));
        auto nRet = MV_CC_GetEnumValue(m_cameraHandle, name.toLocal8Bit().data(), &value);
        if (nRet != MV_OK)
            return WRITE_PARAM_FAILED;

        MVCC_ENUMENTRY entryValue { };
        memset(&entryValue, 0, sizeof(MVCC_ENUMENTRY));
        entryValue.nValue = value.nCurValue;
        MV_CC_GetEnumEntrySymbolic(m_cameraHandle, name.toLocal8Bit().data(), &entryValue);

        // 遍历所有支持的枚举值，获取每个值对应的符号名
        QVector<int> intlist { };
        QVector<QString> strlist { };
        for (unsigned int i = 0; i < value.nSupportedNum; i++) {
            int curInt = value.nSupportValue[i];
            intlist.push_back(curInt);

            MVCC_ENUMENTRY entryValue { };
            memset(&entryValue, 0, sizeof(MVCC_ENUMENTRY));
            entryValue.nValue = curInt;
            MV_CC_GetEnumEntrySymbolic(m_cameraHandle, name.toLocal8Bit().data(), &entryValue);
            QString curStr = entryValue.chSymbolic;
            strlist.push_back(curStr);
        }

        EnumParam varParam = param.GetValue().value<EnumParam>();
        varParam.value = entryValue.chSymbolic; // 当前选中的枚举符号名
        varParam.valueInt = value.nCurValue; // 当前值的整数索引
        varParam.availableInt = intlist; // 所有可选值的整数列表
        varParam.availableValue = strlist; // 所有可选值的符号名列表
        param.SetValue(QVariant::fromValue(varParam));
    } else if (param.type() == BOOL) {
        QString name = param.name();
        bool bValue { };
        auto nRet = MV_CC_GetBoolValue(m_cameraHandle, name.toLocal8Bit().data(), &bValue);
        if (nRet != MV_OK)
            return WRITE_PARAM_FAILED;

        BoolParam varParam = param.GetValue().value<BoolParam>();
        varParam.value = bValue;
        param.SetValue(QVariant::fromValue(varParam));
    } else if (param.type() == CMD) {
    } else if (param.type() == STRING) {
        QString name = param.name();
        MVCC_STRINGVALUE value { };
        auto nRet = MV_CC_GetStringValue(m_cameraHandle, name.toLocal8Bit().data(), &value);
        if (nRet != MV_OK)
            return WRITE_PARAM_FAILED;

        StringParam varParam = param.GetValue().value<StringParam>();
        varParam.value = value.chCurValue;
        varParam.nMaxLength = value.nMaxLength;
        param.SetValue(QVariant::fromValue(varParam));
    }

    return CHONGMING_OK;
}

// ============================================================
// writeParam：写入相机单个参数
// ============================================================
uint32_t HikCamera::writeParam(CameraParam& param)
{
    // 先查访问模式，不可写的参数直接返回
    getFeatureAccessMode(param);
    if (param.isWriteable() == false) {
        return CHONGMING_OK;
    }

    if (param.type() == INT) {
        QString name = param.name();
        IntParam varValue = param.GetValue().value<IntParam>();
        auto nRet = MV_CC_SetIntValue(m_cameraHandle, name.toLocal8Bit().data(), varValue.value);
        if (nRet != MV_OK)
            return WRITE_PARAM_FAILED;
    } else if (param.type() == DOUBLE) {
        QString name = param.name();
        DoubleParam varValue = param.GetValue().value<DoubleParam>();
        float value = varValue.value;
        auto nRet = MV_CC_SetFloatValue(m_cameraHandle, name.toLocal8Bit().data(), value);
        if (nRet != MV_OK)
            return WRITE_PARAM_FAILED;
    } else if (param.type() == ENUM) {
        QString name = param.name();
        EnumParam varValue = param.GetValue().value<EnumParam>();
        int value = varValue.valueInt; // 枚举用整数索引设置
        auto nRet = MV_CC_SetEnumValue(m_cameraHandle, name.toLocal8Bit().data(), value);
        if (nRet != MV_OK)
            return WRITE_PARAM_FAILED;
    } else if (param.type() == BOOL) {
        QString name = param.name();
        BoolParam varValue = param.GetValue().value<BoolParam>();
        bool value = varValue.value;
        auto nRet = MV_CC_SetBoolValue(m_cameraHandle, name.toLocal8Bit().data(), value);
        if (nRet != MV_OK)
            return WRITE_PARAM_FAILED;
    } else if (param.type() == CMD) {
        QString name = param.name();
        auto nRet = MV_CC_SetCommandValue(m_cameraHandle, name.toLocal8Bit().data());
        if (nRet != MV_OK)
            return WRITE_PARAM_FAILED;
    } else if (param.type() == STRING) {
        QString name = param.name();
        StringParam varValue = param.GetValue().value<StringParam>();
        QString value = varValue.value;
        auto nRet = MV_CC_SetStringValue(m_cameraHandle, name.toLocal8Bit().data(), value.toLocal8Bit().data());
        if (nRet != MV_OK)
            return WRITE_PARAM_FAILED;
    }

    return CHONGMING_OK;
}

// ============================================================
// getImageLast：从图像队列取一帧图像
// ============================================================
uint32_t HikCamera::getImageLast(cv::Mat& image)
{
    auto ret = m_imageQueue.Take(image);
    if (ret != CHONGMING_OK) {
        return GETIAMGE_TIMEOUT;
    }

    return CHONGMING_OK;
}

// ============================================================
// ============================================================
uint32_t HikCamera::getFeatureAccessMode(CameraParam& param)
{
    QString name = param.name();

    MV_XML_AccessMode mode;
    MV_XML_GetNodeAccessMode(m_cameraHandle, name.toLocal8Bit().data(), &mode);
    if (mode == AM_NI || mode == AM_NA || mode == AM_Undefined) {
        // 不可用：标记为无效、不可读、不可写
        param.setValid(false);
        param.setReadable(false);
        param.setWriteable(false);
    } else if (mode == AM_WO) {
        // 只写：有效、不可读、可写
        param.setValid(true);
        param.setReadable(false);
        param.setWriteable(true);
    } else if (mode == AM_RO) {
        // 只读：有效、可读、不可写
        param.setValid(true);
        param.setReadable(true);
        param.setWriteable(false);
    } else if (mode == AM_RW) {
        // 读写：有效、可读、可写
        param.setValid(true);
        param.setReadable(true);
        param.setWriteable(true);
    }

    return CHONGMING_OK;
}
