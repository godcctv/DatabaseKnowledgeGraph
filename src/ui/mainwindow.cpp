#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "addnodedialog.h"
#include <QGraphicsTextItem>
#include "../database/DatabaseConnection.h"
#include <QCoreApplication>
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
