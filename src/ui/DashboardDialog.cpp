#include "DashboardDialog.h"
#include "../business/QueryEngine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QPainter>
#include <QPainterPath>
#include <QProgressBar>

// =================== 自定义环形图实现 ===================
SimpleRingChart::SimpleRingChart(QWidget *parent) : QWidget(parent) {
    setMinimumHeight(150);
    // Nord 主题扩展色板，用于图表切片
    // 修改后 (显式调用 QStringList 构造)
    m_colors = QStringList{"#88C0D0", "#B48EAD", "#EBCB8B", "#A3BE8C", "#D08770", "#BF616A", "#5E81AC", "#81A1C1"};
}

void SimpleRingChart::setData(const QMap<QString, int>& data) {
    m_data = data;
    update(); // 触发重绘
}

void SimpleRingChart::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int total = 0;
    for (int v : m_data.values()) total += v;

    int size = qMin(width(), height()) - 20;
    QRectF rect((width() - size) / 2.0, (height() - size) / 2.0, size, size);

    if (total == 0) {
        // 无数据时画一个暗色的空心圆
        painter.setPen(QPen(QColor("#4C566A"), 15));
        painter.drawEllipse(rect.adjusted(15, 15, -15, -15));
        return;
    }

    int startAngle = 90 * 16; // 从12点钟方向开始画
    int colorIdx = 0;

    // 绘制外圈饼图
    for (auto it = m_data.begin(); it != m_data.end(); ++it) {
        int spanAngle = qRound(-(it.value() / (double)total) * 360 * 16);
        painter.setBrush(QColor(m_colors[colorIdx % m_colors.size()]));
        painter.setPen(Qt::NoPen);
        painter.drawPie(rect, startAngle, spanAngle);
        startAngle += spanAngle;
        colorIdx++;
    }

    // 绘制内圈遮挡，形成“环形”效果 (Donut)
    painter.setBrush(QColor("#3B4252")); // 与背景同色
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(rect.adjusted(25, 25, -25, -25)); // 25是环的厚度

    // 中心写字
    painter.setPen(QColor("#ECEFF4"));
    painter.setFont(QFont("Microsoft YaHei", 12, QFont::Bold));
    painter.drawText(rect, Qt::AlignCenter, QString::number(total));
}

// =================== 仪表盘主界面实现 ===================

DashboardDialog::DashboardDialog(int ontologyId, QueryEngine* engine, QWidget *parent)
    : QDialog(parent), m_ontologyId(ontologyId), m_engine(engine)
{
    setWindowTitle("项目数据仪表盘 (Data Dashboard)");
    resize(750, 600); // 稍微加大窗口以容纳图表
    setupUI();
    loadData();
}

DashboardDialog::~DashboardDialog() {}

QWidget* DashboardDialog::createStatCard(const QString& title, QLabel*& valueLabel, const QString& icon) {
    QFrame* card = new QFrame(this);
    card->setObjectName("StatCard");
    QVBoxLayout* layout = new QVBoxLayout(card);

    QLabel* titleLbl = new QLabel(icon + " " + title, card);
    titleLbl->setStyleSheet("color: #D8DEE9; font-size: 13px; font-weight: bold;");

    valueLabel = new QLabel("0", card);
    valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    valueLabel->setStyleSheet("color: #88C0D0; font-size: 26px; font-weight: bold;");

    layout->addWidget(titleLbl);
    layout->addWidget(valueLabel);
    return card;
}

void DashboardDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(25, 25, 25, 25);
    mainLayout->setSpacing(20);

    // --- 1. 顶部：核心指标卡片区 ---
    QHBoxLayout* summaryLayout = new QHBoxLayout();
    summaryLayout->setSpacing(15);

    summaryLayout->addWidget(createStatCard("实体总数", m_lblTotalNodes, "🔵"));
    summaryLayout->addWidget(createStatCard("关系总数", m_lblTotalEdges, "🔗"));
    summaryLayout->addWidget(createStatCard("核心枢纽节点", m_lblCoreNode, "👑"));

    mainLayout->addLayout(summaryLayout);

    // --- 2. 下方：图表与数据分布区 ---
    QHBoxLayout* contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(20);

    // 左侧：节点统计
    QGroupBox* nodeGroup = new QGroupBox("节点类型占比", this);
    QVBoxLayout* nodeLayout = new QVBoxLayout(nodeGroup);
    m_nodeChart = new SimpleRingChart(this); // 原生环形图
    m_nodeTable = new QTableWidget(this);
    m_nodeTable->setColumnCount(3);
    m_nodeTable->setHorizontalHeaderLabels({"类型", "数量", "明细"});
    m_nodeTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_nodeTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_nodeTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_nodeTable->verticalHeader()->setVisible(false);
    m_nodeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_nodeTable->setSelectionMode(QAbstractItemView::NoSelection);
    nodeLayout->addWidget(m_nodeChart);
    nodeLayout->addWidget(m_nodeTable);

    // 右侧：关系统计
    QGroupBox* edgeGroup = new QGroupBox("关系类型占比", this);
    QVBoxLayout* edgeLayout = new QVBoxLayout(edgeGroup);
    m_edgeChart = new SimpleRingChart(this); // 原生环形图
    m_edgeTable = new QTableWidget(this);
    m_edgeTable->setColumnCount(3);
    m_edgeTable->setHorizontalHeaderLabels({"类型", "数量", "明细"});
    m_edgeTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_edgeTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_edgeTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_edgeTable->verticalHeader()->setVisible(false);
    m_edgeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_edgeTable->setSelectionMode(QAbstractItemView::NoSelection);
    edgeLayout->addWidget(m_edgeChart);
    edgeLayout->addWidget(m_edgeTable);

    contentLayout->addWidget(nodeGroup);
    contentLayout->addWidget(edgeGroup);
    mainLayout->addLayout(contentLayout);

    // --- Nord 主题深度美化 ---
    this->setStyleSheet(R"(
        QDialog { background-color: #2E3440; color: #D8DEE9; }
        QFrame#StatCard {
            background-color: #3B4252; border: 1px solid #4C566A;
            border-radius: 8px; padding: 10px;
        }
        QGroupBox {
            background-color: #3B4252; border: 1px solid #4C566A;
            border-radius: 8px; margin-top: 25px; font-weight: bold;
            color: #EBCB8B; font-size: 14px;
        }
        QGroupBox::title {
            subcontrol-origin: margin; subcontrol-position: top center;
            padding: 5px 15px; background-color: #4C566A;
            border-radius: 4px; color: #ECEFF4;
        }
        QTableWidget {
            background-color: transparent; color: #ECEFF4;
            border: none; gridline-color: #434C5E; font-size: 12px;
        }
        QHeaderView::section {
            background-color: #2E3440; color: #88C0D0;
            padding: 4px; border: none; border-bottom: 2px solid #4C566A;
            font-weight: bold;
        }
        QTableWidget::item { border-bottom: 1px solid #434C5E; }
        QProgressBar {
            border: none; border-radius: 3px; background-color: #2E3440;
            text-align: center; color: #ECEFF4; font-size: 11px;
        }
        QProgressBar::chunk { border-radius: 3px; }
    )");
}

void DashboardDialog::loadData() {
    if (!m_engine) return;

    QList<GraphNode> nodes = m_engine->getAllNodes(m_ontologyId);
    QList<GraphEdge> edges = m_engine->getAllRelationships(m_ontologyId);

    int totalNodes = nodes.size();
    int totalEdges = edges.size();

    m_lblTotalNodes->setText(QString::number(totalNodes));
    m_lblTotalEdges->setText(QString::number(totalEdges));

    // 计算度数最高的核心节点（关联关系最多的节点）
    QMap<int, int> nodeDegrees;
    for (const auto& edge : edges) {
        nodeDegrees[edge.sourceId]++;
        nodeDegrees[edge.targetId]++;
    }

    int maxDegree = 0;
    int maxNodeId = -1;
    for (auto it = nodeDegrees.begin(); it != nodeDegrees.end(); ++it) {
        if (it.value() > maxDegree) {
            maxDegree = it.value();
            maxNodeId = it.key();
        }
    }

    QString coreNodeName = "无数据";
    if (maxNodeId != -1) {
        for (const auto& node : nodes) {
            if (node.id == maxNodeId) {
                // 缩短超长名称
                coreNodeName = node.name.length() > 8 ? node.name.left(8) + "..." : node.name;
                break;
            }
        }
    }
    m_lblCoreNode->setText(coreNodeName);
    if(maxDegree > 0) {
        m_lblCoreNode->setToolTip(QString("该节点拥有 %1 条连接关系").arg(maxDegree));
    }

    // --- 统计节点分布 ---
    QMap<QString, int> nodeTypeCounts;
    for (const auto& node : nodes) nodeTypeCounts[node.nodeType]++;

    m_nodeChart->setData(nodeTypeCounts); // 渲染环形图
    m_nodeTable->setRowCount(nodeTypeCounts.size());

    int row = 0;
    // 修改后
    QStringList nordColors = QStringList() << "#88C0D0" << "#B48EAD" << "#EBCB8B" << "#A3BE8C" << "#D08770" << "#BF616A" << "#5E81AC" << "#81A1C1";
    for (auto it = nodeTypeCounts.begin(); it != nodeTypeCounts.end(); ++it) {
        m_nodeTable->setItem(row, 0, new QTableWidgetItem(it.key()));
        m_nodeTable->setItem(row, 1, new QTableWidgetItem(QString::number(it.value())));

        QProgressBar* pBar = new QProgressBar();
        pBar->setMaximum(totalNodes);
        pBar->setValue(it.value());
        QString barColor = nordColors[row % nordColors.size()];
        pBar->setStyleSheet(QString("QProgressBar::chunk { background-color: %1; }").arg(barColor));
        double percent = (totalNodes > 0) ? (it.value() * 100.0 / totalNodes) : 0;
        pBar->setFormat(QString::number(percent, 'f', 1) + "%");

        m_nodeTable->setCellWidget(row, 2, pBar);
        row++;
    }

    // --- 统计关系分布 ---
    QMap<QString, int> edgeTypeCounts;
    for (const auto& edge : edges) edgeTypeCounts[edge.relationType]++;

    m_edgeChart->setData(edgeTypeCounts); // 渲染环形图
    m_edgeTable->setRowCount(edgeTypeCounts.size());

    row = 0;
    for (auto it = edgeTypeCounts.begin(); it != edgeTypeCounts.end(); ++it) {
        m_edgeTable->setItem(row, 0, new QTableWidgetItem(it.key()));
        m_edgeTable->setItem(row, 1, new QTableWidgetItem(QString::number(it.value())));

        QProgressBar* pBar = new QProgressBar();
        pBar->setMaximum(totalEdges);
        pBar->setValue(it.value());
        QString barColor = nordColors[row % nordColors.size()];
        pBar->setStyleSheet(QString("QProgressBar::chunk { background-color: %1; }").arg(barColor));
        double percent = (totalEdges > 0) ? (it.value() * 100.0 / totalEdges) : 0;
        pBar->setFormat(QString::number(percent, 'f', 1) + "%");

        m_edgeTable->setCellWidget(row, 2, pBar);
        row++;
    }
}