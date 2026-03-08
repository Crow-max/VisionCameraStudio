#ifndef CAMERACONTEXT_H
#define CAMERACONTEXT_H


#include "CameraError.h"
#include <QImage>
#include <QMap>
#include <QString>
#include <QVector>
#include <map>

// ============================================================
// CHECK_RETURN 宏：统一错误检查与处理
// ============================================================
#define CHECK_RETURN(Ret)                    \
    if (Ret != CHONGMING_OK) {               \
        QString error = getErrorInfoEn(Ret); \
        emit SigUpdateErrorInfo(error);      \
        return;                              \
    }

// 前向声明：只声明不包含头文件，减少编译依赖
// 这些类型在 .cpp 中才需要完整定义
struct CameraMetaInfo;
class CameraParam;
class CameraInterface;
class CameraImageQueue;

// ============================================================
// CameraContext：相机环境上下文单例类
// 职责：
//   2. 维护序列号→相机指针的映射表
//   4. 统一错误处理
// ============================================================
class CameraContext {
public:
    // 获取单例指针（懒汉式：第一次调用时才创建实例）
    static CameraContext* Instance();
    // 释放单例（程序退出时必须手动调用，否则内存泄漏）
    static void Release();

    // ---------- [1] 相机枚举与创建 ----------
    // 同时为每个枚举到的相机创建实例，存入映射表
    uint32_t EnumerationCamera(QVector<CameraMetaInfo>& cameraInfos);

    // ---------- [2] 参数接口 ----------
    uint32_t getParamList(const QString serial, QVector<CameraParam>& paramList);

    // ---------- [3] 状态查询 ----------
    uint32_t isConnect(const QString serial, bool& state);    // 查询是否已连接
    uint32_t isGrabbing(const QString serial, bool& state);   // 查询是否正在拉流

    // ---------- [4] 连接控制 ----------
    uint32_t connect(const QString serial);
    uint32_t disconnect(const QString serial);

    // ---------- [5] 拉流控制 ----------
    uint32_t startGrabbing(const QString serial);
    uint32_t stopGrabbing(const QString serial);

    // ---------- [6] 配置文件 ----------
    uint32_t loadConfig(const QString serial, const QString path);  // 导入配置
    uint32_t saveConfig(const QString serial, const QString path);  // 导出配置
    QString getConfigFormat(const QString serial);                    // 获取配置文件格式后缀

    // ---------- [7] 参数读写 ----------
    uint32_t readParam(const QString serial, CameraParam& param);   // 读取单个参数
    uint32_t writeParam(const QString serial, CameraParam& param);  // 写入单个参数

    // ---------- [8] 图像获取 ----------
    uint32_t getImageLast(const QString serial, QImage& qImage);
    void wakeImageWait(const QString serial);

private:
    CameraContext();
    ~CameraContext();
    void clearCameras();

private:
    static CameraContext* m_pContext;  // 单例指针（静态成员，类外定义）

    QMap<QString, CameraInterface*> m_serialCamMap;
};

#endif // CAMERACONTEXT_H
