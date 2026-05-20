#ifndef CAMERAPARAMDELEGATE_H
#define CAMERAPARAMDELEGATE_H


#include "CameraInterface/CMCameraParam.h"
#include <QModelIndex>
#include <QPainter>
#include <QStyledItemDelegate>

// ============================================================
// CameraParamDelegate：参数委托
// ============================================================
class CameraParamDelegate : public QStyledItemDelegate {
public:
    CameraParamDelegate(QObject* parent = Q_NULLPTR);
    virtual ~CameraParamDelegate();

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
        const QModelIndex& index) const override;
    // 设置编辑器数据（把模型中的值显示到编辑控件）
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    // 设置模型数据（把编辑控件的值写回模型）
    void setModelData(QWidget* editor, QAbstractItemModel* model,
        const QModelIndex& index) const override;
    // 更新编辑器几何位置（让编辑控件填满单元格）
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
        const QModelIndex& index) const override;
    // 自定义绘制（选中背景、文本、网格线）
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
        const QModelIndex& index) const override;
    // 单元格尺寸提示（这里固定行高20）
    QSize sizeHint(const QStyleOptionViewItem& option,
        const QModelIndex& index) const override;

protected slots:
    void onValueChanged(const CameraParam& param, const QModelIndex& index);
};

#endif // CAMERAPARAMDELEGATE_H
