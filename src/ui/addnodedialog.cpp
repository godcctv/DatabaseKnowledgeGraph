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

    // 1. 隐藏 UI 文件中原生绝对定位的 Label，我们将使用 QFormLayout 重新排版
    ui->label->hide();
    ui->label_2->hide();

    // 2. 清空下拉框并开启编辑模式
    ui->typeCombo->clear();
    ui->typeCombo->setEditable(true);

    // 3. 读取本地保存的类型预设
    QSettings settings("KnowledgeGraphSystem", "Presets");
    QStringList defaultTypes = {"概念", "实体", "方法"};
    QStringList savedTypes = settings.value("NodeTypes", defaultTypes).toStringList();
    savedTypes.removeDuplicates();
    ui->typeCombo->addItems(savedTypes);

    // 4. 新建描述输入框
    descEdit = new QTextEdit(this);
    descEdit->setMaximumHeight(80);

    // 5. 重构界面布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(25, 25, 25, 20);
    mainLayout->setSpacing(15);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(15);
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // 将输入框加入表单布局，并赋予整齐的标签
    formLayout->addRow("节点名称 :", ui->nameEdit);
    formLayout->addRow("节点类型 :", ui->typeCombo);
    formLayout->addRow("详细描述 :", descEdit);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(ui->btnOk);
    btnLayout->addWidget(ui->btnCancel);

    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(btnLayout);

    this->resize(320, 260);

    // 美化样式
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
    node.nodeType = ui->typeCombo->currentText().trimmed();
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

    // 保存新的节点类型
    QSettings settings("KnowledgeGraphSystem", "Presets");
    QStringList defaultTypes = {"概念", "实体", "方法"};
    QStringList savedTypes = settings.value("NodeTypes", defaultTypes).toStringList();

    if (!savedTypes.contains(currentType)) {
        savedTypes.append(currentType);
    }
    savedTypes.removeDuplicates(); // 保存前再次兜底去重
    settings.setValue("NodeTypes", savedTypes);

    accept();
}

void AddNodeDialog::on_btnCancel_clicked() {
    reject();
}

void AddNodeDialog::setNodeData(const GraphNode& node) {
    ui->nameEdit->setText(node.name);
    ui->typeCombo->setCurrentText(node.nodeType);
    descEdit->setPlainText(node.description);
}