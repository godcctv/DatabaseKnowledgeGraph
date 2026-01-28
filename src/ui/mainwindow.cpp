#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "addnodedialog.h"
#include <QGraphicsTextItem>
#include "../database/DatabaseConnection.h"
#include <QDebug>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 1. 初始化后端
    m_graphEditor = new GraphEditor(this);

    // 2. 初始化可视化场景 (为第5周做准备，防止崩溃)
    m_scene = new QGraphicsScene(this);
    ui->graphicsView->setScene(m_scene);

    // 3. 初始化属性面板列头
    ui->propertyPanel->setHeaderLabels(QStringList() << "ID" << "名称" << "类型");

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
    // 1. 获取 ID=1 的本体下的所有节点 (假设我们目前只操作本体1)

    QList<GraphNode> nodes = NodeRepository::getAllNodes(1);

    for (const auto& node : nodes) {
        onNodeAdded(node);
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

}

// 用户点击“添加节点”按钮
void MainWindow::onActionAddNodeTriggered() {
    AddNodeDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        GraphNode newNode = dialog.getNodeData();
        newNode.ontologyId = 1;

        // 🔥 修改：根据返回值判断是否成功 🔥
        if (!m_graphEditor->addNode(newNode)) {
            QMessageBox::warning(this, "添加失败",
                "无法添加节点！可能是节点名称已存在。\n请尝试更换名称。");
        }
    }
}

void MainWindow::onActionDeleteTriggered() {
    // 1. 获取右侧列表选中的项
    QList<QTreeWidgetItem*> selectedItems = ui->propertyPanel->selectedItems();
    if (selectedItems.isEmpty()) {
        ui->statusbar->showMessage("请先在右侧列表中选中一个节点", 2000);
        return;
    }

    // 2. 获取 ID (第0列是 ID)
    int nodeId = selectedItems.first()->text(0).toInt();

    qDebug() << "请求删除节点 ID:" << nodeId;

    // 3. 调用后端删除
    // 后端成功后会 emit nodeDeleted(nodeId)，从而触发上面的 onNodeDeleted
    if (!m_graphEditor->deleteNode(nodeId)) {
        ui->statusbar->showMessage("删除失败，请检查控制台日志", 3000);
    }
}

// src/ui/mainwindow.cpp

void MainWindow::onNodeAdded(const GraphNode& node) {
    // 1. 安全检查：如果 UI 或 场景还没初始化，直接退出，防止崩溃
    if (!ui || !ui->propertyPanel || !m_scene) {
        qWarning() << "onNodeAdded 被调用，但 UI 或 m_scene 未初始化，跳过绘制";
        return;
    }

    // 2. 添加到右侧列表
    QTreeWidgetItem *item = new QTreeWidgetItem(ui->propertyPanel);
    item->setText(0, QString::number(node.id));
    item->setText(1, node.name);
    item->setText(2, node.nodeType);

    // 3. 在画布上画圆
    auto ellipse = m_scene->addEllipse(node.posX, node.posY, 50, 50, QPen(Qt::black), QBrush(Qt::cyan));

    // 存入 ID，为了以后能删除它
    ellipse->setData(0, node.id);
    // 让圆圈可以被鼠标选中和拖动
    ellipse->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);

    // 4. 在圆圈中间画文字
    auto text = m_scene->addText(node.name);
    text->setPos(node.posX + 5, node.posY + 10); // 稍微偏移一点，居中显示
    text->setData(0, node.id); // 文字也存一下 ID

    // 5. 状态栏提示 (加个判断防止崩溃)
    if (ui->statusbar) {
        ui->statusbar->showMessage(QString("节点 %1 加载成功").arg(node.name), 3000);
    }
}
void MainWindow::onNodeDeleted(int nodeId) {
    // --- 1. 安全删除右侧列表项 ---
    // 技巧：使用【倒序遍历】。
    // 如果正序遍历，删除第0个后，第1个会变成第0个，索引就会乱，导致漏删或越界。
    for (int i = ui->propertyPanel->topLevelItemCount() - 1; i >= 0; --i) {
        QTreeWidgetItem *item = ui->propertyPanel->topLevelItem(i);
        if (item->text(0).toInt() == nodeId) {
            delete ui->propertyPanel->takeTopLevelItem(i); // 彻底移除并释放内存
            break; // 找到ID后立即退出循环，提高效率
        }
    }

    // --- 2. 安全删除绘图场景项 (修复卡死的关键) ---

    // 第一步：先“只读”遍历，找出所有要删除的项，存到列表中
    QList<QGraphicsItem*> itemsToDelete;
    foreach (QGraphicsItem *item, m_scene->items()) {
        // data(0) 是我们在 onNodeAdded 时存入的节点ID
        if (item->data(0).toInt() == nodeId) {
            itemsToDelete.append(item);
        }
    }

    // 第二步：遍历临时列表进行真正的删除操作
    for (QGraphicsItem *item : itemsToDelete) {
        m_scene->removeItem(item); // 从场景卸载
        delete item;               // 释放内存
    }

    // 3. 状态栏提示
    ui->statusbar->showMessage(QString("节点 ID %1 已删除").arg(nodeId), 3000);

    // 4. 强制刷新场景（防止有残影）
    m_scene->update();
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
