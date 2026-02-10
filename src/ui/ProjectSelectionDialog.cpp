#include "ProjectSelectionDialog.h"
#include "../database/OntologyRepository.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QDebug>

ProjectSelectionDialog::ProjectSelectionDialog(QWidget *parent)
    : QDialog(parent), m_selectedId(-1)
{
    setWindowTitle("选择知识库项目");
    resize(600, 400);

    // 应用深色样式
    setStyleSheet(R"(
        QDialog { background-color: #0E1019; color: #E0E6ED; }
        QListWidget {
            background-color: #161925;
            border: 1px solid #2A2F45;
            border-radius: 4px;
            color: #E0E6ED;
            font-size: 14px;
        }
        QListWidget::item {
            height: 40px;
            padding: 5px;
        }
        QListWidget::item:selected {
            background-color: #00E5FF;
            color: #000000;
            border-radius: 2px;
        }
        QPushButton {
            background-color: #2A2F45;
            color: white;
            border: none;
            padding: 8px 16px;
            border-radius: 4px;
            font-weight: bold;
        }
        QPushButton:hover { background-color: #3A3F55; }
        QPushButton#BtnPrimary {
            background-color: #00E5FF;
            color: #000000;
        }
        QPushButton#BtnPrimary:hover { background-color: #00B8D4; }
        QPushButton#BtnDanger { color: #FF6B6B; background-color: transparent; border: 1px solid #FF6B6B; }
        QPushButton#BtnDanger:hover { background-color: rgba(255, 107, 107, 0.1); }
    )");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(30, 30, 30, 30);

    // 标题
    QLabel* title = new QLabel("Knowledge Graph Projects", this);
    title->setStyleSheet("font-size: 24px; font-weight: bold; color: #00E5FF; margin-bottom: 10px;");
    mainLayout->addWidget(title);

    // 项目列表
    m_projectList = new QListWidget(this);
    mainLayout->addWidget(m_projectList);

    // 按钮区域
    QHBoxLayout* btnLayout = new QHBoxLayout();

    m_btnCreate = new QPushButton("＋ 新建项目", this);
    m_btnDelete = new QPushButton("删除项目", this);
    m_btnDelete->setObjectName("BtnDanger");

    m_btnOpen = new QPushButton("打开项目", this);
    m_btnOpen->setObjectName("BtnPrimary"); //以此标记为主按钮
    m_btnOpen->setDefault(true);

    btnLayout->addWidget(m_btnCreate);
    btnLayout->addWidget(m_btnDelete);
    btnLayout->addStretch(); // 弹簧，把“打开”按钮顶到右边
    btnLayout->addWidget(m_btnOpen);

    mainLayout->addLayout(btnLayout);

    // 连接信号槽
    connect(m_btnCreate, &QPushButton::clicked, this, &ProjectSelectionDialog::onCreateProject);
    connect(m_btnDelete, &QPushButton::clicked, this, &ProjectSelectionDialog::onDeleteProject);
    connect(m_btnOpen, &QPushButton::clicked, this, &ProjectSelectionDialog::onOpenProject);
    connect(m_projectList, &QListWidget::itemDoubleClicked, this, &ProjectSelectionDialog::onItemDoubleClicked);

    // 初始加载
    loadProjects();
}

void ProjectSelectionDialog::loadProjects() {
    m_projectList->clear();
    QList<Ontology> ontologies = OntologyRepository::getAllOntologies();

    for (const auto& onto : ontologies) {
        QListWidgetItem* item = new QListWidgetItem(m_projectList);
        // 使用 emoji 美化
        item->setText(QString("📘  %1 (v%2)").arg(onto.name).arg(onto.version));
        // 存储 ID
        item->setData(Qt::UserRole, onto.id);
        // 存储 纯名称（用于显示）
        item->setData(Qt::UserRole + 1, onto.name);
    }
}

void ProjectSelectionDialog::onCreateProject() {
    QString name = QInputDialog::getText(this, "新建项目", "请输入项目名称:");
    if (name.trimmed().isEmpty()) return;

    QString desc = QInputDialog::getText(this, "项目描述", "请输入描述 (可选):");

    // 【修改点】: 之前这里是传 newOnto 对象，现在改为传 name 和 desc 两个字符串
    if (OntologyRepository::addOntology(name, desc)) {
        loadProjects();
    } else {
        QMessageBox::warning(this, "错误", "创建失败，可能名称已存在。");
    }
}

void ProjectSelectionDialog::onDeleteProject() {
    QListWidgetItem* item = m_projectList->currentItem();
    if (!item) return;

    int id = item->data(Qt::UserRole).toInt();
    QString name = item->data(Qt::UserRole + 1).toString();

    auto reply = QMessageBox::question(this, "确认删除",
        QString("确定要删除项目 [%1] 吗？\n该操作不可恢复，将清空所有相关数据！").arg(name),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        OntologyRepository::deleteOntology(id);
        loadProjects();
    }
}

void ProjectSelectionDialog::onOpenProject() {
    QListWidgetItem* item = m_projectList->currentItem();
    if (!item) {
        QMessageBox::warning(this, "提示", "请先选择一个项目");
        return;
    }

    m_selectedId = item->data(Qt::UserRole).toInt();
    m_selectedName = item->data(Qt::UserRole + 1).toString();
    accept(); // 关闭对话框并返回 Accepted
}

void ProjectSelectionDialog::onItemDoubleClicked(QListWidgetItem* item) {
    Q_UNUSED(item);
    onOpenProject();
}

int ProjectSelectionDialog::getSelectedOntologyId() const {
    return m_selectedId;
}

QString ProjectSelectionDialog::getSelectedOntologyName() const {
    return m_selectedName;
}