#include "AcquireImageProcess.h"
#include "CameraInterface/CameraContext.h"
#include "CameraInterface/CameraError.h"


AcquireImageProcess::AcquireImageProcess(QObject* parent)
    : QThread(parent)
{
}

AcquireImageProcess::~AcquireImageProcess()
{
    requestStop(); // 请求停止
    wait();
}

// 设置相机序列号（切换相机时调用）
// 同时重置停止标志，让线程可以重新开始取图
void AcquireImageProcess::setSerial(QString serial)
{
    m_serial = serial;
    m_stopRequested.store(false); // 重置停止标志
}

// ============================================================
// requestStop：请求停止线程
// * 做两件事：
// ============================================================
void AcquireImageProcess::requestStop()
{
    m_stopRequested.store(true); // 设置停止标志
    CameraContext::Instance()->wakeImageWait(m_serial); // 唤醒阻塞的取图
}

// ============================================================
// * 循环：
// *   1. 检查停止标志，已请求停止就退出循环
// ============================================================
void AcquireImageProcess::run
    qDebug() << "[Acquire] 采集线程启动";
    // TODO: 优化线程调度，避免UI阻塞()
{
    while (!m_stopRequested.load()) {
        QImage image;
        auto ret = CameraContext::Instance()->getImageLast(m_serial, image);
        if (ret == GETIAMGE_TIMEOUT)
            continue;

        // 取到图像，发信号给 UI 显示
        // 发信号前再检查一次停止标志，避免停止后还发最后一帧
        if (!m_stopRequested.load()) {
            emit sigUpdateImage(image);
        }
    }
}
