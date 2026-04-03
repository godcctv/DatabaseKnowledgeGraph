#include "ProjectSelectionDialog.h"
#include "../database/OntologyRepository.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QDebug>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation> // 保留用于按钮呼吸效果

ProjectSelectionDialog::ProjectSelectionDialog(QWidget *parent)
    : QDialog(parent), m_selectedId(-1)
{
    // 1. 基础窗口设置
    setWindowTitle("选择知识库项目");
    resize(800, 500);

    // 2. 初始化 UI 和 样式
    setupUI();

    // 3. 加载数据
    loadProjects();
}

void ProjectSelectionDialog::setupUI() {
    // 核心配色：深空蓝背景，亮蓝/青色高亮，白色/浅蓝文本
    setStyleSheet(R"(
    QDialog { background-color: #2E3440; color: #D8DEE9; font-family: "Microsoft YaHei", sans-serif; }
    QLabel#TitleLabel {
        font-size: 26px; font-weight: bold; color: #88C0D0; margin-bottom: 25px;
        border-bottom: 2px solid #4C566A; letter-spacing: 2px; padding-bottom: 10px;
    }
    QListWidget {
        background-color: #3B4252; border: 1px solid #4C566A; border-radius: 4px;
        color: #D8DEE9; font-size: 15px; padding: 8px; outline: none;
    }
    QListWidget::item { height: 40px; margin: 2px 0; border-radius: 4px; padding-left: 10px; }
    QListWidget::item:hover { background-color: #434C5E; }
    QListWidget::item:selected { background-color: #4C566A; border-left: 4px solid #88C0D0; color: #ECEFF4; }
    QPushButton {
        background-color: #4C566A; color: #ECEFF4; border: 1px solid #434C5E;
        padding: 10px 24px; font-size: 14px; border-radius: 4px;
    }
    QPushButton:hover { background-color: #5E81AC; }
    QPushButton:pressed { background-color: #81A1C1; }
    QPushButton#BtnPrimary { background-color: #5E81AC; border: 1px solid #81A1C1; }
    QPushButton#BtnPrimary:hover { background-color: #81A1C1; }
    QPushButton#BtnDanger { color: #BF616A; background-color: transparent; border: 1px solid #BF616A; }
    QPushButton#BtnDanger:hover { background-color: #BF616A; color: #ECEFF4; }
)");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(25);
    mainLayout->setContentsMargins(50, 50, 50, 50);

    // 标题：知识图谱项目
    QLabel* title = new QLabel("知 识 图 谱 项 目", this);
    title->setObjectName("TitleLabel");
    title->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(title);

    // 项目列表
    m_projectList = new QListWidget(this);

    mainLayout->addWidget(m_projectList);

    // 按钮区域
    QHBoxLayout* btnLayout = new QHBoxLayout();

    m_btnCreate = new QPushButton("新建项目", this);
    m_btnCreate->setToolTip("创建一个新的知识图谱项目");

    m_btnDelete = new QPushButton("删除项目", this);
    m_btnDelete->setObjectName("BtnDanger");

    m_btnOpen = new QPushButton("打开项目", this);
    m_btnOpen->setObjectName("BtnPrimary");
    m_btnOpen->setDefault(true);

    btnLayout->addWidget(m_btnCreate);
    btnLayout->addWidget(m_btnDelete);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnOpen);

    mainLayout->addLayout(btnLayout);

    // 连接信号槽
    connect(m_btnCreate, &QPushButton::clicked, this, &ProjectSelectionDialog::onCreateProject);
    connect(m_btnDelete, &QPushButton::clicked, this, &ProjectSelectionDialog::onDeleteProject);
    connect(m_btnOpen, &QPushButton::clicked, this, &ProjectSelectionDialog::onOpenProject);
    connect(m_projectList, &QListWidget::itemDoubleClicked, this, &ProjectSelectionDialog::onItemDoubleClicked);
}

// --- 业务逻辑 ---

void ProjectSelectionDialog::loadProjects() {
    m_projectList->clear();
    QList<Ontology> ontologies = OntologyRepository::getAllOntologies();

    for (const auto& onto : ontologies) {
        QListWidgetItem* item = new QListWidgetItem(m_projectList);
        // 使用更符合科幻主题的 Unicode 图标
        item->setText(QString("🌌 %1  [v%2]").arg(onto.name).arg(onto.version));
        item->setData(Qt::UserRole, onto.id);
        item->setData(Qt::UserRole + 1, onto.name);
    }
}

void ProjectSelectionDialog::onCreateProject() {
    // 文案更新为更专业的说法
    QString name = QInputDialog::getText(this, "新建项目", "请输入项目名称：");
    if (name.trimmed().isEmpty()) return;

    QString desc = QInputDialog::getText(this, "描述", "项目描述（可选）：");

    if (OntologyRepository::addOntology(name, desc)) {
        loadProjects();
    } else {
        // 移除了 triggerShake()
        QMessageBox::warning(this, "创建失败", "无法创建项目。\n该名称可能已存在。");
    }
}

void ProjectSelectionDialog::onDeleteProject() {
    QListWidgetItem* item = m_projectList->currentItem();
    if (!item) return;

    int id = item->data(Qt::UserRole).toInt();
    QString name = item->data(Qt::UserRole + 1).toString();

    // 文案更新
    auto reply = QMessageBox::question(this, "确认删除",
        QString("确定要删除项目 [%1] 吗？\n此操作无法撤销。").arg(name),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // 移除了 triggerShake()
        OntologyRepository::deleteOntology(id);
        loadProjects();
    }
}

void ProjectSelectionDialog::onOpenProject() {
    QListWidgetItem* item = m_projectList->currentItem();
    if (!item) {
        // 移除了 triggerShake()
        QMessageBox::warning(this, "提示", "请先选择一个项目。");
        return;
    }

    m_selectedId = item->data(Qt::UserRole).toInt();
    m_selectedName = item->data(Qt::UserRole + 1).toString();
    accept();
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