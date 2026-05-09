#ifndef VIRTUALCAMERAPLUGIN_H
#define VIRTUALCAMERAPLUGIN_H


#include "../../CameraPlugin/CameraPluginInterface.h"
#include <QObject>

// ============================================================
// ============================================================
class VirtualCameraPlugin final : public QObject, public CameraPluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID CameraPluginInterface_iid FILE "VirtualCameraPlugin.json")
    Q_INTERFACES(CameraPluginInterface)

public:
    QString pluginId() const override;
    int interfaceVersion() const override;
    QStringList supportedVendors() const override;
    uint32_t enumerateCameras(QVector<CameraMetaInfo>& cameras) override;
    CameraInterface* createCamera(const CameraMetaInfo& info) override;
    void destroyCamera(CameraInterface* camera) override;
};

#endif // VIRTUALCAMERAPLUGIN_H
