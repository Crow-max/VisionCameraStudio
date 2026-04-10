#ifndef ACQUIREIMAGEPROCESS_H
#define ACQUIREIMAGEPROCESS_H


#include <QImage>
#include <QObject>
#include <QString>
#include <QThread>
#include <atomic>

// ============================================================
// AcquireImageProcess：取图线程
// ============================================================
class AcquireImageProcess : public QThread {
Q_OBJECT // Qt 元对象宏（信号需要）
    public : explicit AcquireImageProcess(QObject* parent = nullptr);
    ~AcquireImageProcess(); // 析构时自动停止线程并等待结束

    // 设置要取图的相机序列号（切换相机时调用）
    void setSerial(QString serial);
    // 请求停止线程（设置标志 + 唤醒阻塞的取图调用）
    void requestStop();

signals:
    // 取到图像后发出的信号，参数是 QImage
    void sigUpdateImage(const QImage& iamge);

protected:
    void run() override;

private:
    QString m_serial { }; // 当前取图的相机序列号
    std::atomic_bool m_stopRequested { false };
};

#endif // ACQUIREIMAGEPROCESS_H
