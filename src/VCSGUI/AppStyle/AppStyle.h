#ifndef APPSTYLE_H
#define APPSTYLE_H


#include <QApplication>

// ============================================================
// AppStyle：全局样式类
// ============================================================
class AppStyle {
public:
    AppStyle() { };
    static void Polish();

private:
    static void setPalette(); // 设置调色板（全局颜色主题）
    static void setQss();
};

#endif // APPSTYLE_H
