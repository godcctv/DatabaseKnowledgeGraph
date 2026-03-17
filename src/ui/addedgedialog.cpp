#include "addedgedialog.h"
#include "ui_addedgedialog.h"
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFrame>
AddEdgeDialog::AddEdgeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddEdgeDialog)
{
    ui->setupUi(this);

    ui->label->hide();

    ui->typeCombo->addItems({"包含", "关联", "依赖", "子类"});

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(25, 20, 25, 20);
    mainLayout->setSpacing(15);

    // 给起点和终点信息加一个带边框的信息面板
    QFrame* infoFrame = new QFrame(this);
    infoFrame->setObjectName("InfoFrame");
    QVBoxLayout* infoLayout = new QVBoxLayout(infoFrame);
    infoLayout->setSpacing(8);
    infoLayout->addWidget(ui->lblSource);
    infoLayout->addWidget(ui->lblTarget);

    // 表单布局放类型选择
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    formLayout->addRow("关系类型 :", ui->typeCombo);

    // 底部按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(ui->btnOk);
    btnLayout->addWidget(ui->btnCancel);

    // 组装布局
    mainLayout->addWidget(infoFrame);
    mainLayout->addSpacing(5);
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(btnLayout);

    this->resize(300, 220);

    this->setStyleSheet(R"(
        QDialog { background-color: #161925; border: 1px solid #2A2F45; }
        QLabel { color: #A0AAB5; font-weight: bold; font-size: 13px; }
        QFrame#InfoFrame {
            background-color: rgba(0, 229, 255, 0.05); /* 微弱的青色背景 */
            border: 1px dashed #3a6ea5;
            border-radius: 6px;
        }
        QFrame#InfoFrame QLabel { color: #00E5FF; } /* 强调首尾节点文字 */
        QComboBox {
            background-color: #08090F;
            border: 1px solid #2A2F45;
            border-radius: 6px;
            padding: 6px;
            color: #00E5FF;
            font-size: 13px;
        }
        QComboBox:focus { border: 1px solid #00E5FF; }
        QComboBox::drop-down { border: none; }
        QPushButton {
            background-color: #1e3a5a;
            color: #d0e6ff;
            border: 1px solid #3a6ea5;
            padding: 6px 20px;
            border-radius: 6px;
            font-weight: bold;
        }
        QPushButton:hover { background-color: #3a6ea5; border-color: #00E5FF; color: #ffffff; }
        QPushButton:pressed { background-color: #0B0D17; }
    )");

    ui->btnOk->setDefault(true);

    // 连接信号
    connect(ui->btnOk, &QPushButton::clicked, this, &AddEdgeDialog::on_btnOk_clicked);
    connect(ui->btnCancel, &QPushButton::clicked, this, &AddEdgeDialog::on_btnCancel_clicked);
}

AddEdgeDialog::~AddEdgeDialog() {
    delete ui;
}

void AddEdgeDialog::setNodes(const QString& sourceName, const QString& targetName) {
    ui->lblSource->setText(sourceName);
    ui->lblTarget->setText(targetName);
}

GraphEdge AddEdgeDialog::getEdgeData() const {
    GraphEdge edge;
    edge.relationType = ui->typeCombo->currentText();
    // 默认权重 1.0，如果UI加了输入框可以在这里获取
    edge.weight = 1.0;
    return edge;
}

void AddEdgeDialog::on_btnOk_clicked() {
    if(ui->typeCombo->currentText().isEmpty()) {
        QMessageBox::warning(this, "提示", "必须选择或输入关系类型");
        return;
    }
    accept();
}

void AddEdgeDialog::on_btnCancel_clicked() {
    reject();
}

void AddEdgeDialog::setEdgeData(const GraphEdge& edge) {
    ui->typeCombo->setCurrentText(edge.relationType);
}