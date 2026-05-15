#ifndef CONTROLWIDGET_H
#define CONTROLWIDGET_H


#include "Listener.h" // 监听者基类
#include <QWidget>

namespace Ui {
class ControlWidget;
}

struct CameraMetaInfo; // 前向声明

// ============================================================
// ControlWidget：控制面板
// ============================================================
class ControlWidget : public QWidget, Listener {
    Q_OBJECT

public:
    explicit ControlWidget(QWidget* parent = nullptr);
    ~ControlWidget();

    // 监听者接口实现：收到事件通知时被调用
    void RespondMessage(int message) override;

    // ---------- 辅助函数 ----------
    // 获取当前列表中选中的相机信息（未选中返回空对象）
    CameraMetaInfo GetCurrentCameraInfo();
    // 获取指定索引的相机信息（索引无效返回空对象）
    CameraMetaInfo GetCameraInfo(int index);

signals:
    void SigUpdateErrorInfo(QString info);

private slots:
    // ---------- 按钮槽函数（Qt 自动连接，命名规则 on_<objectName>_<signal>） ----------
    void on_Enumeration_Button_clicked();
    void on_SaveConfig_Button_clicked();
    void on_LoadConfig_Button_clicked();
    void on_Camera_listWidget_currentRowChanged(int currentRow); // 相机列表选中项变化（切换相机）
    void on_Connect_Button_toggled(bool checked);

private:
    Ui::ControlWidget* ui; // .ui 界面对象
    int m_lastCameraIndex;
    bool m_isEnumerating = false;

    QVector<CameraMetaInfo> m_cameraMetaInfos; // 枚举到的相机信息列表
};

#endif // CONTROLWIDGET_H
