#ifndef PARAMWIDGET_H
#define PARAMWIDGET_H


#include "Listener.h" // 监听者基类
#include <QTextBrowser>
#include <QTreeView>
#include <QVector>
#include <QWidget>

namespace Ui {
class ParamWidget;
}

class CameraParamDelegate; // 前向声明
class CameraParamModel; // 前向声明
class ControlWidget; // 前向声明
class CameraParam; // 前向声明

// ============================================================
// ParamWidget：参数面板
// * 功能：相机参数的读取、显示、编辑、写入
// ============================================================
class ParamWidget : public QWidget, Listener {
    Q_OBJECT

public:
    explicit ParamWidget(ControlWidget* controlWidget, QWidget* parent = nullptr);
    ~ParamWidget();

    void initParamWidget(QVector<CameraParam> paramList);
    // 清空参数面板（断开相机时调用）
    void clearParamWidget();
    void writeCameraParam(const QModelIndex& index);
    // 监听者接口实现：收到相机事件时更新参数面板
    void RespondMessage(int message) override;

signals:
    void SigUpdateErrorInfo(QString info);

public slots:
    void OnUpdataSelection(const QItemSelection& selected, const QItemSelection& deselected);

private slots:
    void on_Refresh_Button_clicked();

private:
    Ui::ParamWidget* ui; // .ui 界面对象
    QTreeView* m_pParamTreeView;
    QTextBrowser* m_pParamDescript;
    CameraParamModel* m_pModel;
    CameraParamDelegate* m_pCameraParamDelegate;
    QItemSelectionModel* m_pSelectionModel; // 选择模型（跟踪选中的参数）

    ControlWidget* m_pControlWidget;
};

#endif // PARAMWIDGET_H
