#include "mainwindow.h"
#include "AppStyle/AppStyle.h"
#include "CameraInterface/CameraContext.h"
#include "ui_mainwindow.h"
#include <QMessageBox>


MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    // 创建三个子界面
    , m_pControlWidget(new ControlWidget())
    , m_pParamWidget(new ParamWidget(m_pControlWidget))
    , m_pViewWidget(new ViewWidget(m_pControlWidget))
    , m_pErrorInfoLabel(new QLabel(""))
{
    ui->setupUi(this);

    ui->ControlWidget->layout()->addWidget(m_pControlWidget);
    ui->ParamWidget->layout()->addWidget(m_pParamWidget);
    ui->ViewWidget->layout()->addWidget(m_pViewWidget);
    // 错误信息标签放到状态栏
    ui->statusbar->addWidget(m_pErrorInfoLabel);
    this->resize(1000, 600); // 初始窗口大小

    // 连接三个子界面的错误信号到统一的错误处理槽
    // 任何一个子界面出错，都会在状态栏显示并弹窗
    connect(m_pControlWidget, &ControlWidget::SigUpdateErrorInfo, this, &MainWindow::OnUpdateErrorInfo);
    connect(m_pParamWidget, &ParamWidget::SigUpdateErrorInfo, this, &MainWindow::OnUpdateErrorInfo);
    connect(m_pViewWidget, &ViewWidget::SigUpdateErrorInfo, this, &MainWindow::OnUpdateErrorInfo);

    // 设置窗口标题和图标
    setWindowTitle(QStringLiteral("VCS - Vision Camera Studio"));
    setWindowIcon(QIcon(":/favicon.ico"));
    // 设置全局样式（QSS 美化）
    AppStyle::Polish();
}

MainWindow::~MainWindow()
{
    delete ui; // 释放 .ui 界面对象
    CameraContext::Release();
}

// ============================================================
// OnUpdateErrorInfo：错误信息处理槽
// * 在状态栏显示错误信息，如果非空则弹窗提示
// ============================================================
void MainWindow::OnUpdateErrorInfo(QString strErrorInfo)
{
    m_pErrorInfoLabel->setText(strErrorInfo); // 状态栏显示
    if (!strErrorInfo.isEmpty()) {
        // 弹窗提示（critical 是错误图标）
        QMessageBox::critical(this, "Error", strErrorInfo, "Close");
    }
}
