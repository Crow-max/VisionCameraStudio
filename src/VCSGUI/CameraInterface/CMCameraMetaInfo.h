#ifndef CMCAMERAMETAINFO_H
#define CMCAMERAMETAINFO_H


#include <QString>

// CameraMetaInfo：相机元信息结构体
struct CameraMetaInfo {
    QString Serial {};
    QString UserDefineID {};
    QString VenderName {};

    // 重载 == 运算符：用序列号判断两台相机是否相同
    bool operator==(const CameraMetaInfo& info) const
    {
        if (Serial == info.Serial) {
            return true;
        }
        return false;
    }
};

#endif // CMCAMERAMETAINFO_H
