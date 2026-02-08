#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "addnodedialog.h"
#include "addedgedialog.h"
#include "VisualNode.h"
#include "VisualEdge.h"
#include"../business/ForceDirectedLayout.h"
#include "../database/DatabaseConnection.h"
#include "../database/RelationshipRepository.h"
#include <QGraphicsTextItem>
#include <QCoreApplication>
#include <QDebug>
#include <QMessageBox>
#include <QRadialGradient>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsLineItem>
#include <QHeaderView>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 1. 初始化后端
    m_graphEditor = new GraphEditor(this);

    // 2. 初始化可视化场景 (为第5周做准备，防止崩溃)
    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(-2000, -2000, 4000, 4000);
    ui->graphicsView->setScene(m_scene);

    ui->graphicsView->setRenderHint(QPainter::Antialiasing);           // 图元抗锯齿
    ui->graphicsView->setRenderHint(QPainter::TextAntialiasing);       // 文字抗锯齿
    ui->graphicsView->setRenderHint(QPainter::SmoothPixmapTransform);  // 图片平滑
    ui->graphicsView->setBackgroundBrush(QColor("#1e1e1e"));           // 强制背景色，防止闪烁
    ui->graphicsView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate); // 避免残留重影

    // 3. 初始化属性面板列头
    ui->propertyPanel->setHeaderLabels(QStringList() << "ID" << "名称" << "类型");

    // 设置列数 (确保有3列)
    ui->propertyPanel->setColumnCount(3);

    // 0列 (ID): 固定宽度 40像素，足够显示 1-3 位数字
    ui->propertyPanel->header()->setSectionResizeMode(0, QHeaderView::Fixed);
    ui->propertyPanel->setColumnWidth(0, 40);

    // 1列 (名称): 自动拉伸 (Stretch)，占据剩余所有空间
    ui->propertyPanel->header()->setSectionResizeMode(1, QHeaderView::Stretch);

    // 2列 (类型): 根据内容调整大小 (ResizeToContents)
    ui->propertyPanel->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    // 去掉表头的点击排序功能（可选，防止误触）
    ui->propertyPanel->header()->setSectionsClickable(false);

    ui->splitter->setStretchFactor(0, 7);
    ui->splitter->setStretchFactor(1, 2);

    // 初始化布局算法
    m_layout = new ForceDirectedLayout(this);

    // 初始化并启动定时器
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, m_layout, &ForceDirectedLayout::calculate);
    m_timer->start(30); // 30ms 刷新一次
    // 4. 建立连接
    setupConnections();
    updateStatusBar();

    if (DatabaseConnection::isConnected()) {
        loadInitialData();
    }
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::loadInitialData() {
    ui->statusbar->showMessage("正在从数据库加载图谱数据...");
    QCoreApplication::processEvents(); // 刷新一下界面防止白屏

    QList<GraphNode> nodes = NodeRepository::getAllNodes(1);
    for (const auto& node : nodes) {
        onNodeAdded(node);
    }
    // 加载关系
    QList<GraphEdge> edges = RelationshipRepository::getAllRelationships(1);
    for (const auto& edge : edges) {
        onRelationshipAdded(edge);
    }

    ui->statusbar->showMessage(QString("已加载 %1 个节点").arg(nodes.size()));
}

void MainWindow::setupConnections() {
    // 连接菜单栏/工具栏的 Action (假设你在UI里把 action_2 改名为了 actionAddNode)
    // 如果没改名，请用 ui->action_2
    connect(ui->actionAddNode, &QAction::triggered, this, &MainWindow::onActionAddNodeTriggered);
    connect(ui->actionDelete, &QAction::triggered, this, &MainWindow::onActionDeleteTriggered);
    // 连接后端信号 -> 前端界面更新
    connect(m_graphEditor, &GraphEditor::nodeAdded, this, &MainWindow::onNodeAdded);
    connect(m_graphEditor, &GraphEditor::graphChanged, this, &MainWindow::onGraphChanged);
    connect(m_graphEditor, &GraphEditor::nodeDeleted, this, &MainWindow::onNodeDeleted);
    //添加关系
    connect(ui->actionAddRelation, &QAction::triggered, this, &MainWindow::onActionAddRelationshipTriggered);
    //连接后端信号
    connect(m_graphEditor, &GraphEditor::relationshipAdded, this, &MainWindow::onRelationshipAdded);
    connect(m_graphEditor, &GraphEditor::relationshipDeleted, this, &MainWindow::onRelationshipDeleted);
}

// 用户点击“添加节点”按钮
void MainWindow::onActionAddNodeTriggered() {
    AddNodeDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        GraphNode newNode = dialog.getNodeData();
        newNode.ontologyId = 1;

        if (!m_graphEditor->addNode(newNode)) {
            QMessageBox::warning(this, "添加失败",
                "无法添加节点！可能是节点名称已存在。\n请尝试更换名称。");
        }
    }
}

// 位于 src/ui/mainwindow.cpp

void MainWindow::onActionDeleteTriggered() {
    // 1. 获取右侧列表选中的项
    QList<QTreeWidgetItem*> selectedItems = ui->propertyPanel->selectedItems();
    if (selectedItems.isEmpty()) {
        ui->statusbar->showMessage("请先在右侧列表中选中一个节点", 2000);
        return;
    }

    // 2. 获取 ID (第0列是 ID)
    int nodeId = selectedItems.first()->text(0).toInt();

    // 3. 【优化】给用户反馈，防止误以为卡死
    ui->statusbar->showMessage(QString("正在请求数据库删除节点 %1...").arg(nodeId), 0);

    // 【关键】强制处理一下界面事件，让“正在删除...”这几个字能显示出来，而不是直接白屏
    QCoreApplication::processEvents();

    qDebug() << ">>> [UI] 准备调用后端删除接口, NodeID:" << nodeId;

    // 4. 调用后端删除
    // 如果数据库被锁住，这行代码可能会阻塞几秒钟
    bool success = m_graphEditor->deleteNode(nodeId);

    qDebug() << ">>> [UI] 后端返回结果:" << success;

    // 5. 根据结果处理
    if (!success) {
        // 如果失败（比如超时或数据库错误），给个红色警告
        ui->statusbar->showMessage("删除失败！可能是数据库繁忙或连接中断。", 5000);
        QMessageBox::critical(this, "删除失败", "无法删除节点，请检查数据库连接或控制台日志。");
    } else {
        // 如果成功，onNodeDeleted 槽函数会被触发，那里会负责清除界面和提示成功
        // 所以这里不需要写“删除成功”的提示，否则会被覆盖
    }
}

// src/ui/mainwindow.cpp

void MainWindow::onNodeAdded(const GraphNode& node) {
    if (!m_scene) return;

    // 直接创建一个智能的 VisualNode，所有的 3D、文字、阴影逻辑都在它里面了
    VisualNode *visualNode = new VisualNode(node.id, node.name, node.nodeType, node.posX, node.posY);

    m_scene->addItem(visualNode);

    // 状态栏提示
    if (ui->statusbar) {
        ui->statusbar->showMessage(QString("节点 %1 加载成功").arg(node.name), 3000);
    }

    // 更新右侧列表 (代码不变)
    if (ui && ui->propertyPanel) {
        QTreeWidgetItem *item = new QTreeWidgetItem(ui->propertyPanel);
        item->setText(0, QString::number(node.id));
        item->setText(1, node.name);
        item->setText(2, node.nodeType);
    }
}

void MainWindow::onNodeDeleted(int nodeId) {
    // --- 1. 安全删除右侧列表项 (倒序遍历) ---
    for (int i = ui->propertyPanel->topLevelItemCount() - 1; i >= 0; --i) {
        QTreeWidgetItem *item = ui->propertyPanel->topLevelItem(i);
        if (item->text(0).toInt() == nodeId) {
            delete ui->propertyPanel->takeTopLevelItem(i);
            break;
        }
    }

    // --- 2. 安全删除绘图场景项 ---
    QList<QGraphicsItem*> itemsToDelete;
    foreach (QGraphicsItem *item, m_scene->items()) {
        if (item->data(0).toInt() == nodeId) {
            itemsToDelete.append(item);
        }
    }

    for (QGraphicsItem *item : itemsToDelete) {
        m_scene->removeItem(item); // 从场景移除
        delete item;
    }

    ui->statusbar->showMessage(QString("节点 ID %1 已删除").arg(nodeId), 3000);
}

void MainWindow::onGraphChanged() {
    // 每次图变动（如撤销/重做）时调用
    // 未来在这里重新刷新整个视图
    qDebug() << "UI: Graph updated";
}

void MainWindow::updateStatusBar() {
    if (DatabaseConnection::isConnected()) {
        ui->statusbar->showMessage("数据库已连接");
    } else {
        ui->statusbar->showMessage("数据库未连接", 0);
    }
}

QGraphicsItem* MainWindow::findItemById(int nodeId) {
    if (!m_scene) return nullptr;
    foreach (QGraphicsItem *item, m_scene->items()) {
        if (item->type() == VisualNode::Type && item->data(0).toInt() == nodeId) {
            return item;
        }
    }
    return nullptr;
}

void MainWindow::onActionAddRelationshipTriggered() {
    // 获取场景中选中的项
    QList<QGraphicsItem*> selected = m_scene->selectedItems();

    QList<VisualNode*> nodes;
    for (auto item : selected) {
        // 检查它是不是 VisualNode，而不是 EllipseItem
        if (item->type() == VisualNode::Type) {
            // 安全转换
            nodes.append(qgraphicsitem_cast<VisualNode*>(item));
        }
    }

    // 2. 校验：必须选中两个节点
    if (nodes.size() != 2) {
        QMessageBox::warning(this, "提示", "请按住 Ctrl 键在画布中选中【两个】节点，然后再点击添加关系！");
        return;
    }

    // 3. 获取 ID (VisualNode 有专门的 getId 方法，比 data(0) 更优雅)
    int id1 = nodes[0]->getId();
    int id2 = nodes[1]->getId();

    // 4. 弹出对话框
    AddEdgeDialog dialog(this);
    dialog.setNodes(QString("节点ID: %1").arg(id1), QString("节点ID: %2").arg(id2));

    if (dialog.exec() == QDialog::Accepted) {
        GraphEdge edge = dialog.getEdgeData();
        edge.ontologyId = 1;
        edge.sourceId = id1;
        edge.targetId = id2;

        if (!m_graphEditor->addRelationship(edge)) {
            QMessageBox::warning(this, "错误", "添加关系失败！可能是关系已存在或方向错误。");
        }
    }
}

void MainWindow::onRelationshipAdded(const GraphEdge& edge) {
    VisualNode* sourceNode = qgraphicsitem_cast<VisualNode*>(findItemById(edge.sourceId));
    VisualNode* targetNode = qgraphicsitem_cast<VisualNode*>(findItemById(edge.targetId));

    if (!sourceNode || !targetNode) return;

    // 1. 创建新边
    VisualEdge *visualEdge = new VisualEdge(edge.id, edge.sourceId, edge.targetId, edge.relationType, sourceNode, targetNode);

    // 🔥🔥🔥 核心修改：计算弯曲偏移量 (Offset) 🔥🔥🔥
    int sameConnectionCount = 0;

    // 遍历场景中所有的线，找找看有没有“老乡”
    foreach(QGraphicsItem* item, m_scene->items()) {
        if (item->type() == VisualEdge::Type) {
            VisualEdge* existing = qgraphicsitem_cast<VisualEdge*>(item);

            // 检查是否是连接同一对节点 (A->B 或 B->A 都算)
            bool isSamePair = (existing->getSourceNode() == sourceNode && existing->getDestNode() == targetNode) ||
                              (existing->getSourceNode() == targetNode && existing->getDestNode() == sourceNode);

            if (isSamePair) {
                sameConnectionCount++;
            }
        }
    }

    if (sameConnectionCount > 0) {
        int direction = (sameConnectionCount % 2 == 0) ? -1 : 1;
        int magnitude = ((sameConnectionCount + 1) / 2) * 40; // 40 是弯曲幅度，可调整
        visualEdge->setOffset(direction * magnitude);
    }

    m_scene->addItem(visualEdge);

    sourceNode->addEdge(visualEdge, true); // true = 我是起点
    targetNode->addEdge(visualEdge, false); // false = 我是终点 (修正：之前这里可能写反了或者没注意，VisualNode内部逻辑要匹配)

}

void MainWindow::onActionDeleteRelationshipTriggered() {
    // 1. 获取选中的项
    QList<QGraphicsItem*> selected = m_scene->selectedItems();
    if (selected.isEmpty()) return;

    // 2. 遍历查找选中的 VisualEdge
    int deletedCount = 0;
    for (auto item : selected) {
        if (item->type() == VisualEdge::Type) {
            VisualEdge* edge = qgraphicsitem_cast<VisualEdge*>(item);
            int edgeId = edge->getId();

            // 3. 调用后端删除
            if (m_graphEditor->deleteRelationship(edgeId)) {
                // UI 移除 (VisualEdge 会被 GraphEditor::relationshipDeleted 信号触发移除，
                // 或者在这里手动移也行，但推荐走信号槽闭环)
                deletedCount++;
            }
        }
    }

    if (deletedCount == 0) {
        ui->statusbar->showMessage("请先选中一条连线（变红）再点击删除", 3000);
    }
}

void MainWindow::onRelationshipDeleted(int edgeId) {
    // 遍历场景找线
    foreach (QGraphicsItem *item, m_scene->items()) {
        if (item->type() == VisualEdge::Type) {
            VisualEdge* edge = qgraphicsitem_cast<VisualEdge*>(item);
            if (edge->getId() == edgeId) {
                VisualNode* src = edge->getSourceNode(); // 你需要在 VisualEdge 加这个 getter
                VisualNode* dst = edge->getDestNode();   // 你需要在 VisualEdge 加这个 getter

                if (src) src->removeEdge(edge);
                if (dst) dst->removeEdge(edge);
                m_scene->removeItem(edge);
                delete edge;
                break; // ID 唯一，删完退出
            }
        }
    }
    ui->statusbar->showMessage("关系已删除", 3000);
}
