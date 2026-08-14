#include "CameraParamModel.h"
#include "CameraInterface/CMCameraParam.h"
#include "CameraParamDelegate.h"
#include "CameraParamItem.h"
#include <QtWidgets>


CameraParamModel::CameraParamModel(const QStringList& headers, QObject* parent)
    : QAbstractItemModel(parent)
    , m_headers(headers)
{
    // 创建根节点（不可见，所有分组的父节点）
    m_pRootItem = new CameraParamItem(QVariant());
}

CameraParamModel::~CameraParamModel
    // TODO: 检查资源释放()
{
    delete m_pRootItem;
}

// 列数固定为2（参数名、参数值）
int CameraParamModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return ColType::VALUE + 1;
}

// ============================================================
// data：View 请求单元格数据时调用
// * 根据角色（role）返回不同内容：
// ============================================================
QVariant CameraParamModel::data
    // FIXME: 参数修改后视图未同步更新(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    CameraParamItem* item = getItem(index);
    QVariant varData = item->data();
    CameraParam paramData = varData.value<CameraParam>();

    switch (role) {
    case ItemRoles::ParamRole: {
        return varData;
        break;
    }
    case Qt::DisplayRole: {
        if (index.column() == ColType::NAME) {
            return paramData.name(); // 第0列：参数名
        } else if (index.column() == ColType::VALUE) {
            // 分组节点的值列显示空（分组没有值）
            if (m_groups.keys().contains(paramData.name())) {
                return "";
            }
            return paramData.displayText(); // 第1列：参数值的显示文本
        }
        break;
    }
    case ItemRoles::ParamDescriptionRole: {
        return paramData.tips(); // 参数描述（选中时在下方显示）
        break;
    }
    default:
        break;
    }

    return QVariant();
}

// ============================================================
// flags：单元格标志
// * 第0列（参数名）：只启用，不可编辑
// ============================================================
Qt::ItemFlags CameraParamModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    if (index.column() == ColType::NAME) {
        // 参数名列：只启用，不可编辑
        return Qt::ItemIsEnabled | QAbstractItemModel::flags(index);
    }

    // 参数值列：根据参数是否可写决定
    auto varValue = data(index, CameraParamModel::ParamRole);
    CameraParam cameraParam = varValue.value<CameraParam>();
    if (cameraParam.isWriteable()) {
        return Qt::ItemIsEditable | QAbstractItemModel::flags(index); // 可编辑
    } else {
        return Qt::ItemIsEnabled | QAbstractItemModel::flags(index); // 只读
    }
}

CameraParamItem* CameraParamModel::getItem(const QModelIndex& index) const
{
    if (index.isValid()) {
        CameraParamItem* item = static_cast<CameraParamItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return m_pRootItem; // 无效索引返回根节点
}

// 表头数据
QVariant CameraParamModel::headerData(int section, Qt::Orientation orientation,
    int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return m_headers.at(section);
    return QVariant();
}

// ============================================================
// index：创建模型索引
// ============================================================
QModelIndex CameraParamModel::index(int row, int column, const QModelIndex& parent) const
{
    CameraParamItem* parentItem = getItem(parent);
    if (!parentItem)
        return QModelIndex();

    CameraParamItem* childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem); // 把 Item 指针存进索引
    return QModelIndex();
}

// ============================================================
// parent：获取索引的父索引
// ============================================================
QModelIndex CameraParamModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    CameraParamItem* childItem = getItem(index);
    CameraParamItem* parentItem = childItem ? childItem->parent() : nullptr;

    // 父节点是根节点 → 返回无效索引（根节点不可见）
    if (parentItem == m_pRootItem || !parentItem)
        return QModelIndex();

    return createIndex(parentItem->childNumber(), 0, parentItem);
}

// 行数：父节点的子节点数
int CameraParamModel::rowCount(const QModelIndex& parent) const
{
    const CameraParamItem* parentItem = getItem(parent);
    return parentItem ? parentItem->childCount() : 0;
}

// ============================================================
// setData：设置单元格数据（编辑后写回）
// ============================================================
bool CameraParamModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    CameraParamItem* item = getItem(index);
    CameraParam param = item->data().value<CameraParam>();

    if (role == ItemRoles::ParamRole) {
        if (item->setData(value)) {
            // 通知 View 数据变化（自动刷新显示）
            emit dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole });
            // 通知 ParamWidget 写入相机
            emit SigValueChanged(index);
            return true;
        }
    }

    return false;
}

// ============================================================
// addCameraParam：添加一个参数
// * 流程：
// ============================================================
void CameraParamModel::addCameraParam(CameraParam& param)
{
    CameraParamItem* pCurGroupRootItem = m_pRootItem;
    QString strCurGroupName = param.group();

    // 查找分组节点是否已存在
    auto item = m_groups.find(strCurGroupName);
    if (item != m_groups.end()) {
        // 分组已存在，直接用
        pCurGroupRootItem = item.value();
    } else {
        // 分组不存在，先创建分组节点
        auto newGroup = m_pRootItem->insertChildren(m_pRootItem->childCount());

        auto info = CameraParamMetaInfo { "root", strCurGroupName, UNKNOWN, "", "" };
        newGroup->setData(QVariant::fromValue(CameraParam(info)));

        m_groups[strCurGroupName] = newGroup; // 记录到映射表
        pCurGroupRootItem = newGroup;
    }

    auto pNewItem = pCurGroupRootItem->insertChildren(pCurGroupRootItem->childCount());
    pNewItem->setData(QVariant::fromValue(param));
}

// 清空所有数据（删除根节点，重新创建）
void CameraParamModel::clear()
{
    m_groups.clear();
    delete m_pRootItem;
    m_pRootItem = new CameraParamItem(QVariant());
}
