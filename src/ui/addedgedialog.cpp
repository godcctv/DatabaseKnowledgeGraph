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
    QDialog { background-color: #2E3440; border: 1px solid #4C566A; }
    QLabel { color: #D8DEE9; font-weight: bold; font-size: 13px; }
    QFrame#InfoFrame {
        background-color: #3B4252; border: 1px solid #4C566A; border-radius: 4px;
    }
    QFrame#InfoFrame QLabel { color: #88C0D0; }
    QComboBox {
        background-color: #3B4252; border: 1px solid #4C566A; border-radius: 4px;
        padding: 6px; color: #ECEFF4; font-size: 13px;
    }
    QComboBox:focus { border: 1px solid #88C0D0; }
    QComboBox::drop-down { border: none; }
    QPushButton {
        background-color: #4C566A; color: #ECEFF4; border: 1px solid #434C5E;
        padding: 6px 20px; border-radius: 4px;
    }
    QPushButton:hover { background-color: #5E81AC; }
    QPushButton:pressed { background-color: #81A1C1; }
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