#include "ProjectSelectionDialog.h"
#include "../database/OntologyRepository.h"
#include "../database/DatabaseConnection.h"
#include "../database/NodeRepository.h"
#include "../database/RelationshipRepository.h"
#include "../model/GraphNode.h"
#include "../model/GraphEdge.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QDebug>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSqlQuery>

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
    QLabel* title = new QLabel("图 谱 项 目", this);
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

    m_btnImport = new QPushButton("导入项目", this);
    m_btnExport = new QPushButton("导出项目", this);

    m_btnOpen = new QPushButton("打开项目", this);
    m_btnOpen->setObjectName("BtnPrimary");
    m_btnOpen->setDefault(true);

    btnLayout->addWidget(m_btnCreate);
    btnLayout->addWidget(m_btnImport);
    btnLayout->addWidget(m_btnExport);
    btnLayout->addWidget(m_btnDelete);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnOpen);

    mainLayout->addLayout(btnLayout);

    // 连接信号槽
    connect(m_btnCreate, &QPushButton::clicked, this, &ProjectSelectionDialog::onCreateProject);
    connect(m_btnDelete, &QPushButton::clicked, this, &ProjectSelectionDialog::onDeleteProject);
    connect(m_btnOpen, &QPushButton::clicked, this, &ProjectSelectionDialog::onOpenProject);
    connect(m_projectList, &QListWidget::itemDoubleClicked, this, &ProjectSelectionDialog::onItemDoubleClicked);
    connect(m_btnImport, &QPushButton::clicked, this, &ProjectSelectionDialog::onImportProject);
    connect(m_btnExport, &QPushButton::clicked, this, &ProjectSelectionDialog::onExportProject);
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

void ProjectSelectionDialog::onExportProject() {
    QListWidgetItem* item = m_projectList->currentItem();
    if (!item) {
        QMessageBox::warning(this, "提示", "请先在列表中选中一个要导出的项目！");
        return;
    }

    int projectId = item->data(Qt::UserRole).toInt();
    QString projectName = item->data(Qt::UserRole + 1).toString();

    QString fileName = QFileDialog::getSaveFileName(this, "导出项目", projectName + "_导出.json", "JSON 文件 (*.json)");
    if (fileName.isEmpty()) return;

    // 获取项目信息和图谱数据
    Ontology onto = OntologyRepository::getOntologyById(projectId);
    QList<GraphNode> nodes = NodeRepository::getAllNodes(projectId);
    QList<GraphEdge> edges = RelationshipRepository::getAllRelationships(projectId);

    QJsonObject rootObj;

    // 1. 序列化项目元数据
    QJsonObject projectObj;
    projectObj["name"] = onto.name;
    projectObj["description"] = onto.description;
    projectObj["version"] = onto.version;
    rootObj["project"] = projectObj;

    // 2. 序列化节点
    QJsonArray nodesArray;
    for (const auto& node : nodes) {
        QJsonObject nodeObj;
        nodeObj["id"] = node.id;
        nodeObj["name"] = node.name;
        nodeObj["nodeType"] = node.nodeType;
        nodeObj["description"] = node.description;
        nodeObj["posX"] = node.posX;
        nodeObj["posY"] = node.posY;
        nodeObj["color"] = node.color;
        nodeObj["properties"] = node.properties;
        nodesArray.append(nodeObj);
    }
    rootObj["nodes"] = nodesArray;

    // 3. 序列化连线
    QJsonArray edgesArray;
    for (const auto& edge : edges) {
        QJsonObject edgeObj;
        edgeObj["sourceId"] = edge.sourceId;
        edgeObj["targetId"] = edge.targetId;
        edgeObj["relationType"] = edge.relationType;
        edgeObj["weight"] = edge.weight;
        edgeObj["properties"] = edge.properties;
        edgesArray.append(edgeObj);
    }
    rootObj["edges"] = edgesArray;

    // 写入文件
    QJsonDocument doc(rootObj);
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        QMessageBox::information(this, "导出成功",
            QString("项目 [%1] 导出成功！\n共包含 %2 个实体，%3 条关系。")
            .arg(projectName).arg(nodes.size()).arg(edges.size()));
    } else {
        QMessageBox::warning(this, "错误", "无法保存文件，请检查目录权限！");
    }
}

void ProjectSelectionDialog::onImportProject() {
    QString fileName = QFileDialog::getOpenFileName(this, "导入项目", "", "JSON 文件 (*.json)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "错误", "无法读取文件！");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        QMessageBox::warning(this, "错误", "无效的 JSON 格式文件！");
        return;
    }

    QJsonObject rootObj = doc.object();
    QJsonObject projectObj = rootObj["project"].toObject();
    QJsonArray nodesArray = rootObj["nodes"].toArray();
    QJsonArray edgesArray = rootObj["edges"].toArray();

    QString baseName = projectObj["name"].toString("导入的图谱项目");
    QString desc = projectObj["description"].toString("从外部 JSON 文件导入");

    // --- 防重名处理：如果项目名称已存在，自动添加后缀 ---
    QString finalName = baseName;
    int counter = 1;
    QList<Ontology> existingOntos = OntologyRepository::getAllOntologies();
    auto nameExists = [&](const QString& name) {
        for (const auto& o : existingOntos) {
            if (o.name == name) return true;
        }
        return false;
    };
    while (nameExists(finalName)) {
        finalName = baseName + "_" + QString::number(counter++);
    }

    // 1. 在数据库中创建一个全新的项目
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);
    query.prepare("INSERT INTO ontology (name, description) VALUES (:name, :desc)");
    query.bindValue(":name", finalName);
    query.bindValue(":desc", desc);

    if (!query.exec()) {
        QMessageBox::warning(this, "错误", "在数据库中创建项目失败！");
        return;
    }

    // 拿到新创建的项目的专属 ID
    int newProjectId = query.lastInsertId().toInt();

    // 2. 导入节点和连线 (包含关键的 ID 映射)
    QMap<int, int> idMapping;
    int importedNodes = 0;
    int importedEdges = 0;

    for (int i = 0; i < nodesArray.size(); ++i) {
        QJsonObject nodeObj = nodesArray[i].toObject();
        GraphNode node;
        node.ontologyId = newProjectId; // 强行挂载到刚创建的全新项目下
        node.name = nodeObj["name"].toString();
        node.nodeType = nodeObj["nodeType"].toString();
        node.description = nodeObj["description"].toString();
        node.posX = nodeObj["posX"].toDouble(0);
        node.posY = nodeObj["posY"].toDouble(0);
        node.color = nodeObj["color"].toString("#3498db");
        node.properties = nodeObj["properties"].toObject();

        int oldId = nodeObj["id"].toInt();

        // 直接调用底层 Repository 插入（这里不在画板中，不需要走 GraphEditor 的撤销栈）
        if (NodeRepository::addNode(node)) {
            idMapping[oldId] = node.id;
            importedNodes++;
        }
    }

    for (int i = 0; i < edgesArray.size(); ++i) {
        QJsonObject edgeObj = edgesArray[i].toObject();
        int oldSourceId = edgeObj["sourceId"].toInt();
        int oldTargetId = edgeObj["targetId"].toInt();

        if (idMapping.contains(oldSourceId) && idMapping.contains(oldTargetId)) {
            GraphEdge edge;
            edge.ontologyId = newProjectId; // 强行挂载到刚创建的全新项目下
            edge.sourceId = idMapping[oldSourceId];
            edge.targetId = idMapping[oldTargetId];
            edge.relationType = edgeObj["relationType"].toString();
            edge.weight = edgeObj["weight"].toDouble(1.0);
            edge.properties = edgeObj["properties"].toObject();

            if (RelationshipRepository::addRelationship(edge)) {
                importedEdges++;
            }
        }
    }

    // 3. 刷新项目列表并提示
    loadProjects();
    QMessageBox::information(this, "导入完成",
        QString("成功导入为新项目 [%1]！\n包含 %2 个节点，%3 条连线。").arg(finalName).arg(importedNodes).arg(importedEdges));
}