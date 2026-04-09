#include "ViewWidget.h"
#include "AcquireImageProcess.h"
#include "CameraInterface/CMCameraMetaInfo.h"
#include "CameraInterface/CMCameraParam.h"
#include "CameraInterface/CameraContext.h"
#include "CameraInterface/CameraError.h"
#include "ControlWidget/ControlWidget.h"
#include "ViewWidget/GraphicsView.h"
#include "ui_ViewWidget.h"


ViewWidget::ViewWidget(ControlWidget* controlWidget, QWidget* parent)
    : QWidget(parent)
    , Listener()
    , ui(new Ui::ViewWidget)
    , m_pViewBox(new GraphicsView())
    , m_pControlWidget(controlWidget)
    , m_pImageProcess(new AcquireImageProcess())
{
    ui->setupUi(this);
    ui->ViewBox_widget->layout()->addWidget(m_pViewBox);

    connect(m_pImageProcess, &AcquireImageProcess::sigUpdateImage, m_pViewBox, &GraphicsView::SetImage);

    // 注册事件监听
    ListenerManger::Instance()->registerMessage(MESSAGE::CAMERA_CONNECT
            | MESSAGE::CAMERA_DISCONNECT
            | MESSAGE::CAMERA_ENUMRTION
            | MESSAGE::CAMERA_STARTGRAB
            | MESSAGE::CAMERA_STOPTGRAB,
        this);
}

ViewWidget::~ViewWidget()
{
    stopImageProcess(); // 先停止取图线程
    delete m_pImageProcess;
    delete ui;
}

// ============================================================
// stopImageProcess：安全停止取图线程
// ============================================================
void ViewWidget::stopImageProcess()
{
    if (!m_pImageProcess) {
        return;
    }

    m_pImageProcess->requestStop(); // 请求停止（设置标志+唤醒阻塞）
    if (m_pImageProcess->isRunning()) {
        m_pImageProcess->wait(); // 等待线程退出
    }
}

// ============================================================
// RespondMessage：监听者接口实现
// * - 开始/停止拉流：更新按钮图标
// ============================================================
void ViewWidget::RespondMessage(int message)
{
    // 断开事件：清空图像，重置按钮
    if ((message & MESSAGE::CAMERA_DISCONNECT) == MESSAGE::CAMERA_DISCONNECT) {
        m_pViewBox->Clear();
        setStarGrabbingState(false);
    }
    if ((message & MESSAGE::CAMERA_ENUMRTION) == MESSAGE::CAMERA_ENUMRTION) {
        stopImageProcess();
        m_pViewBox->Clear();
        setStarGrabbingState(false);
    }
    // 开始拉流：更新按钮为停止图标
    if ((message & MESSAGE::CAMERA_STARTGRAB) == MESSAGE::CAMERA_STARTGRAB) {
        setStarGrabbingState(true);
    }
    // 停止拉流：更新按钮为开始图标
    if ((message & MESSAGE::CAMERA_STOPTGRAB) == MESSAGE::CAMERA_STOPTGRAB) {
        setStarGrabbingState(false);
    }
}

// ============================================================
// * 根据当前拉流状态做相反操作：
// ============================================================
void ViewWidget::on_Grabbing_Button_toggled(bool checked)
{
    Q_UNUSED(checked)
    QString serial = m_pControlWidget->GetCurrentCameraInfo().Serial;
    if (serial.isNull())
        return; // 没有选中相机

    bool state { false };
    CameraContext::Instance()->isGrabbing(serial, state);
    if (state == false) // 未拉流，开启拉流
    {
        // 连接图像信号（确保连接存在）
        connect(m_pImageProcess, &AcquireImageProcess::sigUpdateImage, m_pViewBox, &GraphicsView::SetImage);
        // 启动 SDK 取流
        CHECK_RETURN(CameraContext::Instance()->startGrabbing(serial));

        // 启动取图线程：设置相机序列号，启动线程从队列取图
        m_pImageProcess->setSerial(serial);
        m_pImageProcess->start();

        // 通知其他模块开始拉流
        ListenerManger::Instance()->notify(MESSAGE::CAMERA_STARTGRAB);
    } else {
        // 已拉流，停止：先断开图像信号
        disconnect(m_pImageProcess, &AcquireImageProcess::sigUpdateImage, m_pViewBox, &GraphicsView::SetImage);
        // 请求取图线程停止
        m_pImageProcess->requestStop();
        m_pImageProcess->wait();
        // 停止 SDK 取流
        const uint32_t result = CameraContext::Instance()->stopGrabbing(serial);
        CHECK_RETURN(result);

        // 通知其他模块停止拉流
        ListenerManger::Instance()->notify(MESSAGE::CAMERA_STOPTGRAB);
    }
}

void ViewWidget::setStarGrabbingState(bool state)
{
    if (state == true) {
        ui->Grabbing_Button->setStyleSheet("QPushButton{image:url(:/StopGrab.png);}");
    } else {
        ui->Grabbing_Button->setStyleSheet("QPushButton{image:url(:/StratGrab.png);}");
    }
}
