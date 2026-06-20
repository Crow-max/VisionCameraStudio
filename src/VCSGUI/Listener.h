#ifndef LISTENER_H
#define LISTENER_H


#include <QMap>
#include <QVector>

// ============================================================
// ============================================================
enum MESSAGE {
    CAMERA_ENUMRTION = 0x00000001, // 相机枚举完成事件
    CAMERA_CONNECT = 0x00000002, // 相机连接事件
    CAMERA_DISCONNECT = 0x00000004, // 相机断开连接事件
    CAMERA_STARTGRAB = 0x00000008, // 相机开启拉流事件
    CAMERA_STOPTGRAB = 0x00000010, // 相机停止拉流事件
    CAMERA_CAMERASWICH = 0x00000020
};

// ============================================================
// Listener：监听者抽象基类
// ============================================================
class Listener {
public:
    Listener() { };
    virtual ~Listener() { }; // 虚析构（基类必须有）
    // 纯虚函数：收到事件通知时被调用，参数是事件类型
    virtual void RespondMessage(int message) = 0;
};

// ============================================================
// ============================================================
class ListenerManger {
    // 类型重定义：事件到监听者列表的映射类型
    typedef QMap<int, QVector<Listener*>> mmap;

public:
    static ListenerManger* Instance();
    // 事件到来，通知所有注册了该事件的监听者
    void notify(int message);
    void registerMessage(int message, Listener* listener);

private:
    // 单例：构造/析构私有化
    ListenerManger() { };
    ~ListenerManger() { };
    static ListenerManger* m_pListenerManger; // 单例指针

    QMap<int, QVector<Listener*>> m_messageToLister;
};

#endif // LISTENER_H
