#ifndef IMAGEITEM_H
#define IMAGEITEM_H


#include <QGraphicsPixmapItem>

// ============================================================
// ImageItem：自定义图像项
// * 鼠标悬停时获取像素 RGB 值并发出信号
// ============================================================
class ImageItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    explicit ImageItem(QWidget* parent = nullptr);

signals:
    void RGBValue(QString InfoVal);

protected:
    // 鼠标悬停移动事件（获取当前位置的像素 RGB 值）
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent* event);

public:
    int w;
    int h; // 图像高度
};

#endif // IMAGEITEM_H
