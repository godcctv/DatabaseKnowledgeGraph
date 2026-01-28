#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "addnodedialog.h"
#include <QGraphicsTextItem>
#include "../database/DatabaseConnection.h"
#include <QDebug>

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
}

MainWindow::~MainWindow() {
    delete ui;
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
    // 1. 弹出对话框
    AddNodeDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        // 2. 获取数据
        GraphNode newNode = dialog.getNodeData();
        newNode.ontologyId = 1; // 暂时写死，后续从项目设置中获取

        // 3. 调用后端添加 (后端会负责写数据库 + 发信号)
        m_graphEditor->addNode(newNode);
    }
}

void MainWindow::onActionDeleteTriggered() {
    // 1. 获取右侧列表选中的项
    QList<QTreeWidgetItem*> selectedItems = ui->propertyPanel->selectedItems();
    if (selectedItems.isEmpty()) {
        ui->statusbar->showMessage("请先在右侧列表中选择一个节点", 2000);
        return;
    }
    // 2. 获取 ID
    int nodeId = selectedItems.first()->text(0).toInt();
    // 3. 调用后端删除
    m_graphEditor->deleteNode(nodeId);
}

void MainWindow::onNodeAdded(const GraphNode& node) {
    // 1. 更新右侧列表
    QTreeWidgetItem *item = new QTreeWidgetItem(ui->propertyPanel);
    item->setText(0, QString::number(node.id));
    item->setText(1, node.name);
    item->setText(2, node.nodeType);

    // 2. 更新绘图区
    // 画圆
    auto ellipse = m_scene->addEllipse(node.posX, node.posY, 50, 50, QPen(Qt::black), QBrush(Qt::cyan));

    ellipse->setData(0, node.id);
    // 让圆圈可选择、可移动（为后面铺路）
    ellipse->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);

    // 画文字
    auto text = m_scene->addText(node.name);
    text->setPos(node.posX, node.posY);
    text->setData(0, node.id); // 文字也存一下 ID

    ui->statusbar->showMessage(QString("节点 %1 添加成功").arg(node.name), 3000);
}

void MainWindow::onNodeDeleted(int nodeId) {
    // --- 1. 安全删除右侧列表项 ---
    // 倒序遍历是删除列表项最安全的方式（防止索引错位）
    for (int i = ui->propertyPanel->topLevelItemCount() - 1; i >= 0; --i) {
        QTreeWidgetItem *item = ui->propertyPanel->topLevelItem(i);
        if (item->text(0).toInt() == nodeId) {
            delete ui->propertyPanel->takeTopLevelItem(i); // 使用 takeItem 彻底移除
            break; // 找到后立即退出循环
        }
    }

    // --- 2. 安全删除绘图场景项 (修复卡死问题的关键) ---
    // 第一步：先找出所有需要删除的项，存到一个临时列表里
    QList<QGraphicsItem*> itemsToDelete;
    foreach (QGraphicsItem *item, m_scene->items()) {
        // 检查 data(0) 是否匹配
        if (item->data(0).toInt() == nodeId) {
            itemsToDelete.append(item);
        }
    }

    // 第二步：遍历临时列表进行删除
    // 这样做避免了在 m_scene->items() 循环中直接修改场景结构
    for (QGraphicsItem *item : itemsToDelete) {
        m_scene->removeItem(item); // 从场景移除
        delete item;               // 释放内存
    }

    // 3. 提示信息
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
