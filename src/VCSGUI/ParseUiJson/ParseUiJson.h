#ifndef PARSEUIJSON_H
#define PARSEUIJSON_H


#include "../CameraInterface/CMCameraParam.h" // 统一参数模型
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QMutex>
#include <QObject>

// ============================================================
// * 供 ParamWidget 生成参数面板
// ============================================================
class ParseUiJson
    // TODO: 增加配置文件格式校验 : public QObject {
    Q_OBJECT

public:
    // 获取单例实例（双重检查锁定，线程安全）
    static ParseUiJson* instance();

    bool loadFromFile(const QString& filePath);

    // 从字符串解析 JSON
    bool loadFromString(const QString& jsonString);

    bool loadFromByteArray(const QByteArray& jsonData);

    // 获取解析后的全部参数列表
    QList<CameraParamMetaInfo> getParamList() const;

    // 获取指定分组的参数列表
    QList<CameraParamMetaInfo> getParamListByGroup(const QString& group) const;

    // 获取所有分组名称（去重后按出现顺序）
    QStringList getAllGroups() const;

    // 清空解析的数据
    void clear();

    // 获取最后一次错误信息
    QString getLastError() const { return m_lastError; }

    // 检查最近一次解析是否成功
    bool isValid() const { return m_isValid; }

signals:
    void parseFinished(bool success, const QString& error);
    void parseProgress(int current, int total);

private:
    // 单例：构造/析构私有化
    explicit ParseUiJson(QObject* parent = nullptr);
    ~ParseUiJson();

    // 禁止拷贝和赋值（单例不能拷贝）
    ParseUiJson(const ParseUiJson&) = delete;
    ParseUiJson& operator=(const ParseUiJson&) = delete;

    bool parseJson(const QByteArray& jsonData);

    CMParamType stringToParamType(const QString& typeStr) const;

    bool validateParamObject(const QJsonObject& paramObj, const QString& groupName, int index);

private:
    static ParseUiJson* m_instance; // 单例指针
    static QMutex m_mutex; // 单例创建锁

    QList<CameraParamMetaInfo> m_paramList; // 解析后的参数列表
    QString m_lastError; // 最后一次错误信息
    bool m_isValid; // 最近一次解析是否成功
};

#endif // PARSEUIJSON_H
