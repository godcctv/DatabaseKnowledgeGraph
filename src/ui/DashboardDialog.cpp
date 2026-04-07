#include "DashboardDialog.h"
#include "../business/QueryEngine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QProgressBar>
#include <QMap>

DashboardDialog::DashboardDialog(int ontologyId, QueryEngine* engine, QWidget *parent)
    : QDialog(parent), m_ontologyId(ontologyId), m_engine(engine) 
{
    setWindowTitle("项目数据仪表盘 (Dashboard)");
    resize(650, 500);

    setupUI();
    loadData();
}

DashboardDialog::~DashboardDialog() {}

void DashboardDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(25, 25, 25, 25);
    mainLayout->setSpacing(20);

    // --- 1. 核心数据概览区 ---
    QGroupBox* summaryGroup = new QGroupBox("📊 图谱规模概览", this);
    QHBoxLayout* summaryLayout = new QHBoxLayout(summaryGroup);
    
    m_lblTotalNodes = new QLabel("总节点数: 0", this);
    m_lblTotalEdges = new QLabel("总关系数: 0", this);
    
    // 设置醒目的大字体
    QString statStyle = "font-size: 20px; font-weight: bold; color: #88C0D0; padding: 10px;";
    m_lblTotalNodes->setStyleSheet(statStyle);
    m_lblTotalEdges->setStyleSheet(statStyle);
    m_lblTotalNodes->setAlignment(Qt::AlignCenter);
    m_lblTotalEdges->setAlignment(Qt::AlignCenter);

    summaryLayout->addWidget(m_lblTotalNodes);
    summaryLayout->addWidget(m_lblTotalEdges);
    mainLayout->addWidget(summaryGroup);

    // --- 2. 详细分布表格区 ---
    QHBoxLayout* tablesLayout = new QHBoxLayout();
    
    // 节点类型分布
    QGroupBox* nodeGroup = new QGroupBox("🔵 节点类型分布", this);
    QVBoxLayout* nodeLayout = new QVBoxLayout(nodeGroup);
    m_nodeTable = new QTableWidget(this);
    m_nodeTable->setColumnCount(3);
    m_nodeTable->setHorizontalHeaderLabels({"类型", "数量", "占比"});
    m_nodeTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_nodeTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_nodeTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_nodeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_nodeTable->setSelectionMode(QAbstractItemView::NoSelection);
    nodeLayout->addWidget(m_nodeTable);
    
    // 关系类型分布
    QGroupBox* edgeGroup = new QGroupBox("🔗 关系类型分布", this);
    QVBoxLayout* edgeLayout = new QVBoxLayout(edgeGroup);
    m_edgeTable = new QTableWidget(this);
    m_edgeTable->setColumnCount(3);
    m_edgeTable->setHorizontalHeaderLabels({"类型", "数量", "占比"});
    m_edgeTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_edgeTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_edgeTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_edgeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_edgeTable->setSelectionMode(QAbstractItemView::NoSelection);
    edgeLayout->addWidget(m_edgeTable);

    tablesLayout->addWidget(nodeGroup);
    tablesLayout->addWidget(edgeGroup);
    mainLayout->addLayout(tablesLayout);

    // --- Nord 主题样式 ---
    this->setStyleSheet(R"(
        QDialog { background-color: #2E3440; color: #D8DEE9; }
        QGroupBox { 
            border: 1px solid #4C566A; border-radius: 6px; 
            margin-top: 12px; font-weight: bold; color: #EBCB8B; 
            font-size: 14px;
        }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }
        QTableWidget { 
            background-color: #3B4252; color: #ECEFF4; 
            border: none; gridline-color: #434C5E; font-size: 13px;
        }
        QHeaderView::section { 
            background-color: #2E3440; color: #88C0D0; 
            padding: 6px; border: none; border-bottom: 1px solid #4C566A; 
            font-weight: bold;
        }
        QProgressBar {
            border: 1px solid #4C566A; border-radius: 3px;
            background-color: #2E3440; text-align: center; color: #ECEFF4;
        }
        QProgressBar::chunk { background-color: #81A1C1; width: 10px; }
    )");
}

void DashboardDialog::loadData() {
    if (!m_engine) return;

    // 1. 获取基础数据
    QList<GraphNode> nodes = m_engine->getAllNodes(m_ontologyId);
    QList<GraphEdge> edges = m_engine->getAllRelationships(m_ontologyId);

    int totalNodes = nodes.size();
    int totalEdges = edges.size();

    m_lblTotalNodes->setText(QString("总节点数: %1").arg(totalNodes));
    m_lblTotalEdges->setText(QString("总关系数: %1").arg(totalEdges));

    // 2. 统计节点类型分布
    QMap<QString, int> nodeTypeCounts;
    for (const auto& node : nodes) {
        nodeTypeCounts[node.nodeType]++;
    }

    m_nodeTable->setRowCount(nodeTypeCounts.size());
    int row = 0;
    for (auto it = nodeTypeCounts.begin(); it != nodeTypeCounts.end(); ++it) {
        m_nodeTable->setItem(row, 0, new QTableWidgetItem(it.key()));
        m_nodeTable->setItem(row, 1, new QTableWidgetItem(QString::number(it.value())));
        
        // 使用进度条显示占比可视化
        QProgressBar* pBar = new QProgressBar();
        pBar->setMaximum(totalNodes);
        pBar->setValue(it.value());
        double percent = (totalNodes > 0) ? (it.value() * 100.0 / totalNodes) : 0;
        pBar->setFormat(QString::number(percent, 'f', 1) + "%");
        m_nodeTable->setCellWidget(row, 2, pBar);
        row++;
    }

    // 3. 统计关系类型分布
    QMap<QString, int> edgeTypeCounts;
    for (const auto& edge : edges) {
        edgeTypeCounts[edge.relationType]++;
    }

    m_edgeTable->setRowCount(edgeTypeCounts.size());
    row = 0;
    for (auto it = edgeTypeCounts.begin(); it != edgeTypeCounts.end(); ++it) {
        m_edgeTable->setItem(row, 0, new QTableWidgetItem(it.key()));
        m_edgeTable->setItem(row, 1, new QTableWidgetItem(QString::number(it.value())));
        
        QProgressBar* pBar = new QProgressBar();
        pBar->setMaximum(totalEdges);
        pBar->setValue(it.value());
        // 关系进度条换个颜色以示区分
        pBar->setStyleSheet("QProgressBar::chunk { background-color: #B48EAD; }"); 
        double percent = (totalEdges > 0) ? (it.value() * 100.0 / totalEdges) : 0;
        pBar->setFormat(QString::number(percent, 'f', 1) + "%");
        m_edgeTable->setCellWidget(row, 2, pBar);
        row++;
    }
}