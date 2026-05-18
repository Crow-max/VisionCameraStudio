#ifndef CAMERAPARAMMODEL_H
#define CAMERAPARAMMODEL_H


#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QVector>

class CameraParamItem; // 前向声明
class CameraParam; // 前向声明

// ============================================================
// CameraParamModel：参数数据模型
// ============================================================
class CameraParamModel : public QAbstractItemModel {
    Q_OBJECT
public:
    // 列类型：第0列参数名，第1列参数值
    enum ColType {
        NAME = 0,
        VALUE
    };

    enum ItemRoles {
        ParamRole = Qt::UserRole + 1,
        ParamDescriptionRole = Qt::UserRole + 2, // 存储参数描述（tips）
    };

    CameraParamModel(const QStringList& headers, QObject* parent = nullptr);
    ~CameraParamModel();

    // ---------- 自定义接口 ----------
    void addCameraParam(CameraParam& param); // 添加一个参数（自动按分组组织）
    void clear(); // 清空所有数据

    // ---------- 只读模型接口（QAbstractItemModel 纯虚函数，必须实现） ----------
    QVariant data(const QModelIndex& index, int role) const override; // 获取单元格数据
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override; // 获取表头数据

    QModelIndex index(int row, int column,
        const QModelIndex& parent = QModelIndex()) const override; // 创建索引
    QModelIndex parent(const QModelIndex& index) const override; // 获取父索引

    int rowCount(const QModelIndex& parent = QModelIndex()) const override; // 行数
    int columnCount(const QModelIndex& parent = QModelIndex()) const override; // 列数

    // ---------- 可编辑模型接口 ----------
    Qt::ItemFlags flags(const QModelIndex& index) const override; // 单元格标志（是否可编辑）
    bool setData(const QModelIndex& index, const QVariant& value,
        int role = Qt::EditRole) override; // 设置单元格数据（编辑后写回）

signals:
    void SigValueChanged(const QModelIndex& index);

protected:
    CameraParamItem* getItem(const QModelIndex& index) const;

private:
    CameraParamItem* m_pRootItem;
    QStringList m_headers;
    QMap<QString, CameraParamItem*> m_groups;
};

#endif // CAMERAPARAMMODEL_H
