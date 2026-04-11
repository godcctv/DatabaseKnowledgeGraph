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
#include <QSettings>

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
    // 开启下拉框的文本输入功能，允许用户手动输入新类型
    ui->typeCombo->setEditable(true);

    QSettings settings("KnowledgeGraphSystem", "Presets");
    QStringList defaultTypes = {"概念", "实体", "方法"};
    QStringList savedTypes = settings.value("NodeTypes", defaultTypes).toStringList();
    ui->typeCombo->addItems(savedTypes);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(ui->btnOk);
    btnLayout->addWidget(ui->btnCancel);

    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(btnLayout);

    this->resize(320, 260);

    this->setStyleSheet(R"(
    QDialog { background-color: #2E3440; border: 1px solid #4C566A; }
    QLabel { color: #D8DEE9; font-weight: bold; font-size: 13px; }
    QLineEdit, QComboBox, QTextEdit {
        background-color: #3B4252; border: 1px solid #4C566A; border-radius: 4px;
        padding: 6px; color: #ECEFF4; font-size: 13px;
    }
    QLineEdit:focus, QComboBox:focus, QTextEdit:focus { border: 1px solid #88C0D0; }
    QComboBox::drop-down { border: none; }
    QPushButton {
        background-color: #4C566A; color: #ECEFF4; border: 1px solid #434C5E;
        padding: 6px 20px; border-radius: 4px;
    }
    QPushButton:hover { background-color: #5E81AC; }
    QPushButton:pressed { background-color: #81A1C1; }
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

    if(ui->nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "提示", "节点名称不能为空！");
        return;
    }

    QString currentType = ui->typeCombo->currentText().trimmed();
    if(currentType.isEmpty()) {
        QMessageBox::warning(this, "提示", "节点类型不能为空！");
        return;
    }

    QSettings settings("KnowledgeGraphSystem", "Presets");
    QStringList defaultTypes = {"概念", "实体", "方法"};
    QStringList savedTypes = settings.value("NodeTypes", defaultTypes).toStringList();

    if (!savedTypes.contains(currentType)) {
        savedTypes.append(currentType);
        settings.setValue("NodeTypes", savedTypes);
    }

    accept();
}

void AddNodeDialog::on_btnCancel_clicked() {
    reject(); // 关闭对话框并返回 QDialog::Rejected
}

void AddNodeDialog::setNodeData(const GraphNode& node) {
    ui->nameEdit->setText(node.name);
    ui->typeCombo->setCurrentText(node.nodeType);
    descEdit->setPlainText(node.description);
}