#ifndef VIEWWIDGET_H
#define VIEWWIDGET_H


#include "Listener.h"
#include <QWidget>

namespace Ui {
class ViewWidget;
}

class GraphicsView; // 前向声明
class ControlWidget; // 前向声明
class AcquireImageProcess; // 前向声明

// ============================================================
// ViewWidget：视觉窗口
// * 功能：实时图像显示、拉流控制、取图线程管理
// ============================================================
class ViewWidget : public QWidget, Listener {
    Q_OBJECT

public:
    explicit ViewWidget(ControlWidget* controlWidget, QWidget* parent = nullptr);
    ~ViewWidget();

    // 监听者接口实现
    void RespondMessage(int message) override;

    void stopImageProcess();

signals:
    void SigUpdateErrorInfo(QString info);

private slots:
    void on_Grabbing_Button_toggled(bool checked); // 拉流/停止按钮

private:
    void setStarGrabbingState(bool state); // 设置拉流按钮图标状态

private:
    Ui::ViewWidget* ui;
    GraphicsView* m_pViewBox;

    ControlWidget* m_pControlWidget;
    AcquireImageProcess* m_pImageProcess;
};

#endif // VIEWWIDGET_H
