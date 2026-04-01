#ifndef GRAPHICSVIEW_H
#define GRAPHICSVIEW_H


#include "ImageItem.h"
#include <QBoxLayout>
#include <QGraphicsView>
#include <QLabel>
#include <qevent.h>

// ============================================================
// GraphicsView：自定义图像视图
// ============================================================
class GraphicsView : public QGraphicsView {
    Q_OBJECT

public:
    GraphicsView(QWidget* parent = 0);
    ~GraphicsView();

    bool InitWidget();
    void SetImage(const QImage& image);
    void Clear(); // 清空图像

protected:
    virtual void wheelEvent(QWheelEvent* event) override; // 鼠标滚轮（缩放）
    virtual void mouseDoubleClickEvent(QMouseEvent* event) override; // 双击（居中自适应）
    virtual void paintEvent(QPaintEvent* event) override; // 绘制（棋盘格背景）
    virtual void resizeEvent(QResizeEvent* event) override; // 窗口大小变化（重新自适应）

public slots:
    void OnCenter(); // 视图居中显示
    void OnZoom(double scaleFactor); // 视图缩放

private:
    void fitFrame();
    void PrepareBackgroundBoard(bool invertColor = false); // 准备棋盘格背景

private:
    double m_dZoomValue = 1;

    QGraphicsScene* m_pScene; // 场景（管理所有图形项）
    ImageItem* m_pImageItem;
    QWidget* m_pPosInfoWidget; // 左下角位置信息容器
    QLabel* m_pPosInfoLabel;
    QImage m_qImage; // 当前显示的图像
    QPixmap m_qTilePixmap = QPixmap(36, 36); // 棋盘格背景瓦片（36x36）
    bool m_hasImage = false;
};

#endif // GRAPHICSVIEW_H
