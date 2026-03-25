#ifndef IMAGECONVER_H
#define IMAGECONVER_H

#include "opencv2/core/core.hpp" // OpenCV 核心（cv::Mat）
#include "opencv2/imgproc/imgproc.hpp" // OpenCV 图像处理（cvtColor 颜色转换）
#include <QtCore/QDebug>
#include <QtGui/QImage>

// ============================================================
// ============================================================
namespace ImageConver {

// ============================================================
// ============================================================
static QImage cvMat2QImage(const cv::Mat& mat, bool clone = true, bool rb_swap = true)
{
    const uchar* pSrc = (const uchar*)mat.data; // 图像数据指针

    // 灰度图：1 通道 8 位（CV_8UC1）
    if (mat.type() == CV_8UC1) {
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8);
        if (clone)
            return image.copy(); // 深拷贝一份
        return image;
    }
    else if (mat.type() == CV_8UC3) {
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
        if (clone) {
            if (rb_swap)
                return image.rgbSwapped();
            return image.copy();
        } else {
            if (rb_swap) {
                cv::cvtColor(mat, mat, cv::COLOR_BGR2RGB); // 原地转换 BGR→RGB
            }
            return image;
        }

    }
    else if (mat.type() == CV_8UC4) {
        qDebug() << "CV_8UC4";
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);
        if (clone)
            return image.copy();
        return image;
    } else {
        // 不支持的格式
        qDebug() << "ERROR: Mat could not be converted to QImage.";
        return QImage();
    }
}

// ============================================================
// ============================================================
static cv::Mat QImage2cvMat(QImage& image, bool clone = true, bool rb_swap = true)
{
    cv::Mat mat;
    qDebug() << image.format();
    switch (image.format()) {
    case QImage::Format_ARGB32:
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32_Premultiplied:
        mat = cv::Mat(image.height(), image.width(), CV_8UC4, (void*)image.constBits(), image.bytesPerLine());
        if (clone)
            mat = mat.clone(); // 深拷贝
        break;
    // 24 位彩色（RGB888）→ CV_8UC3
    case QImage::Format_RGB888:
        mat = cv::Mat(image.height(), image.width(), CV_8UC3, (void*)image.constBits(), image.bytesPerLine());
        if (clone)
            mat = mat.clone();
        if (rb_swap)
            cv::cvtColor(mat, mat, cv::COLOR_BGR2RGB);
        break;
    case QImage::Format_Indexed8:
    case QImage::Format_Grayscale8:
        mat = cv::Mat(image.height(), image.width(), CV_8UC1, (void*)image.bits(), image.bytesPerLine());
        if (clone)
            mat = mat.clone();
        break;
    }
    return mat;
}
}

#endif // IMAGECONVER_H
