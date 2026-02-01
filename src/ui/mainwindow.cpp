#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "addnodedialog.h"
#include <QGraphicsTextItem>
#include "../database/DatabaseConnection.h"
#include <QCoreApplication>
#include <QDebug>
#include <QMessageBox>
#include <QRadialGradient>
#include <QGraphicsDropShadowEffect>


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
    // 1. 安全检查
    if (!ui || !ui->propertyPanel || !m_scene) return;

    // --- 更新右侧列表 (保持不变) ---
    QTreeWidgetItem *item = new QTreeWidgetItem(ui->propertyPanel);
    item->setText(0, QString::number(node.id));
    item->setText(1, node.name);
    item->setText(2, node.nodeType);

    // --- 🔥 3D 视觉升级开始 🔥 ---

    // 2.1 定义尺寸和基本颜色
    qreal size = 50.0;
    QColor baseColor(Qt::cyan); // 默认颜色，你也可以读取 node.color
    if (node.nodeType == "Concept") baseColor = QColor("#2ecc71"); // 绿色
    else if (node.nodeType == "Entity") baseColor = QColor("#3498db"); // 蓝色

    // 2.2 创建径向渐变 (模拟光照)
    // 圆心(cx, cy) 和 焦点(fx, fy) 稍微向左上角偏移，模拟光从左上角打过来
    QRadialGradient gradient(node.posX + size/2, node.posY + size/2, size/2,
                             node.posX + size/3, node.posY + size/3);

    // 设置渐变色：中心亮，边缘暗
    gradient.setColorAt(0, baseColor.lighter(150)); // 高光区域
    gradient.setColorAt(0.3, baseColor);            // 本体颜色
    gradient.setColorAt(1, baseColor.darker(150));  // 边缘阴影

    // 2.3 绘制“球体” (去掉边框 pen，只用渐变 brush)
    auto ellipse = m_scene->addEllipse(node.posX, node.posY, size, size,
                                       QPen(Qt::NoPen), QBrush(gradient));

    // 2.4 添加阴影特效 (让球体看起来悬浮)
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(15);        // 模糊半径
    shadow->setOffset(5, 5);          // 阴影向右下偏移
    shadow->setColor(QColor(0, 0, 0, 100)); // 半透明黑色
    ellipse->setGraphicsEffect(shadow);

    // 2.5 设置交互属性
    ellipse->setData(0, node.id);
    ellipse->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);

    // 优化：鼠标悬停时显示手型
    ellipse->setCursor(Qt::PointingHandCursor);

    // --- 绘制文字 ---
    auto text = m_scene->addText(node.name);
    // 让文字居中显示在球体上方或中间
    // 这里的偏移量可能需要根据文字长度微调，或者使用 QFontMetrics 计算
    text->setPos(node.posX + 5, node.posY + 10);
    text->setDefaultTextColor(Qt::white); // 深色球体配白色文字更清晰

    // 给文字也加一点微弱的阴影，防止在浅色背景看不清
    QGraphicsDropShadowEffect *textShadow = new QGraphicsDropShadowEffect();
    textShadow->setBlurRadius(1);
    textShadow->setOffset(1, 1);
    textShadow->setColor(Qt::black);
    text->setGraphicsEffect(textShadow);

    text->setData(0, node.id);

    // --- 状态栏提示 ---
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
