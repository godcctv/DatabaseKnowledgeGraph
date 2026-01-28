#include "addnodedialog.h"
#include "ui_addnodedialog.h"
#include <QRandomGenerator>
#include <QMessageBox>
#include <QDateTime>

AddNodeDialog::AddNodeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddNodeDialog)
{
    ui->setupUi(this);

    // 初始化下拉框类型
    ui->typeCombo->addItem("Concept");
    ui->typeCombo->addItem("Entity");
    ui->typeCombo->addItem("Method");

    // 连接信号槽（如果UI里没选自动连接）
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
