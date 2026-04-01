#include "ImageItem.h"
#include <QGraphicsSceneHoverEvent>


ImageItem::ImageItem(QWidget* parent)
    : QGraphicsPixmapItem(nullptr)
{
    setAcceptHoverEvents(true); // 启用鼠标悬停事件（默认不启用）
}

// ============================================================
// hoverMoveEvent：鼠标悬停移动事件
// ============================================================
void ImageItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    if (pixmap().isNull()) {
        event->ignore();
        return;
    }

    // 获取鼠标在图像项中的坐标（相对于图像项左上角）
    QPointF mousePosition = event->pos();
    int R, G, B;
    int x, y;
    x = mousePosition.x();
    y = mousePosition.y();
    // 限制坐标在图像范围内（防止越界访问导致崩溃）
    x = qBound(0, x, pixmap().width() - 1);
    y = qBound(0, y, pixmap().height() - 1);
    // 获取指定位置的像素颜色
    pixmap().toImage().pixelColor(x, y).getRgb(&R, &G, &B);

    QString InfoVal = QString(" W:%1,H:%2 | X:%3,Y:%4 | R:%5,G:%6,B:%7")
                          .arg(QString::number(w))
                          .arg(QString::number(h))
                          .arg(QString::number(x))
                          .arg(QString::number(y))
                          .arg(QString::number(R))
                          .arg(QString::number(G))
                          .arg(QString::number(B));
    emit RGBValue(InfoVal);
    event->accept();
}
