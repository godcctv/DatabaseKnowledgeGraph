#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "addnodedialog.h"
#include "addedgedialog.h"
#include "VisualNode.h"
#include "VisualEdge.h"
#include "QueryDialog.h"
#include "../database/OntologyRepository.h"
#include "../database/RelationshipRepository.h"
#include "../business/ForceDirectedLayout.h"  // 确保路径正确
#include "../database/DatabaseConnection.h"
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
#include <QRandomGenerator>
#include <QContextMenuEvent>
#include <QMenu>
#include <QDockWidget>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QSlider>

MainWindow::MainWindow(int ontologyId, QString ontologyName, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_currentOntologyId = ontologyId;
    this->setWindowTitle(QString("知识图谱系统 - 当前项目: %1").arg(ontologyName));
    // 1. 初始化后端
    m_graphEditor = new GraphEditor(this);
    m_queryEngine = new QueryEngine(this);
    // 2. 初始化可视化场景
    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(-5000, -5000, 10000, 10000);
    ui->graphicsView->setScene(m_scene);

    // 优化渲染质量
    ui->graphicsView->setRenderHint(QPainter::Antialiasing);
    ui->graphicsView->setRenderHint(QPainter::SmoothPixmapTransform);
    ui->graphicsView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    QPixmap starPixmap(1000, 1000);
    starPixmap.fill(QColor("#0B0D17"));

    QPainter painter(&starPixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // 随机生成星星
    int starCount = 200; // 星星数量
    for (int i = 0; i < starCount; ++i) {
        // 随机坐标
        int x = QRandomGenerator::global()->bounded(1000);
        int y = QRandomGenerator::global()->bounded(1000);

        // 随机透明度 (50~255)，模拟星星闪烁亮暗不同
        int alpha = QRandomGenerator::global()->bounded(50, 255);
        QColor starColor(255, 255, 255, alpha);

        // 随机大小 (1~3像素)
        int size = QRandomGenerator::global()->bounded(1, 3);

        painter.setPen(Qt::NoPen);
        painter.setBrush(starColor);
        painter.drawEllipse(x, y, size, size);
    }

    // 将生成的星空图设置为背景刷
    ui->graphicsView->setBackgroundBrush(QBrush(starPixmap));

    // 允许鼠标拖拽画布（像地图一样平移）
    ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
    // 隐藏滚动条（可选，看起来更简洁）
    ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // 设置缩放锚点为鼠标位置（缩放时以鼠标为中心，而不是画布中心）
    ui->graphicsView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    ui->graphicsView->setResizeAnchor(QGraphicsView::AnchorViewCenter);

    ui->graphicsView->viewport()->installEventFilter(this);

    // 初始化属性面板列头
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

    createControlPanel();
   //建立连接
    setupConnections();
    setupToolbar();
    updateStatusBar();
    ui->graphicsView->centerOn(0, 0);
    // 5. 加载数据
    if (DatabaseConnection::isConnected()) {
        loadInitialData();
    }

    QTimer *renderTimer = new QTimer(this);

    connect(renderTimer, &QTimer::timeout, this, [this]() {
        if (m_scene) {
            m_scene->update(); // 触发全场景重绘
        }
    });

    renderTimer->start(16);
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
        newNode.ontologyId = m_currentOntologyId;

        if (m_hasClickPos) {
            newNode.posX = m_clickPos.x();
            newNode.posY = m_clickPos.y();
            m_hasClickPos = false; // 用完重置
        } else {
            // 如果是从上方菜单栏点击的，则随机在中心附近生成
            newNode.posX = QRandomGenerator::global()->bounded(200) - 100;
            newNode.posY = QRandomGenerator::global()->bounded(200) - 100;
        }

        if (!m_graphEditor->addNode(newNode)) {
            QMessageBox::warning(this, "添加失败", "无法添加节点！可能是节点名称已存在。\n请尝试更换名称。");
        }
    } else {
        // 如果取消了对话框，也清理坐标状态
        m_hasClickPos = false;
    }
}

void MainWindow::onActionDeleteTriggered() {
    // 1. 优先尝试从场景(画板)中删除选中的元素
    QList<QGraphicsItem*> selectedSceneItems = m_scene->selectedItems();
    if (!selectedSceneItems.isEmpty()) {
        if (QMessageBox::question(this, "确认删除", "确定要删除选中的实体及其关联吗？") == QMessageBox::Yes) {
            for(auto item : selectedSceneItems) {
                if (item->type() == VisualNode::Type) {
                    int id = item->data(0).toInt();
                    m_graphEditor->deleteNode(id);
                } else if (item->type() == VisualEdge::Type) {
                    VisualEdge* edge = qgraphicsitem_cast<VisualEdge*>(item);
                    m_graphEditor->deleteRelationship(edge->getId());
                }
            }
        }
        return; // 处理完毕退出
    }

    // 2. 如果画板中没有选中，再看右侧属性面板
    QList<QTreeWidgetItem*> selectedItems = ui->propertyPanel->selectedItems();
    if (selectedItems.isEmpty()) {
        ui->statusbar->showMessage("请先在视图或右侧列表中选中要删除的节点", 2000);
        return;
    }

    int nodeId = selectedItems.first()->text(0).toInt();
    if (QMessageBox::question(this, "确认删除", "确定要删除选中的实体及其关联吗？") == QMessageBox::Yes) {
        m_graphEditor->deleteNode(nodeId);
    }
}

void MainWindow::onNodeAdded(const GraphNode& node) {
    if (m_timer->isActive()) { // 只有在全图动态模式下才自动添加显示
        drawNode(node.id, node.name, node.nodeType, node.posX, node.posY);
    }
}
void MainWindow::onNodeDeleted(int nodeId) {
    for (int i = ui->propertyPanel->topLevelItemCount() - 1; i >= 0; --i) {
        if (ui->propertyPanel->topLevelItem(i)->text(0).toInt() == nodeId) {
            delete ui->propertyPanel->takeTopLevelItem(i);
            break;
        }
    }

    VisualNode* nodeToDelete = nullptr;
    foreach (QGraphicsItem *item, m_scene->items()) {
        if (item->type() == VisualNode::Type && item->data(0).toInt() == nodeId) {
            nodeToDelete = qgraphicsitem_cast<VisualNode*>(item);
            break;
        }
    }

    if (nodeToDelete) {
        QList<VisualEdge*> edgesToDelete;
        foreach (QGraphicsItem *item, m_scene->items()) {
            if (item->type() == VisualEdge::Type) {
                VisualEdge* edge = qgraphicsitem_cast<VisualEdge*>(item);
                if (edge->getSourceNode() == nodeToDelete || edge->getDestNode() == nodeToDelete) {
                    edgesToDelete.append(edge);
                }
            }
        }

        for (VisualEdge* edge : edgesToDelete) {
            if (edge->getSourceNode()) edge->getSourceNode()->removeEdge(edge);
            if (edge->getDestNode()) edge->getDestNode()->removeEdge(edge);

            if (m_layout) m_layout->removeEdge(edge);

            m_scene->removeItem(edge);
            delete edge;
        }

        if (m_layout) m_layout->removeNode(nodeToDelete);
        m_scene->removeItem(nodeToDelete);
        delete nodeToDelete;
    }

    updateStatusBar();
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
        edge.ontologyId = m_currentOntologyId;
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
    if (obj == ui->graphicsView->viewport()) {

        //  处理鼠标滚轮缩放
        if (event->type() == QEvent::Wheel) {
            QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
            if (wheelEvent->modifiers() & Qt::ControlModifier) {
                const double scaleFactor = 1.1;
                if (wheelEvent->angleDelta().y() > 0) ui->graphicsView->scale(scaleFactor, scaleFactor);
                else ui->graphicsView->scale(1.0 / scaleFactor, 1.0 / scaleFactor);
                return true; // 拦截事件，不再传递
            }
        }

        else if (event->type() == QEvent::ContextMenu) {
            QContextMenuEvent *cme = static_cast<QContextMenuEvent*>(event);

            // 获取鼠标在场景中的精确坐标
            QPointF scenePos = ui->graphicsView->mapToScene(cme->pos());

            // 检查鼠标下方是否有实体（节点或边）
            QGraphicsItem *item = m_scene->itemAt(scenePos, ui->graphicsView->transform());

            if (!item) {
                // 如果是空白区域，弹出添加节点菜单
                QMenu menu(this);
                QAction *addNodeAct = menu.addAction("添加节点");

                // 记录坐标，供生成节点使用
                m_hasClickPos = true;
                m_clickPos = scenePos;

                if (menu.exec(cme->globalPos()) == addNodeAct) {
                    onActionAddNodeTriggered();
                } else {
                    m_hasClickPos = false; // 用户取消了操作
                }
                return true; // 告诉系统：事件我处理完了，不要再往下传了
            }
            // 如果点中了节点或连线，返回 false，让 Qt 系统默认把右键事件传递给 VisualNode/VisualEdge
            return false;
        }
    }
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

    toolbar->addSeparator();

    // 属性面板切换按钮 ---
    QAction* actToggle = toolbar->addAction("属性面板");
    actToggle->setToolTip("显示/隐藏右侧属性列表");
    actToggle->setCheckable(true); // 设置为可勾选状态
    actToggle->setChecked(false);
    ui->propertyPanel->setVisible(false);
    connect(actToggle, &QAction::triggered, this, &MainWindow::onTogglePropertyPanel);

    QAction* actParams = toolbar->addAction("参数调节");
    actParams->setToolTip("打开/关闭引力参数控制台");
    actParams->setCheckable(true); // 让按钮变成可按下的状态

    // 连接点击信号：控制面板的显示/隐藏
    connect(actParams, &QAction::toggled, this, [this](bool checked){
        m_controlDock->setVisible(checked);
    });

    // 双向绑定：如果用户点了面板右上角的 'X' 关闭，按钮状态也要弹起来
    connect(m_controlDock, &QDockWidget::visibilityChanged, actParams, &QAction::setChecked);
}

void MainWindow::onTogglePropertyPanel() {
    bool isVisible = ui->propertyPanel->isVisible();
    ui->propertyPanel->setVisible(!isVisible);
}

// --- 1. 全图查询  ---
void MainWindow::onQueryFullGraph() {
    // 1. 获取数据
    QList<GraphNode> nodes = m_queryEngine->getAllNodes(m_currentOntologyId);
    QList<GraphEdge> edges = m_queryEngine->getAllRelationships(m_currentOntologyId);

    // 2. 清空视图
    m_scene->clear();
    m_layout->clear();
    ui->propertyPanel->clear();

    m_timer->start(30);
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
        VisualEdge* vEdge = nullptr;
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

void MainWindow::onSwitchOntology(int ontologyId, QString name) {
    if (m_currentOntologyId == ontologyId) return;

    m_currentOntologyId = ontologyId;
    this->setWindowTitle(QString("知识图谱系统 - 当前项目: %1").arg(name));

    // 重新查询全图
    onQueryFullGraph();
}

void MainWindow::createControlPanel() {
    // 1. 创建停靠窗口
    m_controlDock = new QDockWidget("参数控制台", this);
    m_controlDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);

    // 2. 创建面板内的容器
    QWidget *container = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setSpacing(20); // 控件稍微隔开点

    // --- 斥力滑块 ---
    layout->addWidget(new QLabel("斥力 :"));
    QSlider *sldRepulsion = new QSlider(Qt::Horizontal);
    sldRepulsion->setRange(100, 2000);
    sldRepulsion->setValue(800); // 默认值
    connect(sldRepulsion, &QSlider::valueChanged, this, [this](int val){
        if(m_layout) m_layout->setRepulsion(val);
    });
    layout->addWidget(sldRepulsion);

    // --- 引力滑块 ---
    layout->addWidget(new QLabel("引力:"));
    QSlider *sldStiffness = new QSlider(Qt::Horizontal);
    sldStiffness->setRange(1, 20); // 0.01 - 0.20
    sldStiffness->setValue(8);
    connect(sldStiffness, &QSlider::valueChanged, this, [this](int val){
        if(m_layout) m_layout->setStiffness(val / 100.0);
    });
    layout->addWidget(sldStiffness);

    // --- 公转速度滑块 ---
    layout->addWidget(new QLabel("公转速度 :"));
    QSlider *sldSpeed = new QSlider(Qt::Horizontal);
    sldSpeed->setRange(0, 50); // 0.0 - 5.0
    sldSpeed->setValue(10); // 默认 1.0
    connect(sldSpeed, &QSlider::valueChanged, this, [this](int val){
        if(m_layout) m_layout->setOrbitSpeed(val / 10.0);
    });
    layout->addWidget(sldSpeed);

    // 底部弹簧，把控件顶上去
    layout->addStretch();

    // 3. 装载并初始隐藏
    m_controlDock->setWidget(container);
    this->addDockWidget(Qt::RightDockWidgetArea, m_controlDock);
    m_controlDock->setVisible(false); // 默认隐藏，保持界面清爽
}

