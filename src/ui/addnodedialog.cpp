#include "addnodedialog.h"
#include "ui_addnodedialog.h"
#include <QRandomGenerator>
#include <QMessageBox>
#include <QDateTime>
#include <QTextEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>


AddNodeDialog::AddNodeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddNodeDialog)
{
    ui->setupUi(this);

    ui->label->hide();
    ui->label_2->hide();

    // 初始化下拉框类型
    ui->typeCombo->addItem("概念");
    ui->typeCombo->addItem("实体");
    ui->typeCombo->addItem("方法");

    descEdit = new QTextEdit(this);
    descEdit->setMaximumHeight(80);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(25, 25, 25, 20);
    mainLayout->setSpacing(15);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(15);
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    formLayout->addRow("名称 :", ui->nameEdit);
    formLayout->addRow("类型 :", ui->typeCombo);
    formLayout->addRow("描述 :", descEdit);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(ui->btnOk);
    btnLayout->addWidget(ui->btnCancel);

    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(btnLayout);

    this->resize(320, 260);

    this->setStyleSheet(R"(
        QDialog { background-color: #161925; border: 1px solid #2A2F45; }
        QLabel { color: #A0AAB5; font-weight: bold; font-size: 13px; }
        QLineEdit, QComboBox, QTextEdit {
            background-color: #08090F;
            border: 1px solid #2A2F45;
            border-radius: 6px;
            padding: 6px;
            color: #00E5FF;
            font-size: 13px;
        }
        QLineEdit:focus, QComboBox:focus, QTextEdit:focus { border: 1px solid #00E5FF; }
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

    // 连接信号槽
    connect(ui->btnOk, &QPushButton::clicked, this, &AddNodeDialog::on_btnOk_clicked);
    connect(ui->btnCancel, &QPushButton::clicked, this, &AddNodeDialog::on_btnCancel_clicked);
}

AddNodeDialog::~AddNodeDialog() {
    delete ui;
}

GraphNode AddNodeDialog::getNodeData() const {
    GraphNode node;
    node.name = ui->nameEdit->text().trimmed();
    node.nodeType = ui->typeCombo->currentText();
    node.description = descEdit->toPlainText().trimmed();
    node.posX = QRandomGenerator::global()->bounded(50, 450);
    node.posY = QRandomGenerator::global()->bounded(50, 350);

    return node;
}

void AddNodeDialog::on_btnOk_clicked() {
    // 简单校验
    if(ui->nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "提示", "节点名称不能为空！");
        return;
    }
    accept(); // 关闭对话框并返回 QDialog::Accepted
}

void AddNodeDialog::on_btnCancel_clicked() {
    reject(); // 关闭对话框并返回 QDialog::Rejected
}

void AddNodeDialog::setNodeData(const GraphNode& node) {
    ui->nameEdit->setText(node.name);
    ui->typeCombo->setCurrentText(node.nodeType);
    descEdit->setPlainText(node.description);
}