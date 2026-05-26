#include "ParamWidget.h"
#include "CameraInterface/CMCameraMetaInfo.h"
#include "CameraInterface/CMCameraParam.h"
#include "CameraInterface/CameraContext.h"
#include "CameraInterface/CameraError.h"
#include "ControlWidget/ControlWidget.h"
#include "ParamWidget/CameraParamDelegate.h"
#include "ParamWidget/CameraParamItem.h"
#include "ParamWidget/CameraParamModel.h"
#include "ui_ParamWidget.h"
#include <QDebug>


ParamWidget::ParamWidget(ControlWidget* controlWidget, QWidget* parent)
    : QWidget(parent)
    , Listener()
    , ui(new Ui::ParamWidget)
    , m_pCameraParamDelegate(new CameraParamDelegate())
    , m_pControlWidget(controlWidget)
{
    ui->setupUi(this);
    m_pParamTreeView = ui->Param_treeView;
    m_pParamDescript = ui->ParamDescript_textBrowser;
    m_pParamDescript->setFixedHeight(58); // 描述框固定高度

    // 初始化 MVD 三件套
    // Model：管理数据
    QStringList headerList;
    headerList << "Param" << "Value";
    m_pModel = new CameraParamModel(headerList);
    // View：显示数据
    m_pParamTreeView->setModel(m_pModel);
    // Delegate：负责创建编辑控件
    m_pParamTreeView->setItemDelegate(m_pCameraParamDelegate);
    m_pParamTreeView->expandAll(); // 默认展开所有分组
    m_pSelectionModel = m_pParamTreeView->selectionModel();

    // 描述框初始文本
    m_pParamDescript->append(QStringLiteral("相机参数注释"));

    // 连接信号槽
    // 选中参数变化 → 更新描述框
    connect(m_pSelectionModel, &QItemSelectionModel::selectionChanged,
        this, &ParamWidget::OnUpdataSelection);
    // 模型数据变化 → 写入相机
    connect(m_pModel, &CameraParamModel::SigValueChanged, this, &ParamWidget::writeCameraParam);

    // 注册事件监听（关心所有相机事件）
    ListenerManger::Instance()->registerMessage(MESSAGE::CAMERA_CONNECT
            | MESSAGE::CAMERA_DISCONNECT
            | MESSAGE::CAMERA_ENUMRTION
            | MESSAGE::CAMERA_STARTGRAB
            | MESSAGE::CAMERA_STOPTGRAB
            | MESSAGE::CAMERA_CAMERASWICH,
        this);
}

ParamWidget::~ParamWidget()
{
    delete ui;
}

// ============================================================
// ============================================================
void ParamWidget::initParamWidget(QVector<CameraParam> paramList)
{
    CameraMetaInfo currentCameraInfo = m_pControlWidget->GetCurrentCameraInfo();
    QString serial = currentCameraInfo.Serial;

    for (auto param : paramList) {
        // 从相机读取参数当前值
        auto ret = CameraContext::Instance()->readParam(serial, param);
        if (ret != CHONGMING_OK) {
            QString error = param.name() + QString(" read failed");
            emit SigUpdateErrorInfo(error);
        }

        // 加入模型（自动按分组组织成树形结构）
        m_pModel->addCameraParam(param);
    }
}

// 清空参数面板
void ParamWidget::clearParamWidget()
{
    m_pModel->clear(); // 清空模型数据
    m_pParamDescript->clear(); // 清空描述框
    m_pParamTreeView->reset(); // 重置视图
}

// ============================================================
// ============================================================
void ParamWidget::writeCameraParam(const QModelIndex& index)
{
    QVariant dataValue = m_pModel->data(index, CameraParamModel::ItemRoles::ParamRole);
    CameraParam curCameraParam = dataValue.value<CameraParam>();

    CameraMetaInfo currentCameraInfo = m_pControlWidget->GetCurrentCameraInfo();
    QString serial = currentCameraInfo.Serial;

    CHECK_RETURN(CameraContext::Instance()->writeParam(serial, curCameraParam));
}

// ============================================================
// RespondMessage：监听者接口实现
// * 根据事件类型更新参数面板状态：
// *   - 枚举/断开/切换：清空面板
// *   - 连接：启用面板，读取并显示参数
// *   - 停止拉流：启用面板
// ============================================================
void ParamWidget::RespondMessage(int message)
{
    // 枚举事件：清空面板
    if ((message & MESSAGE::CAMERA_ENUMRTION) == MESSAGE::CAMERA_ENUMRTION) {
        clearParamWidget();
    }
    // 连接事件：启用面板，读取参数
    if ((message & MESSAGE::CAMERA_CONNECT) == MESSAGE::CAMERA_CONNECT) {
        this->setEnabled(true);

        // 从相机获取参数列表（元信息）
        CameraMetaInfo currentCameraInfo = m_pControlWidget->GetCurrentCameraInfo();
        QVector<CameraParam> paramList;
        CameraContext::Instance()->getParamList(currentCameraInfo.Serial, paramList);
        initParamWidget(paramList); // 读取当前值并显示
        on_Refresh_Button_clicked(); // 刷新视图（展开所有节点）
    }
    // 断开事件：清空面板
    if ((message & MESSAGE::CAMERA_DISCONNECT) == MESSAGE::CAMERA_DISCONNECT) {
        clearParamWidget();
    }
    // 开始拉流：禁用面板（拉流中不允许改参数）
    if ((message & MESSAGE::CAMERA_STARTGRAB) == MESSAGE::CAMERA_STARTGRAB) {
        this->setEnabled(false);
    }
    // 停止拉流：启用面板
    if ((message & MESSAGE::CAMERA_STOPTGRAB) == MESSAGE::CAMERA_STOPTGRAB) {
        this->setEnabled(true);
    }
    // 相机切换：清空面板
    if ((message & MESSAGE::CAMERA_CAMERASWICH) == MESSAGE::CAMERA_CAMERASWICH) {
        clearParamWidget();
    }
}

// ============================================================
// ============================================================
void ParamWidget::OnUpdataSelection(const QItemSelection& selected, const QItemSelection& deselected)
{
    Q_UNUSED(deselected)
    m_pParamDescript->clear();

    QModelIndex index = selected.indexes().first();
    // 从模型的自定义角色取出参数描述（tips）
    QVariant varValue = m_pParamTreeView->model()->data(index, CameraParamModel::ParamDescriptionRole);
    QString strDescript = varValue.toString();
    m_pParamDescript->append(strDescript);
}

// 刷新按钮：重置视图并展开所有节点
void ParamWidget::on_Refresh_Button_clicked()
{
    m_pParamTreeView->reset();
    m_pParamTreeView->expandAll();
}
