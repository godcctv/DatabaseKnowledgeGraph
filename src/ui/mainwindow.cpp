#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "addnodedialog.h"
#include "addedgedialog.h"
#include "VisualNode.h"
#include "VisualEdge.h"
#include "QueryDialog.h"
#include "DashboardDialog.h"
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
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QDialog>
#include <QFrame>
#include <QTextEdit>
#include <QPushButton>

QWidget* createSliderRow(QWidget* parent, const QString& labelText, int min, int max, int val, const QString& suffix, std::function<void(int)> callback) {
    QWidget* widget = new QWidget(parent);
    QHBoxLayout* layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);

    QLabel* label = new QLabel(labelText, widget);
    label->setMinimumWidth(60);

    QSlider* slider = new QSlider(Qt::Horizontal, widget);
    slider->setRange(min, max);
    slider->setValue(val);

    // 数值显示框
    QSpinBox* spinBox = new QSpinBox(widget);
    spinBox->setRange(min, max);
    spinBox->setValue(val);
    spinBox->setSuffix(suffix);
    spinBox->setButtonSymbols(QAbstractSpinBox::NoButtons); // 隐藏上下箭头，纯显示用
    spinBox->setFixedWidth(60);
    spinBox->setAlignment(Qt::AlignCenter);

    // 双向绑定：滑块动 -> 数字变 -> 触发回调
    QObject::connect(slider, &QSlider::valueChanged, spinBox, &QSpinBox::setValue);
    QObject::connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), slider, &QSlider::setValue);

    // 核心回调
    QObject::connect(slider, &QSlider::valueChanged, callback);

    layout->addWidget(label);
    layout->addWidget(slider);
    layout->addWidget(spinBox);

    return widget;
}

MainWindow::MainWindow(int ontologyId, QString ontologyName,User currentUser, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_currentUser(currentUser)
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


    // --- Nord Theme 极简极客风：点阵网格背景 ---
    // 生成一个 40x40 的平铺图块
    QPixmap bgPixmap(40, 40);
    bgPixmap.fill(QColor("#2E3440")); // Nord 的标志性基础底色 (Polar Night)

    QPainter painter(&bgPixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#434C5E")); // 稍微亮一点的网格点颜色

    // 在中心画一个 2x2 的细微方点，平铺后会形成科技感极强的网格
    painter.drawRect(19, 19, 2, 2);

    // 将网格图设置为画板背景（Qt 会自动平铺）
    ui->graphicsView->setBackgroundBrush(QBrush(bgPixmap));

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
    if (!m_currentUser.isAdmin) {
        // 1. 修改权限拦截
        ui->actionAddNode->setEnabled(m_currentUser.canEdit);
        ui->actionAddRelation->setEnabled(m_currentUser.canEdit);

        // 2. 删除权限拦截
        ui->actionDelete->setEnabled(m_currentUser.canDelete);
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
    connect(m_graphEditor, &GraphEditor::nodeUpdated, this, &MainWindow::onNodeUpdated);
    connect(m_graphEditor, &GraphEditor::relationshipUpdated, this, &MainWindow::onRelationshipUpdated);

    // 绑定新的批量导入和导出动作
    connect(ui->actionImportData, &QAction::triggered, this, &MainWindow::onActionImportTriggered);
    connect(ui->actionExportData, &QAction::triggered, this, &MainWindow::onActionExportTriggered);
}

void MainWindow::onActionAddNodeTriggered() {
    if (!m_currentUser.isAdmin && !m_currentUser.canEdit) {
        QMessageBox::warning(this, "权限不足", "您没有修改图谱数据的权限！");
        return;
    }
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
    if (!m_currentUser.isAdmin && !m_currentUser.canDelete) {
        QMessageBox::warning(this, "权限不足", "您没有删除数据的权限！");
        return;
    }
    // 1. 优先尝试从场中删除选中的元素
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

void MainWindow::onActionEditNodeTriggered(int nodeId) {
    GraphNode oldNode = m_queryEngine->getNodeById(nodeId);
    if (!oldNode.isValid()) return;

    AddNodeDialog dialog(this);
    dialog.setWindowTitle("修改节点");
    dialog.setNodeData(oldNode);

    if (dialog.exec() == QDialog::Accepted) {
        GraphNode newNode = dialog.getNodeData();
        // 继承原有不可变属性
        newNode.id = oldNode.id;
        newNode.ontologyId = oldNode.ontologyId;
        newNode.posX = oldNode.posX;
        newNode.posY = oldNode.posY;
        newNode.color = oldNode.color;
        newNode.properties = oldNode.properties;

        if (!m_graphEditor->updateNode(oldNode, newNode)) {
            QMessageBox::warning(this, "错误", "更新节点失败，名称可能已存在！");
        }
    }
}

void MainWindow::onActionEditRelationshipTriggered(int edgeId) {
    GraphEdge oldEdge = RelationshipRepository::getRelationshipById(edgeId);
    if (oldEdge.id <= 0) return;

    AddEdgeDialog dialog(this);
    dialog.setWindowTitle("修改关系");
    dialog.setNodes(QString("起点ID: %1").arg(oldEdge.sourceId), QString("终点ID: %2").arg(oldEdge.targetId));
    dialog.setEdgeData(oldEdge);

    if (dialog.exec() == QDialog::Accepted) {
        GraphEdge newEdge = dialog.getEdgeData();
        newEdge.id = oldEdge.id;
        newEdge.ontologyId = oldEdge.ontologyId;
        newEdge.sourceId = oldEdge.sourceId;
        newEdge.targetId = oldEdge.targetId;
        newEdge.properties = oldEdge.properties;

        if (!m_graphEditor->updateRelationship(oldEdge, newEdge)) {
            QMessageBox::warning(this, "错误", "更新关系失败！");
        }
    }
}

void MainWindow::onNodeUpdated(const GraphNode& node) {
    foreach (QGraphicsItem *item, m_scene->items()) {
        if (item->type() == VisualNode::Type && item->data(0).toInt() == node.id) {
            VisualNode* vNode = qgraphicsitem_cast<VisualNode*>(item);
            vNode->updateData(node.name, node.nodeType);
            break;
        }
    }
    for (int i = 0; i < ui->propertyPanel->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = ui->propertyPanel->topLevelItem(i);
        if (item->text(0).toInt() == node.id) {
            item->setText(1, node.name);
            item->setText(2, node.nodeType);
            break;
        }
    }
    ui->statusbar->showMessage("节点更新成功", 2000);
}

void MainWindow::onRelationshipUpdated(const GraphEdge& edge) {
    foreach (QGraphicsItem *item, m_scene->items()) {
        if (item->type() == VisualEdge::Type) {
            VisualEdge* vEdge = qgraphicsitem_cast<VisualEdge*>(item);
            if (vEdge->getId() == edge.id) {
                vEdge->updateData(edge.relationType);
                break;
            }
        }
    }
    ui->statusbar->showMessage("关系更新成功", 2000);
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
                if (!m_currentUser.isAdmin && !m_currentUser.canEdit) {
                    ui->statusbar->showMessage("权限不足：您没有修改(添加节点)的权限", 3000);
                    return true;
                }
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
    QAction* actDashboard = toolbar->addAction("数据仪表盘");
    actDashboard->setToolTip("查看当前图谱的数据统计与分析");
    connect(actDashboard, &QAction::triggered, this, &MainWindow::onOpenDashboard);

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
            QString actualRelationType = "未知";
            QList<GraphEdge> relatedEdges = m_queryEngine->getRelatedRelationships(prevVNode->getId());

            for (const auto& e : relatedEdges) {
                if ((e.sourceId == prevVNode->getId() && e.targetId == currVNode->getId()) ||
                    (e.sourceId == currVNode->getId() && e.targetId == prevVNode->getId())) {
                    actualRelationType = e.relationType;
                    break;
                    }
            }

            VisualEdge* edge = new VisualEdge(-1, prevVNode->getId(), currVNode->getId(), actualRelationType, prevVNode, currVNode);
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


    m_controlDock->setFeatures(QDockWidget::NoDockWidgetFeatures);

    // 2. 创建面板内的容器
    QWidget *container = new QWidget();
    // 设置 Nord 风格的参数控制台底色与控件样式
    container->setStyleSheet(R"(
        QWidget { background-color: #3B4252; border-radius: 4px; border: 1px solid #4C566A; }
        QLabel { color: #D8DEE9; font-weight: bold; font-size: 13px; border: none; background: transparent; }
        QSlider::groove:horizontal { border: 1px solid #4C566A; height: 6px; background: #2E3440; border-radius: 3px; }
        QSlider::handle:horizontal { background: #88C0D0; border: 1px solid #88C0D0; width: 14px; margin: -5px 0; border-radius: 7px; }
        QSlider::handle:horizontal:hover { background: #8FBCBB; }
        QSpinBox { background-color: #2E3440; color: #ECEFF4; border: 1px solid #4C566A; border-radius: 3px; padding: 2px; }
    )");

    m_controlDock->setStyleSheet("QDockWidget { color: #88C0D0; font-weight: bold; } QDockWidget::title { background: #2E3440; padding: 6px; }");
    QVBoxLayout *mainLayout = new QVBoxLayout(container);
    mainLayout->setSpacing(12); // 稍微紧凑一点的间距
    mainLayout->setContentsMargins(15, 20, 15, 20); // 增加内边距，不那么拥挤
    // --- 斥力 (Repulsion) ---
    mainLayout->addWidget(createSliderRow(container, "星体斥力:", 100, 3000,
        static_cast<int>(m_layout->getRepulsion()), "",
        [this](int val){ if(m_layout) m_layout->setRepulsion(val); }));

    // --- 引力 (Stiffness) ---
    mainLayout->addWidget(createSliderRow(container, "引力强度:", 1, 20,
        static_cast<int>(m_layout->getStiffness() * 100), "",
        [this](int val){ if(m_layout) m_layout->setStiffness(val / 100.0); }));

    // --- 阻尼 (Damping) ---
    mainLayout->addWidget(createSliderRow(container, "空间阻力:", 50, 99,
        static_cast<int>(m_layout->getDamping() * 100), "%",
        [this](int val){ if(m_layout) m_layout->setDamping(val / 100.0); }));

    // 底部弹簧
    mainLayout->addStretch();

    // 3. 装载
    m_controlDock->setWidget(container);
    this->addDockWidget(Qt::RightDockWidgetArea, m_controlDock);

    // 默认隐藏，由工具栏按钮控制显示
    m_controlDock->setVisible(false);
}

void MainWindow::showNodeDetails(int nodeId) {
    GraphNode node = m_queryEngine->getNodeById(nodeId);
    if (!node.isValid()) return;

    QString desc = node.description.trimmed().isEmpty() ? "（无描述信息）" : node.description;

    //创建自定义弹窗
    QDialog dialog(this);
    dialog.setWindowTitle("节点详细信息");
    dialog.resize(320, 280); // 设定和添加节点差不多的尺寸

    //设置主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setContentsMargins(25, 25, 25, 20);
    mainLayout->setSpacing(15);

    //顶部信息展示区
    QFrame* infoFrame = new QFrame(&dialog);
    infoFrame->setObjectName("InfoFrame");
    QVBoxLayout* formLayout = new QVBoxLayout(infoFrame);
    formLayout->setSpacing(12);

    QLabel* idLabel = new QLabel(QString("<b>节点 ID:</b>  %1").arg(node.id), infoFrame);
    QLabel* nameLabel = new QLabel(QString("<b>节点名称:</b>  %1").arg(node.name), infoFrame);
    QLabel* typeLabel = new QLabel(QString("<b>节点类型:</b>  %1").arg(node.nodeType), infoFrame);

    formLayout->addWidget(idLabel);
    formLayout->addWidget(nameLabel);
    formLayout->addWidget(typeLabel);

    //  描述信息展示区
    QLabel* descTitle = new QLabel("<b>详细描述:</b>", &dialog);
    QTextEdit* descEdit = new QTextEdit(&dialog);
    descEdit->setPlainText(desc);
    descEdit->setReadOnly(true); // 设为只读
    descEdit->setMaximumHeight(80);

    // 底部按钮区
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    QPushButton* okBtn = new QPushButton("确定", &dialog);
    okBtn->setMinimumWidth(80);
    btnLayout->addWidget(okBtn);

    // 点击确定关闭窗口
    connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

    // 组装所有控件
    mainLayout->addWidget(infoFrame);
    mainLayout->addWidget(descTitle);
    mainLayout->addWidget(descEdit);
    mainLayout->addLayout(btnLayout);

    dialog.setStyleSheet(R"(
        QDialog { background-color: #2E3440; border: 1px solid #4C566A; }
        QLabel { color: #D8DEE9; font-size: 13px; }
        QFrame#InfoFrame {
            background-color: #3B4252;
            border: 1px solid #4C566A;
            border-radius: 4px;
            padding: 8px;
        }
        QFrame#InfoFrame QLabel { color: #ECEFF4; }
        QTextEdit {
            background-color: #3B4252;
            border: 1px solid #4C566A;
            border-radius: 4px;
            padding: 6px;
            color: #ECEFF4;
            font-size: 13px;
        }
        QPushButton {
            background-color: #4C566A;
            color: #ECEFF4;
            border: 1px solid #434C5E;
            padding: 6px 20px;
            border-radius: 4px;
            font-weight: bold;
        }
        QPushButton:hover { background-color: #5E81AC; border-color: #81A1C1; }
        QPushButton:pressed { background-color: #81A1C1; }
    )");
    okBtn->setDefault(true);
    dialog.exec(); // 阻塞式弹出窗口
}

void MainWindow::onActionExportTriggered() {
    // 打开文件保存对话框
    QString fileName = QFileDialog::getSaveFileName(this, "导出知识图谱", "", "JSON 图谱文件 (*.json)");
    if (fileName.isEmpty()) return;

    // 1. 从数据库获取当前图谱的所有数据
    QList<GraphNode> nodes = m_queryEngine->getAllNodes(m_currentOntologyId);
    QList<GraphEdge> edges = m_queryEngine->getAllRelationships(m_currentOntologyId);

    QJsonObject rootObj;

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

    // 3. 序列化关系
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

    rootObj["nodes"] = nodesArray;
    rootObj["edges"] = edgesArray;

    // 4. 写入文件
    QJsonDocument doc(rootObj);
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        QMessageBox::information(this, "导出成功", QString("成功导出 %1 个节点和 %2 条关系！").arg(nodes.size()).arg(edges.size()));
    } else {
        QMessageBox::warning(this, "错误", "无法保存文件，请检查权限！");
    }
}

void MainWindow::onActionImportTriggered() {
    // 权限拦截
    if (!m_currentUser.isAdmin && !m_currentUser.canEdit) {
        QMessageBox::warning(this, "权限不足", "您没有导入(修改)图谱数据的权限！");
        return;
    }

    QString fileName = QFileDialog::getOpenFileName(this, "导入知识图谱", "", "JSON 图谱文件 (*.json)");
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
    QJsonArray nodesArray = rootObj["nodes"].toArray();
    QJsonArray edgesArray = rootObj["edges"].toArray();

    // 核心字典：保存【原图谱文件里的旧 ID】与【刚存入数据库分配的新 ID】的映射
    QMap<int, int> idMapping;

    int importedNodes = 0;
    int importedEdges = 0;

    // 暂停动画与渲染，提高大量插入性能
    m_timer->stop();

    // 1. 先导入所有节点
    for (int i = 0; i < nodesArray.size(); ++i) {
        QJsonObject nodeObj = nodesArray[i].toObject();
        GraphNode node;
        node.ontologyId = m_currentOntologyId; // 强行挂载到当前的图谱项目下
        node.name = nodeObj["name"].toString();
        node.nodeType = nodeObj["nodeType"].toString();
        node.description = nodeObj["description"].toString();
        node.posX = nodeObj["posX"].toDouble(0);
        node.posY = nodeObj["posY"].toDouble(0);
        node.color = nodeObj["color"].toString("#3498db");
        node.properties = nodeObj["properties"].toObject();

        int oldId = nodeObj["id"].toInt();

        // 插入数据库 (GraphEditor里自带了撤销栈管理和事务)
        if (m_graphEditor->addNode(node)) {
            // node.id 此时已经被 Repository 刷新成了数据库里的真实 Auto-Increment ID
            idMapping[oldId] = node.id;
            importedNodes++;
        }
    }

    // 2. 再导入连线
    for (int i = 0; i < edgesArray.size(); ++i) {
        QJsonObject edgeObj = edgesArray[i].toObject();
        int oldSourceId = edgeObj["sourceId"].toInt();
        int oldTargetId = edgeObj["targetId"].toInt();

        // 只有当源节点和目标节点都成功导入，且映射存在时才连线
        if (idMapping.contains(oldSourceId) && idMapping.contains(oldTargetId)) {
            GraphEdge edge;
            edge.ontologyId = m_currentOntologyId;
            edge.sourceId = idMapping[oldSourceId]; // 替换为真实的新 ID
            edge.targetId = idMapping[oldTargetId]; // 替换为真实的新 ID
            edge.relationType = edgeObj["relationType"].toString();
            edge.weight = edgeObj["weight"].toDouble(1.0);
            edge.properties = edgeObj["properties"].toObject();

            if (m_graphEditor->addRelationship(edge)) {
                importedEdges++;
            }
        }
    }

    // 3. 导入完成，重新加载全图渲染
    onQueryFullGraph();

    QMessageBox::information(this, "导入完成",
        QString("成功导入:\n%1 个节点\n%2 条连线").arg(importedNodes).arg(importedEdges));
}

void MainWindow::onOpenDashboard() {
    DashboardDialog dialog(m_currentOntologyId, m_queryEngine, this);
    dialog.exec();
}