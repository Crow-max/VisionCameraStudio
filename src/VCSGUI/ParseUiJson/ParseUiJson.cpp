#include "ParseUiJson.h"
#include <QFile>
#include <QJsonParseError>


// JSON 字段名常量（避免硬编码字符串拼写错误）
const QString kGroup = "group";
const QString kParams = "params";
const QString kName = "name";
const QString kType = "type";
const QString kRelativeList = "relative_list";
const QString kTips = "tips";

// 参数类型字符串常量
const QString kInt = "INT";
const QString kDouble = "DOUBLE";
const QString kEnum = "ENUM";
const QString kBool = "BOOL";
const QString kCmd = "CMD";
const QString kString = "STRING";

// 静态成员初始化
ParseUiJson* ParseUiJson::m_instance = nullptr;
QMutex ParseUiJson::m_mutex;

ParseUiJson::ParseUiJson(QObject* parent)
    : QObject(parent)
    , m_isValid(false)
{
}

ParseUiJson::~ParseUiJson() { }

// ============================================================
// instance：获取单例（双重检查锁定 DCL）
// * 这是单例模式的经典线程安全实现
// ============================================================
ParseUiJson* ParseUiJson::instance()
{
    if (!m_instance) {
        QMutexLocker locker(&m_mutex); // 加锁
        if (!m_instance) {
            m_instance = new ParseUiJson();
        }
    }
    return m_instance;
}

// ============================================================
// loadFromFile：从文件加载 JSON
// ============================================================
bool ParseUiJson::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.exists()) {
        m_lastError = QString("文件不存在: %1").arg(filePath);
        m_isValid = false;
        emit parseFinished(false, m_lastError);
        return false;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = QString("无法打开文件: %1").arg(file.errorString());
        m_isValid = false;
        emit parseFinished(false, m_lastError);
        return false;
    }

    QByteArray jsonData = file.readAll(); // 一次性读入全部内容
    file.close();

    return loadFromByteArray(jsonData); // 转到底层入口
}

// 从字符串加载（转成 UTF-8 字节数组）
bool ParseUiJson::loadFromString(const QString& jsonString)
{
    return loadFromByteArray(jsonString.toUtf8());
}

// ============================================================
// ============================================================
bool ParseUiJson::loadFromByteArray(const QByteArray& jsonData)
{
    clear(); // 清空旧数据

    bool success = parseJson(jsonData);
    emit parseFinished(success, m_lastError);

    return success;
}

// ============================================================
// parseJson：解析 JSON 核心方法
// * 流程：
//  *   2. 检查根节点必须是数组
// ============================================================
bool ParseUiJson::parseJson(const QByteArray& jsonData)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    // JSON 语法错误（如括号不匹配、引号缺失）
    if (parseError.error != QJsonParseError::NoError) {
        m_lastError = QString("JSON解析错误: %1 (位置: %2)")
                          .arg(parseError.errorString())
                          .arg(parseError.offset);
        return false;
    }

    // 根节点必须是数组（每个元素是一个分组）
    if (!doc.isArray()) {
        m_lastError = "JSON根节点必须是数组";
        return false;
    }

    QJsonArray rootArray = doc.array();
    int totalGroups = rootArray.size();

    // 遍历每个分组
    for (int i = 0; i < rootArray.size(); ++i) {
        emit parseProgress(i, totalGroups); // 发解析进度信号

        QJsonValue groupValue = rootArray.at(i);
        if (!groupValue.isObject()) {
            m_lastError = QString("第%1个元素不是对象格式").arg(i + 1);
            return false;
        }

        QJsonObject groupObj = groupValue.toObject();

        // 检查 group 字段（分组名）
        if (!groupObj.contains(kGroup) || !groupObj[kGroup].isString()) {
            m_lastError = QString("第%1个分组缺少group字段或group不是字符串").arg(i + 1);
            return false;
        }

        QString groupName = groupObj[kGroup].toString();

        // 检查 params 字段（必须是数组）
        if (!groupObj.contains(kParams) || !groupObj[kParams].isArray()) {
            m_lastError = QString("分组'%1'缺少params字段或params不是数组").arg(groupName);
            return false;
        }

        QJsonArray paramsArray = groupObj[kParams].toArray();

        // 遍历该分组下的所有参数
        for (int j = 0; j < paramsArray.size(); ++j) {
            QJsonValue paramValue = paramsArray.at(j);
            if (!paramValue.isObject()) {
                m_lastError = QString("分组'%1'的第%2个参数不是对象格式")
                                  .arg(groupName)
                                  .arg(j + 1);
                return false;
            }

            QJsonObject paramObj = paramValue.toObject();

            // 验证参数字段完整性和类型
            if (!validateParamObject(paramObj, groupName, j)) {
                return false;
            }

            // 组装参数元信息对象
            CameraParamMetaInfo paramInfo;
            paramInfo.group = groupName; // 分组名
            paramInfo.name = paramObj[kName].toString(); // 参数名
            paramInfo.type = stringToParamType(paramObj[kType].toString()); // 参数类型（字符串→枚举）
            paramInfo.relative_list = paramObj[kRelativeList].toString(); // 关联参数（可选，没有就是空串）
            paramInfo.tips = paramObj[kTips].toString(); // 提示信息

            m_paramList.append(paramInfo); // 加入结果列表
        }
    }

    m_isValid = true;
    return true;
}

// ============================================================
// ============================================================
bool ParseUiJson::validateParamObject(const QJsonObject& paramObj, const QString& groupName, int index)
{
    // 检查必填字段是否存在
    QStringList requiredFields = { kName, kType, kTips };
    for (const QString& field : requiredFields) {
        if (!paramObj.contains(field)) {
            m_lastError = QString("分组'%1'的第%2个参数缺少'%3'字段")
                              .arg(groupName)
                              .arg(index + 1)
                              .arg(field);
            return false;
        }
    }

    // 检查字段类型
    if (!paramObj[kName].isString()) {
        m_lastError = QString("分组'%1'的第%2个参数的'name'字段必须是字符串")
                          .arg(groupName)
                          .arg(index + 1);
        return false;
    }

    if (!paramObj[kType].isString()) {
        m_lastError = QString("分组'%1'的第%2个参数的'type'字段必须是字符串")
                          .arg(groupName)
                          .arg(index + 1);
        return false;
    }

    if (paramObj.contains(kRelativeList) && !paramObj[kRelativeList].isString()) {
        m_lastError = QString("分组'%1'的第%2个参数的'relative_list'字段必须是字符串")
                          .arg(groupName)
                          .arg(index + 1);
        return false;
    }

    if (!paramObj[kTips].isString()) {
        m_lastError = QString("分组'%1'的第%2个参数的'tips'字段必须是字符串")
                          .arg(groupName)
                          .arg(index + 1);
        return false;
    }

    return true;
}

// ============================================================
// ============================================================
CMParamType ParseUiJson::stringToParamType(const QString& typeStr) const
{
    static QMap<QString, CMParamType> typeMap = {
        { kInt, INT },
        { kDouble, DOUBLE },
        { kEnum, ENUM },
        { kBool, BOOL },
        { kCmd, CMD },
        { kString, STRING }
    };

    return typeMap.value(typeStr, UNKNOWN);
}

// ---------- 查询接口 ----------
QList<CameraParamMetaInfo> ParseUiJson::getParamList() const
{
    return m_paramList;
}

// 按分组筛选参数
QList<CameraParamMetaInfo> ParseUiJson::getParamListByGroup(const QString& group) const
{
    QList<CameraParamMetaInfo> result;
    for (const CameraParamMetaInfo& param : m_paramList) {
        if (param.group == group) {
            result.append(param);
        }
    }
    return result;
}

// 获取所有分组名（去重，按出现顺序）
QStringList ParseUiJson::getAllGroups() const
{
    QStringList groups;
    for (const CameraParamMetaInfo& param : m_paramList) {
        if (!groups.contains(param.group)) {
            groups.append(param.group);
        }
    }
    return groups;
}

// 清空所有解析数据
void ParseUiJson::clear()
{
    m_paramList.clear();
    m_lastError.clear();
    m_isValid = false;
}
