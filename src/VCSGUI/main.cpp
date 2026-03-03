#include "mainwindow.h"
#include <QApplication>


int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    MainWindow w;                 // 创建主窗口
    w.show();                     // 显示主窗口
    return a.exec();
}
