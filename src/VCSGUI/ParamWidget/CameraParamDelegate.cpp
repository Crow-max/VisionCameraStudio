#include "CameraParamDelegate.h"
#include "CameraParamModel.h"
#include "CustomWidget/BoolCustomWidget.h"
#include "CustomWidget/CmdCustomWidget.h"
#include "CustomWidget/DoubleCustomWidget.h"
#include "CustomWidget/EnumCustomWidget.h"
#include "CustomWidget/IntCustomWidget.h"
#include "CustomWidget/StringCustomWidget.h"
#include "OneCustomWidget.h"


CameraParamDelegate::CameraParamDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

CameraParamDelegate::~CameraParamDelegate()
{
}

// ============================================================
// createEditor：创建编辑控件
// ============================================================
QWidget* CameraParamDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option)

    // lambda：根据参数类型创建对应的编辑控件
    auto createWidget = [](CameraParam& cameraParam, const QModelIndex& index, QWidget* parent) -> OneCustomWidget* {
        switch (cameraParam.type()) {
        case STRING:
            return new StringCustomWidget(cameraParam, index, parent);
        case CMD:
            return new CmdCustomWidget(cameraParam, index, parent);
        case INT:
            return new IntCustomWidget(cameraParam, index, parent);
        case DOUBLE:
            return new DoubleCustomWidget(cameraParam, index, parent);
        case BOOL:
            return new BoolCustomWidget(cameraParam, index, parent);
        case ENUM:
            return new EnumCustomWidget(cameraParam, index, parent);
        default:
            return nullptr;
        }
    };

    // 只为第1列（参数值列）创建编辑控件
    if (index.column() == CameraParamModel::ColType::VALUE) {
        const QVariant varParam = index.data(CameraParamModel::ParamRole);
        CameraParam cameraParam = varParam.value<CameraParam>();

        // 创建对应的编辑控件
        auto* oneCustomWidget = createWidget(cameraParam, index, parent);
        if (oneCustomWidget) {
            oneCustomWidget->InitWidget(); // 初始化控件（设置范围、选项等）
            // 连接值变化信号（用户修改时立即写回模型）
            connect(oneCustomWidget, &OneCustomWidget::sigValueChanged, this,
                &CameraParamDelegate::onValueChanged, Qt::UniqueConnection);
        }
        return oneCustomWidget;
    }
    return nullptr;
}

// ============================================================
// ============================================================
void CameraParamDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    if (index.column() == CameraParamModel::ColType::VALUE) {
        OneCustomWidget* pCustomEdit = qobject_cast<OneCustomWidget*>(editor);

        const QVariant varParam = index.data(CameraParamModel::ParamRole);
        CameraParam cameraParam = varParam.value<CameraParam>();

        pCustomEdit->setParam(cameraParam); // 把参数值显示到控件
    }
}

// ============================================================
// setModelData：把编辑控件的值写回模型
// ============================================================
void CameraParamDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if (index.column() == CameraParamModel::ColType::VALUE) {
        OneCustomWidget* pCustomEdit = qobject_cast<OneCustomWidget*>(editor);
        CameraParam cameraParam = pCustomEdit->getParam(); // 从控件取值
        model->setData(index, QVariant::fromValue(cameraParam), CameraParamModel::ParamRole);
    }
}

// 更新编辑器几何位置（让编辑控件填满单元格区域）
void CameraParamDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(index)
    editor->setGeometry(option.rect);
}

// ============================================================
// paint：自定义绘制
// * 1. 选中背景（高亮色）
// * 2. 文本内容（左对齐垂直居中）
// * 3. 网格线（右边框+下边框，浅灰色）
// ============================================================
void CameraParamDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
    const QModelIndex& index) const
{
    painter->save(); // 保存画笔状态

    // 1. 绘制选中背景
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    }

    // 2. 绘制文本内容
    QString text = index.data().toString();
    painter->drawText(option.rect, Qt::AlignLeft | Qt::AlignVCenter, text);

    // 3. 绘制网格线（浅灰色）
    QPen pen(QColor(220, 220, 220), 1, Qt::SolidLine);
    painter->setPen(pen);
    painter->drawLine(option.rect.topRight(), option.rect.bottomRight()); // 右边框
    painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight()); // 下边框

    painter->restore(); // 恢复画笔状态
}

// 单元格尺寸提示（固定行高20，宽度用默认）
QSize CameraParamDelegate::sizeHint(const QStyleOptionViewItem& option,
    const QModelIndex& index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(20); // 固定行高
    return size;
}

// ============================================================
// ============================================================
void CameraParamDelegate::onValueChanged(const CameraParam& param, const QModelIndex& index)
{
    OneCustomWidget* pCustomEdit = qobject_cast<OneCustomWidget*>(sender());
    if (pCustomEdit) {
        if (index.isValid()) {
            QAbstractItemModel* model = const_cast<QAbstractItemModel*>(index.model());
            if (model) {
                model->setData(index, QVariant::fromValue(param), CameraParamModel::ParamRole);
            }
        }
    }
}
