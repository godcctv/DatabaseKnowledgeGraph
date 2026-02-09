#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "addnodedialog.h"
#include "addedgedialog.h"
#include "VisualNode.h"
#include "VisualEdge.h"
#include "QueryDialog.h"
#include "../business/ForceDirectedLayout.h"  // 确保路径正确
#include "../database/DatabaseConnection.h"
#include "../database/RelationshipRepository.h"
#include "../business/GraphEditor.h"
#include "../business/QueryEngine.h"
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
#include <QToolBar>
#include <QtMath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 1. 初始化后端
    m_graphEditor = new GraphEditor(this);
    m_queryEngine = new QueryEngine(this);
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
    ui->propertyPanel->setColumnWidth(0, 50);

    // 名称列自适应拉伸 (Stretch)
    ui->propertyPanel->header()->setSectionResizeMode(1, QHeaderView::Stretch);

    // 类型列根据内容调整
    ui->propertyPanel->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    // 禁止点击表头排序（
    ui->propertyPanel->header()->setSectionsClickable(false);

    // 设置右侧属性面板的最小宽度，死守底线，防止被挤压
    ui->propertyPanel->setMinimumWidth(240);

    // 设置初始比例：左边 3 份，右边 1 份
    ui->splitter->setStretchFactor(0, 4);
    ui->splitter->setStretchFactor(1, 1);

    // --- 初始化力导向布局 ---
    m_layout = new ForceDirectedLayout(this);

    // 初始化并启动定时器
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, m_layout, &ForceDirectedLayout::calculate);
    m_timer->start(30); // 30ms 刷新一次

    // 4. 建立连接
    setupConnections();
    setupToolbar();
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
    onQueryFullGraph();
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
    if (m_timer->isActive()) { // 只有在全图动态模式下才自动添加显示
        drawNode(node.id, node.name, node.nodeType, node.posX, node.posY);
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

void MainWindow::setupToolbar() {
    QToolBar* toolbar = this->findChild<QToolBar*>("QueryToolbar");// 使用设计器里已有的，或者 addToolBar
    if (!toolbar) toolbar = addToolBar("Query");

    toolbar->addSeparator();

    QAction* actFull = toolbar->addAction("全图");
    connect(actFull, &QAction::triggered, this, &MainWindow::onQueryFullGraph);

    QAction* actNode = toolbar->addAction("单节点");
    actNode->setToolTip("先选中一个节点，然后点击此按钮查看关联");
    connect(actNode, &QAction::triggered, this, &MainWindow::onQuerySingleNode);

    QAction* actAttr = toolbar->addAction("属性查询");
    connect(actAttr, &QAction::triggered, this, &MainWindow::onQueryAttribute);

    QAction* actPath = toolbar->addAction("路径查询");
    actPath->setToolTip("先选中两个节点，然后点击此按钮");
    connect(actPath, &QAction::triggered, this, &MainWindow::onQueryPath);
}

// --- 1. 全图查询  ---
void MainWindow::onQueryFullGraph() {
    // 1. 获取数据
    QList<GraphNode> nodes = m_queryEngine->getAllNodes(1);
    QList<GraphEdge> edges = m_queryEngine->getAllRelationships(1);

    // 2. 清空视图
    m_scene->clear();
    m_layout->clear();
    ui->propertyPanel->clear();

    // 3. 添加所有节点和边
    for (const auto& node : nodes) {
        // 全图模式：随机位置，让力导向算法去跑
        drawNode(node.id, node.name, node.nodeType, rand() % 800 - 400, rand() % 600 - 300);
        // 同时更新列表
        QTreeWidgetItem *item = new QTreeWidgetItem(ui->propertyPanel);
        item->setText(0, QString::number(node.id));
        item->setText(1, node.name);
        item->setText(2, node.nodeType);
    }
    for (const auto& edge : edges) {
        // 需要查找指针来构建 VisualEdge，这里复用 drawEdge 逻辑需要先拿到 VisualNode
        // 简单起见，我们重新实现这部分逻辑
        VisualEdge* vEdge = nullptr;
        // 查找 source 和 target
        VisualNode* src = nullptr;
        VisualNode* dst = nullptr;
        foreach(QGraphicsItem* item, m_scene->items()) {
            if (item->type() == VisualNode::Type) {
                VisualNode* vn = qgraphicsitem_cast<VisualNode*>(item);
                if (vn->getId() == edge.sourceId) src = vn;
                if (vn->getId() == edge.targetId) dst = vn;
            }
        }
        if (src && dst) {
            vEdge = new VisualEdge(edge.id, edge.sourceId, edge.targetId, edge.relationType, src, dst);
            m_scene->addItem(vEdge);
            m_layout->addEdge(vEdge);
            src->addEdge(vEdge, true);
            dst->addEdge(vEdge, false);
        }
    }

    // 4. 开启布局算法
    m_timer->start(30);
    ui->statusbar->showMessage(QString("全图模式：已加载 %1 个节点").arg(nodes.size()));
}

// --- 2. 单节点查询 ---
void MainWindow::onQuerySingleNode() {
    // 获取当前选中节点
    QList<QGraphicsItem*> selected = m_scene->selectedItems();
    int centerId = -1;
    for (auto item : selected) {
        if (item->type() == VisualNode::Type) {
            centerId = qgraphicsitem_cast<VisualNode*>(item)->getId();
            break;
        }
    }

    if (centerId == -1) {
        QMessageBox::warning(this, "提示", "请先在图中选中一个节点！");
        return;
    }

    // 1. 查询数据
    GraphNode centerNode = m_queryEngine->getNodeById(centerId);
    QList<GraphEdge> relatedEdges = m_queryEngine->getRelatedRelationships(centerId);

    // 2. 暂停力导向 (静态布局)
    m_timer->stop();
    m_scene->clear();
    m_layout->clear(); // 清空算法中的数据引用

    // 3. 绘制中心节点
    drawNode(centerNode.id, centerNode.name, centerNode.nodeType, 0, 0);

    // 4. 绘制周围节点 (圆形布局)
    QSet<int> addedNodes;
    addedNodes.insert(centerId);

    int count = relatedEdges.size(); // 实际上可能是边数，这里近似处理
    double radius = 200.0;
    double angleStep = (2 * M_PI) / (count > 0 ? count : 1);
    int currentIdx = 0;

    for (const auto& edge : relatedEdges) {
        int neighborId = (edge.sourceId == centerId) ? edge.targetId : edge.sourceId;

        if (!addedNodes.contains(neighborId)) {
            GraphNode neighbor = m_queryEngine->getNodeById(neighborId);

            // 计算坐标
            double x = radius * cos(currentIdx * angleStep);
            double y = radius * sin(currentIdx * angleStep);

            drawNode(neighbor.id, neighbor.name, neighbor.nodeType, x, y);
            addedNodes.insert(neighborId);

            // 手动画边 (不需要加入 m_layout)
            // 这里为了简单，需重新查找 VisualNode 指针
            // 实际项目中可以优化
            currentIdx++;
        }
    }

    // 重新遍历连接边
    foreach(QGraphicsItem* item, m_scene->items()) {
        if (item->type() == VisualNode::Type) {
            VisualNode* vn = qgraphicsitem_cast<VisualNode*>(item);
            if (vn->getId() != centerId) {
                // 连接中心和它
                VisualNode* centerV = nullptr;
                // 找中心节点指针
                foreach(QGraphicsItem* it, m_scene->items()) {
                     if (it->type() == VisualNode::Type && qgraphicsitem_cast<VisualNode*>(it)->getId() == centerId) {
                         centerV = qgraphicsitem_cast<VisualNode*>(it);
                         break;
                     }
                }
                if (centerV) {
                    VisualEdge* edge = new VisualEdge(-1, centerId, vn->getId(), "related", centerV, vn);
                    m_scene->addItem(edge);
                    centerV->addEdge(edge, true);
                    vn->addEdge(edge, false);
                }
            }
        }
    }

    ui->graphicsView->centerOn(0, 0);
    ui->statusbar->showMessage(QString("单节点查询：ID %1").arg(centerId));
}

// --- 3. 属性查询 ---
void MainWindow::onQueryAttribute() {
    QueryDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString name = dialog.getAttrName();
        QString value = dialog.getAttrValue();

        QList<GraphNode> results = m_queryEngine->queryByAttribute(name, value);

        if (results.isEmpty()) {
            QMessageBox::information(this, "结果", "未找到匹配节点");
            return;
        }

        // 停止布局，清空视图，只显示结果
        m_timer->stop();
        m_scene->clear();
        m_layout->clear();

        // 网格布局展示结果
        int col = 0;
        int row = 0;
        int gap = 150;
        int maxCols = qCeil(qSqrt(results.size()));

        for (const auto& node : results) {
            drawNode(node.id, node.name, node.nodeType, col * gap, row * gap);
            col++;
            if (col >= maxCols) {
                col = 0;
                row++;
            }
        }
        ui->statusbar->showMessage(QString("属性查询：找到 %1 个结果").arg(results.size()));
    }
}

// --- 4. 路径查询 ---
void MainWindow::onQueryPath() {
    QList<QGraphicsItem*> selected = m_scene->selectedItems();
    QList<VisualNode*> nodes;
    for (auto item : selected) {
        if (item->type() == VisualNode::Type) {
            nodes.append(qgraphicsitem_cast<VisualNode*>(item));
        }
    }

    if (nodes.size() != 2) {
        QMessageBox::warning(this, "提示", "请先选中两个节点（起点和终点）");
        return;
    }

    int startId = nodes[0]->getId();
    int endId = nodes[1]->getId();

    QList<int> pathNodes = m_queryEngine->findPath(startId, endId);

    if (pathNodes.isEmpty()) {
        QMessageBox::information(this, "结果", "无路径连接");
        return;
    }

    // 停止布局，清空
    m_timer->stop();
    m_scene->clear();
    m_layout->clear();

    // 线性布局绘制路径
    int x = 0;
    VisualNode* prevVNode = nullptr;

    for (int nodeId : pathNodes) {
        GraphNode node = m_queryEngine->getNodeById(nodeId);
        // 调用 drawNode 创建 VisualNode
        drawNode(node.id, node.name, node.nodeType, x, 0);

        // 获取刚刚创建的 VisualNode (为了连线)
        VisualNode* currVNode = nullptr;
        foreach(QGraphicsItem* item, m_scene->items()) {
            VisualNode* vn = qgraphicsitem_cast<VisualNode*>(item);
            if (vn && vn->getId() == nodeId) {
                currVNode = vn;
                break;
            }
        }

        if (prevVNode && currVNode) {
            // 连线
            VisualEdge* edge = new VisualEdge(-1, prevVNode->getId(), currVNode->getId(), "path", prevVNode, currVNode);
            m_scene->addItem(edge);
            prevVNode->addEdge(edge, true);
            currVNode->addEdge(edge, false);
        }

        prevVNode = currVNode;
        x += 200; // 间距
    }

    ui->statusbar->showMessage("路径查询完成");
    ui->graphicsView->centerOn(x/2, 0);
}

// 辅助绘图函数
void MainWindow::drawNode(int id, QString name, QString type, double x, double y) {
    VisualNode *vNode = new VisualNode(id, name, type, x, y);
    m_scene->addItem(vNode);
    // 只有在全图模式下才加入 m_layout，静态模式不需要
    if (m_timer->isActive()) {
        m_layout->addNode(vNode);
    }
}