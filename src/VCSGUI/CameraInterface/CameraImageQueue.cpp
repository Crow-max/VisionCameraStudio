#include "CameraImageQueue.h"
#include "CameraError.h"


CameraImageQueue::CameraImageQueue()
{
    m_queueSize = 10;
    m_needStop = false;
    for (int i = 0; i < m_queueSize; i++) {
        freeImageQueue.push(cv::Mat());
    }
}

// 带参构造：可自定义队列大小
CameraImageQueue::CameraImageQueue(int maxSize)
{
    m_queueSize = maxSize;
    m_needStop = false;
    for (int i = 0; i < maxSize; i++) {
        freeImageQueue.push(cv::Mat());
    }
}

// ============================================================
// 流程：
//   1. 加锁保护队列
//   2. 如果已请求停止，直接返回（不再接收新帧）
//   5. 唤醒一个等待中的消费者线程
// ============================================================
uint32_t CameraImageQueue::Put(const cv::Mat& m)
{
    std::unique_lock<std::mutex> locker(m_mutex);  // 加锁，函数结束自动解锁
    if (m_needStop) {
        return CHONGMING_OK;  // 已停止，静默丢弃新帧
    }
    if (freeImageQueue.size() != 0) {
        // 空闲队列有位置：从空闲队列拿一个空 Mat
        cv::Mat temp = freeImageQueue.front();
        freeImageQueue.pop();
        temp = m;
        workImageQueue.push(temp);  // 放入工作队列，等消费者取
    } else {
        // 空闲队列没有位置 = 工作队列已满
        // 覆盖式策略：弹出最旧的一帧（丢弃），放入新帧
        // 这样保证消费者永远能取到最新画面，而不是被旧帧堵住
        workImageQueue.pop();
        workImageQueue.push(m);
    }
    m_condition.notify_one();  // 唤醒一个正在等待的消费者线程
    return CHONGMING_OK;
}

// ============================================================
// 流程：
//   1. 加锁
// ============================================================
uint32_t CameraImageQueue::Take(cv::Mat& m)
{
    std::unique_lock<std::mutex> locker(m_mutex);
    std::chrono::milliseconds dura(TIME_OUT_MS);  // 超时时间 5 秒

    // wait_for 详解：
    //   - 第二个参数：最长等待时间
    auto state = m_condition.wait_for(locker, dura, [this] {
        return m_needStop || NotEmpty();
    });

    // 两种情况返回超时：
    if (state == false || m_needStop) {
        return GETIAMGE_TIMEOUT;
    }

    // 从工作队列取出一帧图像
    m = workImageQueue.front();
    workImageQueue.pop();
    freeImageQueue.push(m);
    return CHONGMING_OK;
}

// ---------- 状态查询（都加锁，线程安全） ----------
bool CameraImageQueue::Empty()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    return workImageQueue.empty();
}

bool CameraImageQueue::Full()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    return workImageQueue.size() == m_queueSize;
}

size_t CameraImageQueue::Size()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    return workImageQueue.size();
}

// ============================================================
// Start：开始接收图像
// ============================================================
void CameraImageQueue::Start()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    // 把工作队列中残留的图像全部弹出，放回空闲队列
    while (!workImageQueue.empty()) {
        workImageQueue.pop();
        freeImageQueue.push(cv::Mat());
    }
    m_needStop = false;
}

// ============================================================
// Stop：停止队列
//   设置停止标志，然后唤醒所有等待中的消费者线程
// ============================================================
void CameraImageQueue::Stop()
{
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        m_needStop = true;  // 设置停止标志
    }
    m_condition.notify_all();  // 唤醒所有等待中的消费者
}

// ---------- 内部谓词（不加锁，由调用者保证已持锁） ----------
bool CameraImageQueue::isFull() const
{
    bool full = workImageQueue.size() >= m_queueSize;
    return full;
}

bool CameraImageQueue::isEmpty() const
{
    bool empty = workImageQueue.empty();
    return empty;
}

bool CameraImageQueue::NotFull() const
{
    bool full = workImageQueue.size() >= m_queueSize;
    return !full;
}

bool CameraImageQueue::NotEmpty() const
{
    bool empty = workImageQueue.empty();
    return !empty;
}
