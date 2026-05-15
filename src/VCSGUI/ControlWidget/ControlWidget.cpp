#include "ControlWidget.h"
#include "CameraInterface/CMCameraMetaInfo.h"
#include "CameraInterface/CameraContext.h"
#include "LoadingDialog/LoadingDialog.h"
#include "ui_ControlWidget.h"
#include <QFileDialog>
#include <algorithm>
#include <iostream>


ControlWidget::ControlWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::ControlWidget)
    , Listener()
    , m_lastCameraIndex(-1)
{
    ui->setupUi(this);

    // 用按位或 | 组合多个事件，一次注册
    ListenerManger::Instance()->registerMessage(MESSAGE::CAMERA_CONNECT
            | MESSAGE::CAMERA_DISCONNECT
            | MESSAGE::CAMERA_ENUMRTION
            | MESSAGE::CAMERA_STARTGRAB
            | MESSAGE::CAMERA_STOPTGRAB
            | MESSAGE::CAMERA_CAMERASWICH,
        this);
}

ControlWidget::~ControlWidget()
{
    delete ui;
}

// ============================================================
// * 流程：
// *   1. 显示加载动画（枚举可能耗时几秒）
// *   6. 隐藏加载动画
// ============================================================
void ControlWidget::on_Enumeration_Button_clicked()
{
    LoadingDialog::Loading(); // 显示加载动画

    m_isEnumerating = true;
    ListenerManger::Instance()->notify(MESSAGE::CAMERA_ENUMRTION);

    QVector<CameraMetaInfo> cameras;
    CameraContext::Instance()->EnumerationCamera(cameras);

    // 刷新相机列表 UI
    m_lastCameraIndex = -1;
    ui->Camera_listWidget->clear();
    m_cameraMetaInfos.clear();
    m_cameraMetaInfos = cameras;
    for (CameraMetaInfo cameraInfo : m_cameraMetaInfos) {
        QString name = QString(cameraInfo.UserDefineID.data());
        QString serial = QString(cameraInfo.Serial.data());
        // 列表项显示"用户名(序列号)"
        ui->Camera_listWidget->addItem(name + "(" + serial + ")");
    }

    // 枚举完成，通知其他 Widget 刷新
    m_isEnumerating = false;
    ListenerManger::Instance()->notify(MESSAGE::CAMERA_ENUMRTION);
    LoadingDialog::HideLoading(); // 隐藏加载动画
}

// ============================================================
// ============================================================
void ControlWidget::on_SaveConfig_Button_clicked()
{
    QString serial = GetCurrentCameraInfo().Serial;
    QString format = CameraContext::Instance()->getConfigFormat(serial);
    QString filter = tr("config files(*.%1)").arg(format);

    // 弹出保存文件对话框
    QString filePath = QFileDialog::getSaveFileName(this, "Save Config",
        "", filter);
    if (filePath.isEmpty())
        return; // 用户取消

    CHECK_RETURN(CameraContext::Instance()->saveConfig(serial, filePath));
}

// ============================================================
// ============================================================
void ControlWidget::on_LoadConfig_Button_clicked()
{
    QString serial = GetCurrentCameraInfo().Serial;
    QString format = CameraContext::Instance()->getConfigFormat(serial);
    QString filter = tr("config files(*.%1)").arg(format);

    QString filePath = QFileDialog::getOpenFileName(this, "Load Config",
        "", filter);
    if (filePath.isEmpty())
        return;

    CHECK_RETURN(CameraContext::Instance()->loadConfig(serial, filePath));
    ListenerManger::Instance()->notify(MESSAGE::CAMERA_CONNECT);
}

// ============================================================
// * 当列表选中项变化时：
// * 注意：这保证了同一时刻只有一台相机处于连接状态
// ============================================================
void ControlWidget::on_Camera_listWidget_currentRowChanged(int currentRow)
{
    if (currentRow == -1)
        return; // 没有选中项

    // 先处理旧相机：停止拉流+断开连接
    CameraMetaInfo lastCameraInfo = GetCameraInfo(m_lastCameraIndex);
    CameraContext::Instance()->stopGrabbing(lastCameraInfo.Serial);
    CameraContext::Instance()->disconnect(lastCameraInfo.Serial);
    ListenerManger::Instance()->notify(MESSAGE::CAMERA_DISCONNECT);
    m_lastCameraIndex = currentRow; // 更新为当前索引

    // 处理新相机：检查连接状态，更新按钮图标
    CameraMetaInfo currentCameraInfo = GetCurrentCameraInfo();
    bool connectState;
    CameraContext::Instance()->isConnect(currentCameraInfo.Serial, connectState);
    if (connectState == true) {
        // 已连接：显示断开图标
        ui->Connect_Button->setStyleSheet("QPushButton{image:url(:/DisConnect.png);}");
        ListenerManger::Instance()->notify(MESSAGE::CAMERA_CONNECT);
    } else {
        // 未连接：显示连接图标
        ui->Connect_Button->setStyleSheet("QPushButton{image:url(:/Connect.png);}");
        ListenerManger::Instance()->notify(MESSAGE::CAMERA_DISCONNECT);
    }
}

// ============================================================
// * toggle 按钮：按下=连接，弹起=断开
// * 根据当前相机的连接状态做相反操作：
// *   - 未连接 → 连接，图标改为断开
// ============================================================
void ControlWidget::on_Connect_Button_toggled(bool checked)
{
    Q_UNUSED(checked)

    CameraMetaInfo currentCameraInfo = GetCurrentCameraInfo();
    QString serial = currentCameraInfo.Serial;
    if (serial.isNull()) // 没有选中相机
    {
        ui->Connect_Button->setStyleSheet("QPushButton{image:url(:/Connect.png);}");
        return;
    }

    bool connectState;
    CameraContext::Instance()->isConnect(serial, connectState);
    if (connectState == true) {
        // 已连接 → 断开
        CHECK_RETURN(CameraContext::Instance()->stopGrabbing(serial));
        CHECK_RETURN(CameraContext::Instance()->disconnect(serial));
        ui->Connect_Button->setStyleSheet("QPushButton{image:url(:/Connect.png);}");
        ListenerManger::Instance()->notify(MESSAGE::CAMERA_DISCONNECT);
    } else {
        // 未连接 → 连接
        CHECK_RETURN(CameraContext::Instance()->connect(serial));
        ui->Connect_Button->setStyleSheet("QPushButton{image:url(:/DisConnect.png);}");
        ListenerManger::Instance()->notify(MESSAGE::CAMERA_CONNECT);
    }
}

// ---------- 辅助函数 ----------
CameraMetaInfo ControlWidget::GetCurrentCameraInfo()
{
    int index = ui->Camera_listWidget->currentRow();
    if (index == -1 || index >= m_cameraMetaInfos.size()) {
        return CameraMetaInfo();
    } else {
        return m_cameraMetaInfos.at(index);
    }
}

// 获取指定索引的相机信息
CameraMetaInfo ControlWidget::GetCameraInfo(int index)
{
    if (index == -1 || index >= m_cameraMetaInfos.size()) {
        return CameraMetaInfo();
    } else {
        return m_cameraMetaInfos.at(index);
    }
}

// ============================================================
// RespondMessage：监听者接口实现
// * （用于其他地方触发枚举后刷新连接按钮状态）
// ============================================================
void ControlWidget::RespondMessage(int message)
{
    if ((message & MESSAGE::CAMERA_ENUMRTION) == MESSAGE::CAMERA_ENUMRTION) {
        if (m_isEnumerating) {
            return;
        }
        on_Connect_Button_toggled(true); // 触发连接按钮刷新状态
    }
}
