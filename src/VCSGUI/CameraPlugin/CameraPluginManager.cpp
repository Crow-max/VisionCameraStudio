#include "CameraPluginManager.h"
#include "../CameraInterface/CameraError.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLibrary>
#include <QMutexLocker>
#include <QPluginLoader>
#include <QSet>


namespace {
// 升级接口时改这个数字，旧版插件会被自动拒绝
constexpr int kSupportedInterfaceVersion = 1;
}

// ============================================================
// 第一次调用时创建，之后都返回同一个引用
// ============================================================
CameraPluginManager& CameraPluginManager::instance()
{
    static CameraPluginManager manager;
    return manager;
}

// 析构函数：卸载所有插件
CameraPluginManager::~CameraPluginManager()
{
    unloadPlugins();
}

// ============================================================
// ============================================================
QString CameraPluginManager::resolvePluginDirectory(const QString& directoryPath) const
{
    // ① 显式传入的路径（去掉首尾空格后非空）
    if (!directoryPath.trimmed().isEmpty()) {
        return QDir(directoryPath).absolutePath();
    }

    const QString environmentPath = qEnvironmentVariable("VCS_CAMERA_PLUGIN_PATH");
    if (!environmentPath.trimmed().isEmpty()) {
        return QDir(environmentPath).absolutePath();
    }

    return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("plugins/cameras");
}

// ============================================================
// 流程：
//   1. 加锁
//   2. 解析插件目录
//   3. 如果已经加载过且目录相同，直接返回（幂等）
//   5. 清空旧插件和诊断信息
//   6. 扫描目录下所有文件，过滤出动态库
//   8. 每步失败都记录诊断信息
//   9. 至少加载一个才返回 true
// ============================================================
bool CameraPluginManager::loadPlugins
    // TODO: 调试插件加载失败问题(const QString& directoryPath)
{
    QMutexLocker locker(&m_mutex);  // 加锁，函数结束自动解锁
    const QString resolvedPath = resolvePluginDirectory(directoryPath);

    // 幂等检查：已经加载过且目录相同，不用重复加载
    if (!m_plugins.empty() && resolvedPath == m_pluginDirectory) {
        return true;
    }
    if (!m_cameraOwners.isEmpty()) {
        m_diagnostics.append("Cannot reload camera plugins while camera instances are active.");
        return false;
    }

    // 清空旧状态
    m_plugins.clear();
    m_diagnostics.clear();
    m_pluginDirectory = resolvedPath;

    // 检查目录是否存在
    QDir directory(resolvedPath);
    if (!directory.exists()) {
        m_diagnostics.append(QString("Camera plugin directory does not exist: %1").arg(resolvedPath));
        return false;
    }

    QSet<QString> pluginIds;  // 用于插件 ID 去重
    // 按文件名排序，保证加载顺序确定（方便调试）
    const QFileInfoList files = directory.entryInfoList(QDir::Files, QDir::Name);
    for (const QFileInfo& file : files) {
        if (!QLibrary::isLibrary(file.absoluteFilePath())) {
            continue;
        }

        // 用 QPluginLoader 加载 dll
        auto loader = std::make_unique<QPluginLoader>(file.absoluteFilePath());
        QObject* pluginObject = loader->instance();
        if (!pluginObject) {
            m_diagnostics.append(QString("Failed to load %1: %2")
                                     .arg(file.fileName(), loader->errorString()));
            qWarning().noquote() << m_diagnostics.back();
            continue;
        }

        auto* cameraPlugin = qobject_cast<CameraPluginInterface*>(pluginObject);
        if (!cameraPlugin) {
            m_diagnostics.append(QString("Ignored %1: incompatible camera plugin interface.")
                                     .arg(file.fileName()));
            qWarning().noquote() << m_diagnostics.back();
            loader->unload();  // 卸载，释放资源
            continue;
        }
        if (cameraPlugin->interfaceVersion() != kSupportedInterfaceVersion) {
            m_diagnostics.append(QString("Ignored %1: interface version %2 is not supported.")
                                     .arg(file.fileName())
                                     .arg(cameraPlugin->interfaceVersion()));
            qWarning().noquote() << m_diagnostics.back();
            loader->unload();
            continue;
        }
        // 去重：插件 ID 重复（同一个插件被复制了多份）
        if (pluginIds.contains(cameraPlugin->pluginId())) {
            m_diagnostics.append(QString("Ignored %1: duplicate plugin id %2.")
                                     .arg(file.fileName(), cameraPlugin->pluginId()));
            qWarning().noquote() << m_diagnostics.back();
            loader->unload();
            continue;
        }

        // 所有检查通过：记录插件 ID，保存到列表
        pluginIds.insert(cameraPlugin->pluginId());
        m_plugins.push_back({ std::move(loader), cameraPlugin });
    }

    // 一个合法插件都没加载到
    if (m_plugins.empty()) {
        m_diagnostics.append(QString("No usable camera plugins found in %1").arg(resolvedPath));
        return false;
    }
    return true;
}

// ============================================================
// unloadPlugins：卸载所有插件
// 有相机实例在用时拒绝卸载
// ============================================================
bool CameraPluginManager::unloadPlugins()
{
    QMutexLocker locker(&m_mutex);
    if (!m_cameraOwners.isEmpty()) {
        m_diagnostics.append("Cannot unload camera plugins while camera instances are active.");
        return false;
    }

    bool success = true;
    // 逆序遍历（rbegin→rend），后加载的先卸载
    for (auto it = m_plugins.rbegin(); it != m_plugins.rend(); ++it) {
        if (it->loader && it->loader->isLoaded() && !it->loader->unload()) {
            // 卸载失败（可能有对象还在引用）
            m_diagnostics.append(QString("Failed to unload %1: %2")
                                     .arg(it->loader->fileName(), it->loader->errorString()));
            success = false;
        }
    }
    m_plugins.clear();
    return success;
}

// ============================================================
// 去重规则：厂商名+序列号 相同的视为同一台相机
// ============================================================
uint32_t CameraPluginManager::enumerateCameras(QVector<CameraMetaInfo>& cameras)
{
    QMutexLocker locker(&m_mutex);
    if (m_plugins.empty()) {
        return NOCAMERA_ERROR;  // 没有加载任何插件
    }

    uint32_t firstError = CHONGMING_OK;
    QSet<QString> knownCameras;
    for (const CameraMetaInfo& camera : cameras) {
        knownCameras.insert(camera.VenderName + QLatin1Char(':') + camera.Serial);
    }

    // 遍历每个插件，让它枚举自己支持的相机
    for (const LoadedPlugin& plugin : m_plugins) {
        QVector<CameraMetaInfo> pluginCameras;
        const uint32_t result = plugin.interface->enumerateCameras(pluginCameras);
        // 记录第一个非 OK 的错误（后续插件的错误不覆盖）
        if (result != CHONGMING_OK && firstError == CHONGMING_OK) {
            firstError = result;
        }
        // 汇总并去重
        for (const CameraMetaInfo& camera : pluginCameras) {
            const QString key = camera.VenderName + QLatin1Char(':') + camera.Serial;
            if (!knownCameras.contains(key)) {
                cameras.push_back(camera);
                knownCameras.insert(key);
            }
        }
    }
    return firstError;
}

// ============================================================
// ============================================================
CameraInterface* CameraPluginManager::createCamera(const CameraMetaInfo& info)
{
    QMutexLocker locker(&m_mutex);
    for (const LoadedPlugin& plugin : m_plugins) {
        // 找到支持该厂商的插件（不区分大小写）
        if (!plugin.interface->supportedVendors().contains(info.VenderName, Qt::CaseInsensitive)) {
            continue;
        }
        // 调用插件创建相机实例
        CameraInterface* camera = plugin.interface->createCamera(info);
        if (camera) {
            // 记录所有者：这个相机是哪个插件创建的
            // 销毁时必须查这个表，交给创建它的插件销毁
            m_cameraOwners.insert(camera, plugin.interface);
        }
        return camera;
    }
    return nullptr;  // 没有插件支持该厂商
}

// ============================================================
// destroyCamera：销毁相机实例
//   跨 dll delete 可能导致堆损坏）
// ============================================================
void CameraPluginManager::destroyCamera(CameraInterface* camera)
{
    if (!camera) {
        return;
    }

    QMutexLocker locker(&m_mutex);
    // 从所有者映射表中取出创建它的插件指针
    CameraPluginInterface* owner = m_cameraOwners.take(camera);
    if (owner) {
        owner->destroyCamera(camera);  // 交给插件销毁
    }
}

// ---------- 查询接口（都加锁，线程安全） ----------
QString CameraPluginManager::pluginDirectory() const
{
    QMutexLocker locker(&m_mutex);
    return m_pluginDirectory;
}

QStringList CameraPluginManager::loadedPluginIds() const
{
    QMutexLocker locker(&m_mutex);
    QStringList ids;
    for (const LoadedPlugin& plugin : m_plugins) {
        ids.append(plugin.interface->pluginId());
    }
    return ids;
}

QStringList CameraPluginManager::diagnostics() const
{
    QMutexLocker locker(&m_mutex);
    return m_diagnostics;
}
