#include "addedgedialog.h"
#include "ui_addedgedialog.h"
#include <QMessageBox>

AddEdgeDialog::AddEdgeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddEdgeDialog)
{
    ui->setupUi(this);

    // 初始化一些常用关系类型
    ui->typeCombo->addItems({"Include", "RelatedTo", "Dependency", "SubClassOf"});

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