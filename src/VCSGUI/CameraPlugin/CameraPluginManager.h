#ifndef CAMERAPLUGINMANAGER_H
#define CAMERAPLUGINMANAGER_H


#include "CameraPluginInterface.h"  // 插件接口
#include <QHash>
#include <QMutex>
#include <QStringList>
#include <memory>
#include <vector>

class QPluginLoader;

// ============================================================
// 线程安全：所有公共接口都加了互斥锁
// ============================================================
class CameraPluginManager {
public:
    static CameraPluginManager& instance();

    // 加载指定目录下的所有相机插件
    // 返回 true 表示至少加载了一个合法插件
    bool loadPlugins(const QString& directoryPath = QString());

    bool unloadPlugins();

    // 让所有已加载插件枚举相机，汇总去重后返回
    uint32_t enumerateCameras(QVector<CameraMetaInfo>& cameras);

    // 根据相机元信息（厂商名）选择对应插件，创建相机实例
    // 返回 nullptr 表示没有插件支持该厂商
    CameraInterface* createCamera(const CameraMetaInfo& info);

    // 销毁相机实例（交给创建它的插件处理）
    void destroyCamera(CameraInterface* camera);

    // ---------- 查询接口 ----------
    QString pluginDirectory() const;     // 当前插件目录
    QStringList loadedPluginIds() const;  // 已加载插件的 ID 列表
    QStringList diagnostics() const;

private:
    // LoadedPlugin：已加载插件的内部记录
    struct LoadedPlugin {
        std::unique_ptr<QPluginLoader> loader;
        CameraPluginInterface* interface = nullptr;
    };

    // 单例：构造/析构私有化，禁止拷贝和赋值
    CameraPluginManager() = default;
    ~CameraPluginManager();
    CameraPluginManager(const CameraPluginManager&) = delete;
    CameraPluginManager& operator=(const CameraPluginManager&) = delete;

    QString resolvePluginDirectory(const QString& directoryPath) const;

    mutable QMutex m_mutex;
    QString m_pluginDirectory;  // 当前插件目录
    QStringList m_diagnostics;  // 诊断信息列表
    std::vector<LoadedPlugin> m_plugins;  // 已加载插件列表

    // 相机对象 → 创建它的插件 的映射
    // 销毁相机时必须查这个表，找到创建它的插件来销毁
    QHash<CameraInterface*, CameraPluginInterface*> m_cameraOwners;
};

#endif // CAMERAPLUGINMANAGER_H
