#ifndef CAMERAIMAGEQUEUE_H
#define CAMERAIMAGEQUEUE_H


#include <condition_variable>   // 条件变量：线程间等待/唤醒
#include <iostream>
#include <mutex>                // 互斥锁：保护共享数据
#include <opencv2/opencv.hpp>  // OpenCV 全头（cv::Mat）
#include <queue>                // std::queue 队列容器
#include <thread>               // 线程相关

#define TIME_OUT_MS 5000
#define ImageQueueSize 10

// ============================================================
// 所有公共接口都加了锁，可在多线程环境下安全使用
// ============================================================
class CameraImageQueue {
public:
    CameraImageQueue();
    // 带参构造函数：可自定义队列最大长度
    CameraImageQueue(int maxSize);

    // ---------- 生产者接口 ----------
    // 向队列中放入一帧图像（相机回调线程调用）
    // 策略：队列有空闲位置就直接放；满了就覆盖最旧的一帧
    uint32_t Put(const cv::Mat& m);

    // ---------- 消费者接口 ----------
    // 从队列中取出一帧图像（取图线程调用，阻塞等待）
    uint32_t Take(cv::Mat& m);

    // ---------- 状态查询接口 ----------
    bool Empty();
    bool Full();
    size_t Size();  // 工作队列当前有多少帧待取

    // ---------- 生命周期控制 ----------
    void Start();
    void Stop();

private:
    // 内部状态判断（不加锁，由调用者保证已持锁）
    bool isFull() const;    // 工作队列是否满
    bool isEmpty() const;   // 工作队列是否空
    bool NotFull() const;
    bool NotEmpty() const;

private:
    std::mutex m_mutex;                          // 互斥锁：保护两个队列和停止标志
    std::condition_variable m_condition;
    std::queue<cv::Mat> freeImageQueue;
    std::queue<cv::Mat> workImageQueue;          // 工作队列：已填好图像、等着被取

    uint8_t m_queueSize;   // 队列最大长度（默认10）
    bool m_needStop;
};

#endif // CAMERAIMAGEQUEUE_H
