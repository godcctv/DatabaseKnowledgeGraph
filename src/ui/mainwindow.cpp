#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "addnodedialog.h"
#include "addedgedialog.h"
#include "VisualNode.h"
#include "VisualEdge.h"
#include "../business/ForceDirectedLayout.h"  // 确保路径正确
#include "../database/DatabaseConnection.h"
#include "../database/RelationshipRepository.h"
#include "../business/GraphEditor.h"         // 确保包含 GraphEditor 定义

#include <QGraphicsTextItem>
#include <QCoreApplication>
#include <QDebug>
#include <QMessageBox>
#include <QRadialGradient>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsLineItem>
#include <QHeaderView>
#include <QTimer>
#include <QWheelEvent>
#include <QtMath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 1. 初始化后端
    m_graphEditor = new GraphEditor(this);

    // 2. 初始化可视化场景
    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(-5000, -5000, 10000, 10000);
    ui->graphicsView->setScene(m_scene);

    // 优化渲染质量
    ui->graphicsView->setRenderHint(QPainter::Antialiasing);
    ui->graphicsView->setRenderHint(QPainter::TextAntialiasing);
    ui->graphicsView->setRenderHint(QPainter::SmoothPixmapTransform);
    ui->graphicsView->setBackgroundBrush(QColor("#1e1e1e"));
    ui->graphicsView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    // 允许鼠标拖拽画布（像地图一样平移）
    ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
    // 隐藏滚动条（可选，看起来更简洁）
    ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // 设置缩放锚点为鼠标位置（缩放时以鼠标为中心，而不是画布中心）
    ui->graphicsView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    ui->graphicsView->setResizeAnchor(QGraphicsView::AnchorViewCenter);

    ui->graphicsView->viewport()->installEventFilter(this);

    // 3. 初始化属性面板列头
    ui->propertyPanel->setHeaderLabels(QStringList() << "ID" << "名称" << "类型");
    ui->propertyPanel->setColumnCount(3);
    ui->propertyPanel->header()->setSectionResizeMode(0, QHeaderView::Fixed);
    ui->propertyPanel->setColumnWidth(0, 40);
    ui->propertyPanel->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->propertyPanel->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->propertyPanel->header()->setSectionsClickable(false);

    // 设置分割器比例
    ui->splitter->setStretchFactor(0, 7); // 画布占 70%
    ui->splitter->setStretchFactor(1, 3); // 属性栏占 30%

    // --- 初始化力导向布局 ---
    m_layout = new ForceDirectedLayout(this);

    // 初始化并启动定时器
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, m_layout, &ForceDirectedLayout::calculate);
    m_timer->start(30); // 30ms 刷新一次

    // 4. 建立连接
    setupConnections();
    updateStatusBar();
    ui->graphicsView->centerOn(0, 0);
    // 5. 加载数据
    if (DatabaseConnection::isConnected()) {
        loadInitialData();
    }
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::loadInitialData() {
    ui->statusbar->showMessage("正在从数据库加载图谱数据...");
    QCoreApplication::processEvents();

    QList<GraphNode> nodes = NodeRepository::getAllNodes(1);
    for (const auto& node : nodes) {
        onNodeAdded(node);
    }

    QList<GraphEdge> edges = RelationshipRepository::getAllRelationships(1);
    for (const auto& edge : edges) {
        onRelationshipAdded(edge);
    }

    ui->statusbar->showMessage(QString("已加载 %1 个节点").arg(nodes.size()));
}

void MainWindow::setupConnections() {
    connect(ui->actionAddNode, &QAction::triggered, this, &MainWindow::onActionAddNodeTriggered);
    connect(ui->actionDelete, &QAction::triggered, this, &MainWindow::onActionDeleteTriggered);

    // 节点相关信号
    connect(m_graphEditor, &GraphEditor::nodeAdded, this, &MainWindow::onNodeAdded);
    connect(m_graphEditor, &GraphEditor::graphChanged, this, &MainWindow::onGraphChanged);
    connect(m_graphEditor, &GraphEditor::nodeDeleted, this, &MainWindow::onNodeDeleted);

    // 关系相关信号
    connect(ui->actionAddRelation, &QAction::triggered, this, &MainWindow::onActionAddRelationshipTriggered);
    connect(m_graphEditor, &GraphEditor::relationshipAdded, this, &MainWindow::onRelationshipAdded);
    connect(m_graphEditor, &GraphEditor::relationshipDeleted, this, &MainWindow::onRelationshipDeleted);
}

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

void MainWindow::onActionDeleteTriggered() {
    QList<QTreeWidgetItem*> selectedItems = ui->propertyPanel->selectedItems();
    if (selectedItems.isEmpty()) {
        ui->statusbar->showMessage("请先在右侧列表中选中一个节点", 2000);
        return;
    }

    int nodeId = selectedItems.first()->text(0).toInt();
    ui->statusbar->showMessage(QString("正在请求数据库删除节点 %1...").arg(nodeId), 0);
    QCoreApplication::processEvents();

    qDebug() << ">>> [UI] 准备调用后端删除接口, NodeID:" << nodeId;
    bool success = m_graphEditor->deleteNode(nodeId);
    qDebug() << ">>> [UI] 后端返回结果:" << success;

    if (!success) {
        ui->statusbar->showMessage("删除失败！可能是数据库繁忙或连接中断。", 5000);
        QMessageBox::critical(this, "删除失败", "无法删除节点，请检查数据库连接或控制台日志。");
    }
}

void MainWindow::onNodeAdded(const GraphNode& node) {
    if (!m_scene) return;

    // 创建可视化节点
    VisualNode *visualNode = new VisualNode(node.id, node.name, node.nodeType, node.posX, node.posY);
    m_scene->addItem(visualNode);

    // 🔥 将新节点加入力导向布局算法管理 🔥
    if (m_layout) {
        m_layout->addNode(visualNode);
    }

    if (ui->statusbar) {
        ui->statusbar->showMessage(QString("节点 %1 加载成功").arg(node.name), 3000);
    }

    if (ui && ui->propertyPanel) {
        QTreeWidgetItem *item = new QTreeWidgetItem(ui->propertyPanel);
        item->setText(0, QString::number(node.id));
        item->setText(1, node.name);
        item->setText(2, node.nodeType);
    }
}

void MainWindow::onNodeDeleted(int nodeId) {
    // 1. 删除右侧列表项
    for (int i = ui->propertyPanel->topLevelItemCount() - 1; i >= 0; --i) {
        QTreeWidgetItem *item = ui->propertyPanel->topLevelItem(i);
        if (item->text(0).toInt() == nodeId) {
            delete ui->propertyPanel->takeTopLevelItem(i);
            break;
        }
    }

    // 2. 删除绘图场景项
    QList<QGraphicsItem*> itemsToDelete;
    foreach (QGraphicsItem *item, m_scene->items()) {
        // 使用 data(0) 或者类型判断
        if (item->type() == VisualNode::Type && item->data(0).toInt() == nodeId) {
            itemsToDelete.append(item);
        }
    }

    for (QGraphicsItem *item : itemsToDelete) {
        VisualNode* vNode = qgraphicsitem_cast<VisualNode*>(item);
        if (vNode && m_layout) {
            m_layout->removeNode(vNode);
        }
        m_scene->removeItem(item);
        delete item;
    }

    ui->statusbar->showMessage(QString("节点 ID %1 已删除").arg(nodeId), 3000);
}

void MainWindow::onGraphChanged() {
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
    QList<QGraphicsItem*> selected = m_scene->selectedItems();
    QList<VisualNode*> nodes;
    for (auto item : selected) {
        if (item->type() == VisualNode::Type) {
            nodes.append(qgraphicsitem_cast<VisualNode*>(item));
        }
    }

    if (nodes.size() != 2) {
        QMessageBox::warning(this, "提示", "请按住 Ctrl 键在画布中选中【两个】节点，然后再点击添加关系！");
        return;
    }

    int id1 = nodes[0]->getId();
    int id2 = nodes[1]->getId();

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

    // 创建新边
    VisualEdge *visualEdge = new VisualEdge(edge.id, edge.sourceId, edge.targetId, edge.relationType, sourceNode, targetNode);

    // 计算弯曲偏移量
    int sameConnectionCount = 0;
    foreach(QGraphicsItem* item, m_scene->items()) {
        if (item->type() == VisualEdge::Type) {
            VisualEdge* existing = qgraphicsitem_cast<VisualEdge*>(item);
            bool isSamePair = (existing->getSourceNode() == sourceNode && existing->getDestNode() == targetNode) ||
                              (existing->getSourceNode() == targetNode && existing->getDestNode() == sourceNode);
            if (isSamePair) {
                sameConnectionCount++;
            }
        }
    }

    if (sameConnectionCount > 0) {
        int direction = (sameConnectionCount % 2 == 0) ? -1 : 1;
        int magnitude = ((sameConnectionCount + 1) / 2) * 40;
        visualEdge->setOffset(direction * magnitude);
    }

    m_scene->addItem(visualEdge);

    // 🔥 将新边加入力导向布局算法 🔥
    if (m_layout) {
        m_layout->addEdge(visualEdge);
    }

    sourceNode->addEdge(visualEdge, true);
    targetNode->addEdge(visualEdge, false);
}

void MainWindow::onActionDeleteRelationshipTriggered() {
    QList<QGraphicsItem*> selected = m_scene->selectedItems();
    if (selected.isEmpty()) return;

    int deletedCount = 0;
    for (auto item : selected) {
        if (item->type() == VisualEdge::Type) {
            VisualEdge* edge = qgraphicsitem_cast<VisualEdge*>(item);
            int edgeId = edge->getId();
            if (m_graphEditor->deleteRelationship(edgeId)) {
                deletedCount++;
            }
        }
    }

    if (deletedCount == 0) {
        ui->statusbar->showMessage("请先选中一条连线（变红）再点击删除", 3000);
    }
}

void MainWindow::onRelationshipDeleted(int edgeId) {
    foreach (QGraphicsItem *item, m_scene->items()) {
        if (item->type() == VisualEdge::Type) {
            VisualEdge* edge = qgraphicsitem_cast<VisualEdge*>(item);
            if (edge->getId() == edgeId) {
                VisualNode* src = edge->getSourceNode();
                VisualNode* dst = edge->getDestNode();

                if (src) src->removeEdge(edge);
                if (dst) dst->removeEdge(edge);

                // 🔥 从算法中移除 🔥
                if (m_layout) {
                    m_layout->removeEdge(edge);
                }

                m_scene->removeItem(edge);
                delete edge;
                break;
            }
        }
    }
    ui->statusbar->showMessage("关系已删除", 3000);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    // 拦截 GraphicsView 视口的滚轮事件
    if (obj == ui->graphicsView->viewport() && event->type() == QEvent::Wheel) {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);

        // 检查是否按住了 Ctrl 键
        if (wheelEvent->modifiers() & Qt::ControlModifier) {
            const double scaleFactor = 1.15; // 缩放倍率
            if (wheelEvent->angleDelta().y() > 0) {
                // 向上滚：放大
                ui->graphicsView->scale(scaleFactor, scaleFactor);
            } else {
                // 向下滚：缩小
                ui->graphicsView->scale(1.0 / scaleFactor, 1.0 / scaleFactor);
            }
            // 返回 true 表示事件已被处理，不再传递给默认的滚动条逻辑
            return true;
        }
    }
    // 其他事件交给父类处理
    return QMainWindow::eventFilter(obj, event);
}