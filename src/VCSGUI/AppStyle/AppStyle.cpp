#include "AppStyle/AppStyle.h"

#include <QFile>
#include <QStyle>
#include <QStyleFactory>


// 入口函数：应用全部样式
void AppStyle::Polish()
{
    setPalette(); // 设置调色板
    setQss(); // 设置 QSS 样式表
}

// ============================================================
// setPalette：设置调色板（全局颜色主题）
// * 颜色角色说明：
// *   - Window：窗口背景色
// *   - WindowText：窗口文字色
// *   - Base：输入框/列表背景色
// *   - Text：文字色
// *   - Button：按钮背景色
// *   - ButtonText：按钮文字色
// *   - Highlight：选中高亮色
// ============================================================
void AppStyle::setPalette()
{
    // 设置 Fusion 风格（Qt 内置的现代风格）
    qApp->setStyle(QStyleFactory::create("Fusion"));
    qApp->setPalette(QApplication::style()->standardPalette());

    // 创建自定义调色板
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(200, 200, 200)); // 窗口背景（浅灰）
    palette.setColor(QPalette::WindowText, Qt::black); // 窗口文字（黑）
    palette.setColor(QPalette::Disabled, QPalette::WindowText,
        QColor(127, 127, 127)); // 禁用时窗口文字（灰）
    palette.setColor(QPalette::Base, QColor(246, 246, 246)); // 输入框背景（近白）
    palette.setColor(QPalette::AlternateBase, QColor(64, 157, 224)); // 交替行背景（蓝）
    palette.setColor(QPalette::ToolTipBase, Qt::white); // 提示框背景
    palette.setColor(QPalette::ToolTipText, Qt::black); // 提示框文字
    palette.setColor(QPalette::Text, Qt::black); // 文字色
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(127, 127, 127)); // 禁用文字
    palette.setColor(QPalette::Dark, QColor(72, 72, 72)); // 暗色（阴影）
    palette.setColor(QPalette::Shadow, Qt::black); // 阴影色
    palette.setColor(QPalette::Mid, QColor(77, 77, 77)); // 中间色
    palette.setColor(QPalette::Button, QColor(88, 88, 88)); // 按钮背景（深灰）
    palette.setColor(QPalette::Light, QColor(98, 98, 98)); // 亮色（按钮高光）
    palette.setColor(QPalette::ButtonText, Qt::white); // 按钮文字（白）
    palette.setColor(QPalette::Disabled, QPalette::ButtonText,
        QColor(127, 127, 127)); // 禁用按钮文字
    palette.setColor(QPalette::BrightText, QColor(247, 181, 84)); // 亮色文字（橙）
    palette.setColor(QPalette::Link, QColor(222, 137, 10)); // 链接色（橙）
    palette.setColor(QPalette::Highlight, QColor(246, 134, 86)); // 选中高亮色（橙）
    palette.setColor(QPalette::Disabled, QPalette::Highlight,
        QColor(127, 127, 127)); // 禁用时高亮色
    palette.setColor(QPalette::HighlightedText, Qt::white); // 选中时文字色
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText,
        QColor(127, 127, 127)); // 禁用选中文字

    // 应用到全局应用程序
    qApp->setPalette(palette);
}

// ============================================================
// setQss：设置 QSS 样式表
// ============================================================
void AppStyle::setQss()
{
    // 从资源文件加载 QSS
    QFile qssfile(":/vcs.qss");
    if (qssfile.open(QFile::ReadOnly)) {
        qApp->setStyleSheet(qssfile.readAll()); // 应用到全局
        qssfile.close();
    }
}
