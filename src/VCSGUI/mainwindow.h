#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include "ControlWidget/ControlWidget.h" // 控制面板（相机枚举、连接、拉流、切换）
#include "ParamWidget/ParamWidget.h" // 参数面板（相机参数的显示和编辑）
#include "ViewWidget/ViewWidget.h" // 视觉窗口（实时图像显示）
#include <QLabel>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

// ============================================================
// MainWindow：主窗口
// ============================================================
class MainWindow : public QMainWindow {
Q_OBJECT // Qt 元对象宏（信号槽需要）

    public : MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

public slots:
    void OnUpdateErrorInfo(QString strErrorInfo);

private:
    Ui::MainWindow* ui;
    ControlWidget* m_pControlWidget;
    ParamWidget* m_pParamWidget; // 参数面板（右侧：相机参数树）
    ViewWidget* m_pViewWidget; // 视觉窗口（中央：实时图像显示）
    QLabel* m_pErrorInfoLabel; // 状态栏错误信息标签
};
#endif // MAINWINDOW_H
