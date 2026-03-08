#ifndef CAMERAPARAM_H
#define CAMERAPARAM_H


#include <QString>
#include <QVariant>
#include <QVector>

// ============================================================
// CMParamType：统一参数类型枚举
// 所有品牌相机的参数都被抽象成这六种类型之一
// ============================================================
enum CMParamType {
    UNKNOWN = 0,
    INT, // 整数类型（曝光、增益等）
    DOUBLE, // 浮点数类型（帧率等）
    ENUM,
    BOOL, // 布尔类型（自动曝光开关等）
    CMD,
    STRING // 字符串类型（用户自定义名称等）
};

// ============================================================
// ============================================================
struct CameraParamMetaInfo {
    QString group;
    QString name;
    CMParamType type { UNKNOWN }; // 参数类型（六种之一）
    QString relative_list;
    QString tips;
};

// ============================================================
// CMParam：六种具体参数类型的基类（抽象基类）
// 只有一个纯虚 clone() 函数，用于多态拷贝
// ============================================================
struct CMParam {
    virtual ~CMParam() = default; // 虚析构（基类必须有）
    virtual CMParam* clone() = 0;
};

// ---------- 六种具体参数类型 ----------

// IntParam：整数参数
struct IntParam : public CMParam {
    int64_t value { }; // 当前值
    int64_t min { }; // 最小值
    int64_t max { }; // 最大值
    int64_t increment { }; // 步进值（每次增加/减少的量）

    CMParam* clone() override { return new IntParam(*this); }
};

// DoubleParam：浮点数参数
struct DoubleParam : public CMParam {
    double value { }; // 当前值
    double min { }; // 最小值
    double max { }; // 最大值
    double increment { }; // 步进值（每次增加/减少的量）

    CMParam* clone() override { return new DoubleParam(*this); }
};

// BoolParam：布尔参数
struct BoolParam : public CMParam {
    bool value { }; // 当前值（true/false）

    CMParam* clone() override { return new BoolParam(*this); }
};

// StringParam：字符串参数
struct StringParam : public CMParam {
    QString value { }; // 当前值
    unsigned int nMaxLength { }; // 最大长度

    CMParam* clone() override { return new StringParam(*this); }
};

// EnumParam：枚举参数
struct EnumParam : public CMParam {
    QString value { }; // 当前选中的枚举值（字符串形式）
    QVector<QString> availableValue; // 可选的枚举值列表（字符串形式）
    int valueInt { }; // 当前值的整数索引
    QVector<int> availableInt; // 可选值的整数索引列表

    CMParam* clone() override { return new EnumParam(*this); }
};

struct CmdParam : public CMParam {
    CMParam* clone() override { return new CmdParam(*this); }
};

// ============================================================
// 这是"类型擦除"模式：一个类可以装六种不同类型
// ============================================================
class CameraParam {
public:
    CameraParam() { } // 默认构造

    // 带元信息的构造
    CameraParam(CameraParamMetaInfo meta)
        : _meta(meta)
    {
    }

    // 拷贝构造
    CameraParam(const CameraParam& param)
    {
        _meta = param._meta;
        _value = param._value;
        _accessMode = param._accessMode;
    }

    // ---------- 值的读写 ----------
    QVariant GetValue() const { return _value; }
    void SetValue(const QVariant& value) { _value = value; } // 设置值

    // 重置元信息（切换参数时用）
    void reset(CameraParamMetaInfo meta) { _meta = meta; }

    // ---------- 元信息访问 ----------
    const QString& name() const { return _meta.name; } // 参数名
    const QString& group() const { return _meta.group; } // 分组名
    const QString& relativeList() const { return _meta.relative_list; } // 关联参数
    CMParamType type() const { return _meta.type; } // 参数类型
    const QString& tips() const { return _meta.tips; } // 提示信息

    // ============================================================
    // displayText：把参数值转成显示用的字符串
    // 这是 UI 显示参数值的统一入口
    // ============================================================
    QString displayText() const
    {
        switch (type()) {
        case STRING: {
            StringParam varParam = GetValue().value<StringParam>();
            return varParam.value;
        }
        case CMD: {
            return "{Commond}"; // 命令类型不显示值，显示占位符
        }
        case INT: {
            IntParam varParam = GetValue().value<IntParam>();
            return QString::number(varParam.value);
        }
        case DOUBLE: {
            DoubleParam varParam = GetValue().value<DoubleParam>();
            return QString::number(varParam.value);
        }
        case BOOL: {
            BoolParam varParam = GetValue().value<BoolParam>();
            return varParam.value ? "True" : "False";
        }
        case ENUM: {
            EnumParam varParam = GetValue().value<EnumParam>();
            return varParam.value;
        }
        default:
            break;
        }
        return QString("unknow");
    }

    // ---------- 访问权限 ----------
    bool isValid() { return _accessMode.valid; } // 参数是否有效
    bool isReadable() { return _accessMode.readable; } // 是否可读
    bool isWriteable() { return _accessMode.writeable; } // 是否可写

    void setValid(bool valid) { _accessMode.valid = valid; }
    void setReadable(bool readable) { _accessMode.readable = readable; }
    void setWriteable(bool writeable) { _accessMode.writeable = writeable; }

private:
    CameraParamMetaInfo _meta;
    QVariant _value;

    struct
    {
        bool valid : 1; // 参数是否有效
        bool readable : 1; // 是否可读
        bool writeable : 1; // 是否可写
    } _accessMode { };
};

// ============================================================
// ============================================================
Q_DECLARE_METATYPE(IntParam);
Q_DECLARE_METATYPE(DoubleParam);
Q_DECLARE_METATYPE(BoolParam);
Q_DECLARE_METATYPE(StringParam);
Q_DECLARE_METATYPE(EnumParam);
Q_DECLARE_METATYPE(CmdParam);
Q_DECLARE_METATYPE(CameraParam);

#endif // CAMERAPARAM_H
