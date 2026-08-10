#include "GraphicsView.h"
#include <QMutexLocker>

#define ZOOMMAX 50 // 最大放大倍数（原始图像的50倍）
#define ZOOMMIN 0.1 // 最小缩小倍数（原始图像的0.1倍）


GraphicsView::GraphicsView(QWidget* parent)
    : QGraphicsView(parent)
    , m_pScene(Q_NULLPTR)
    , m_pImageItem(Q_NULLPTR)
    , m_pPosInfoWidget(Q_NULLPTR)
    , m_pPosInfoLabel(Q_NULLPTR)
{
    // 禁用滚动条（用拖动代替滚动条）
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // 反锯齿（图像缩放更平滑）
    this->setRenderHint(QPainter::Antialiasing);
    // 缩放锚点在鼠标位置（鼠标指向的点缩放后仍在鼠标下）
    this->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    // 全视口更新（解决拖动时背景残影）
    this->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    // 拖动模式：手型拖动（按住左键拖动平移视图）
    setDragMode(QGraphicsView::ScrollHandDrag);
    // 设置场景范围（很大的范围，允许图像在任意位置）
    this->setSceneRect(INT_MIN / 2, INT_MIN / 2, INT_MAX, INT_MAX);
    // 准备棋盘格背景
    PrepareBackgroundBoard();
    centerOn(0, 0);

    if (false == InitWidget()) {
        throw std::bad_alloc(); // 初始化失败抛异常
    }
}

GraphicsView::~GraphicsView()
{
    m_pScene->deleteLater();
    delete m_pImageItem;
}

// ============================================================
// ============================================================
bool GraphicsView::InitWidget()
{
    // 创建场景和图像项
    m_pScene = new QGraphicsScene(this);
    m_pImageItem = new ImageItem(this);
    this->setScene(m_pScene);
    m_pScene->addItem(m_pImageItem); // 把图像项加入场景

    m_pPosInfoLabel = new QLabel(this);
    m_pPosInfoWidget = new QWidget(this);

    // 标签样式：绿色文字、半透明深灰背景
    m_pPosInfoLabel->setStyleSheet("color:rgb(200,255,200); "
                                   "background-color:rgba(50,50,50,160); "
                                   "font: Microsoft YaHei;"
                                   "font-size: 15px;");
    m_pPosInfoLabel->setText(" W:0,H:0 | X:0,Y:0 | R:0,G:0,B:0");
    // 位置信息容器固定在底部，高度25
    m_pPosInfoWidget->setFixedHeight(25);
    m_pPosInfoWidget->setGeometry(0, this->height() - 25, this->width(), 25);
    m_pPosInfoWidget->setStyleSheet("background-color:rgba(0,0,0,0);");
    QHBoxLayout* pInfoLayout = new QHBoxLayout();
    pInfoLayout->setSpacing(0);
    pInfoLayout->setContentsMargins(0, 0, 0, 0);
    pInfoLayout->addWidget(m_pPosInfoLabel);
    m_pPosInfoWidget->setLayout(pInfoLayout);

    // 连接图像项的 RGB 值信号：鼠标悬停时更新标签
    // 用 lambda 捕获 this，直接设置标签文本
    connect(m_pImageItem, &ImageItem::RGBValue, this, [&](QString InfoVal) {
        m_pPosInfoLabel->setText(InfoVal);
    });

    return true;
}

// ============================================================
// ============================================================
void GraphicsView::SetImage(const QImage& image)
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);

    const bool imageChanged = !m_hasImage
        || m_qImage.size() != image.size()
        || image.isNull();

    m_qImage = image.copy(); // 深拷贝（防止外部修改影响显示）
    auto qPixmap = QPixmap::fromImage(image);
    // 更新图像项的尺寸和 Pixmap
    m_pImageItem->w = qPixmap.width();
    m_pImageItem->h = qPixmap.height();
    m_pImageItem->setPixmap(qPixmap);

    if (imageChanged) {
        resetTransform(); // 重置变换（缩放/平移归零）
        m_dZoomValue = 1.0;
        if (!image.isNull()) {
            fitFrame(); // 自适应大小
            OnCenter(); // 居中
            m_hasImage = true;
        } else {
            m_hasImage = false;
        }
    }
    show();
}

// 清空图像和位置信息
void GraphicsView::Clear()
{
    m_pPosInfoLabel->setText(" W:0,H:0 | X:0,Y:0 | R:0,G:0,B:0");
    SetImage(QImage()); // 设置空图像
}

// ============================================================
// wheelEvent：鼠标滚轮事件（缩放）
// * 滚轮向上（远离使用者）= 放大 1.1x
// * 滚轮向下（朝向使用者）= 缩小 0.9x
// ============================================================
void GraphicsView::wheelEvent
    // TODO: 缩放动画效果后续优化
    // TODO: 缩放围绕鼠标位置，当前为中心缩放(QWheelEvent* event)
{
    if (!m_hasImage || event->angleDelta().y() == 0) {
        event->ignore();
        return;
    }
    QPoint scrollAmount = event->angleDelta();
    // 达到最大/最小倍数时不再缩放
    if ((scrollAmount.y() > 0) && (m_dZoomValue >= ZOOMMAX)) {
        event->accept();
        return;
    } else if ((scrollAmount.y() < 0) && (m_dZoomValue <= ZOOMMIN)) {
        event->accept();
        return;
    }

    // 向上放大1.1x，向下缩小0.9x
    scrollAmount.y() > 0 ? OnZoom(1.1) : OnZoom(0.9);
    event->accept();
}

// ============================================================
// * 双击左键：自适应图像大小 + 居中显示
// ============================================================
void GraphicsView::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_hasImage) {
        fitFrame(); // 自适应大小
        OnCenter(); // 居中
    }
    QGraphicsView::mouseDoubleClickEvent(event); // 调用基类处理
}

// ============================================================
// paintEvent：绘制事件（棋盘格背景）
// ============================================================
void GraphicsView::paintEvent(QPaintEvent* event)
{
    QPainter paint(this->viewport());
    // 绘制棋盘格背景（平铺 36x36 的瓦片）
    paint.drawTiledPixmap(QRect(QPoint(0, 0), QPoint(this->width(), this->height())), m_qTilePixmap);
    QGraphicsView::paintEvent(event); // 基类绘制场景和图像
}

// ============================================================
// resizeEvent：窗口大小变化事件
// ============================================================
void GraphicsView::resizeEvent(QResizeEvent* event)
{
    fitFrame();
    OnCenter();
    // 位置信息标签始终在底部
    m_pPosInfoWidget->setGeometry(0, this->height() - 25, this->width(), 25);
    QGraphicsView::resizeEvent(event);
}

// ============================================================
// OnCenter：视图居中
// ============================================================
void GraphicsView::OnCenter()
{
    if (!m_hasImage || m_pImageItem->pixmap().isNull()) {
        return;
    }
    // centerOn 把视图中心移到指定场景坐标
    this->centerOn(m_pImageItem->pixmap().width() / 2, m_pImageItem->pixmap().height() / 2);
    m_pImageItem->setPos(0, 0); // 图像项位于场景原点
}

// ============================================================
// OnZoom：视图缩放
// ============================================================
void GraphicsView::OnZoom(double scaleFactor)
{
    if (!m_hasImage || scaleFactor <= 0.0) {
        return;
    }

    // 计算目标缩放比例，超过限制时截断
    const double targetZoom = m_dZoomValue * scaleFactor;
    if (targetZoom > ZOOMMAX) {
        scaleFactor = ZOOMMAX / m_dZoomValue;
    } else if (targetZoom < ZOOMMIN) {
        scaleFactor = ZOOMMIN / m_dZoomValue;
    }

    // 记录当前缩放比例（相对于原始图像大小）
    m_dZoomValue *= scaleFactor;
    this->scale(scaleFactor, scaleFactor);
}

// ============================================================
// fitFrame：自适应大小
// * 然后调用 OnZoom 补齐缩放比例
// ============================================================
void GraphicsView::fitFrame()
{
    if (this->viewport()->width() < 1 || m_qImage.width() < 1)
        return;

    // 计算图像宽高与视图宽高的比例
    double winWidth = this->viewport()->width();
    double winHeight = this->viewport()->height();
    double ScaleWidth = (m_qImage.width() + 1) / winWidth;
    double ScaleHeight = (m_qImage.height() + 1) / winHeight;
    double s_temp = ScaleWidth >= ScaleHeight ? 1 / ScaleWidth : 1 / ScaleHeight;
    double scale = s_temp / m_dZoomValue;

    OnZoom(scale); // 执行缩放
    m_dZoomValue = s_temp; // 更新为目标缩放比例
}

// ============================================================
// * 平铺后形成棋盘格效果（透明区域显示棋盘格）
// ============================================================
void GraphicsView::PrepareBackgroundBoard(bool invertColor)
{
    // 填充基础色（深灰）
    m_qTilePixmap.fill(invertColor ? QColor(220, 220, 220) : QColor(35, 35, 35));
    QPainter tilePainter(&m_qTilePixmap);
    constexpr QColor color(50, 50, 50, 255);
    constexpr QColor invertedColor(210, 210, 210, 255);
    tilePainter.fillRect(0, 0, 18, 18, invertColor ? invertedColor : color);
    tilePainter.fillRect(18, 18, 18, 18, invertColor ? invertedColor : color);
    tilePainter.end();
}
