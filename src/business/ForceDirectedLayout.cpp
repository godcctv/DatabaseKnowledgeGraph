#include "ForceDirectedLayout.h"
#include <QtMath>
#include <QVector2D>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QDebug>

ForceDirectedLayout::ForceDirectedLayout(QObject *parent) : QObject(parent) {
    // --- 物理参数初始化 ---
    m_stiffness = 0.08;      // 弹性系数 (拉力): 越大线越紧
    m_repulsion = 20000.0;   // 斥力强度 (推力): 越大节点分得越开
    m_damping = 0.90;        // 阻尼 (0.0-1.0): 越小停得越快
    m_idealLength = 150.0;   // 理想边长
    m_centerAttraction = 0.02; // 向心力
    m_maxVelocity = 50.0;    // 最大速度限制
}

void ForceDirectedLayout::addNode(VisualNode* node) {
    if (!m_nodes.contains(node)) {
        m_nodes.append(node);
        m_displacements[node] = QPointF(0, 0);
        qDebug() << "Layout: Node added. Total nodes:" << m_nodes.size();
    }
}

void ForceDirectedLayout::addEdge(VisualEdge* edge) {
    if (!m_edges.contains(edge)) {
        m_edges.append(edge);
        qDebug() << "Layout: Edge added. Total edges:" << m_edges.size();
    }
}

void ForceDirectedLayout::removeNode(VisualNode* node) {
    m_nodes.removeOne(node);
    m_displacements.remove(node);
}

void ForceDirectedLayout::removeEdge(VisualEdge* edge) {
    m_edges.removeOne(edge);
}

void ForceDirectedLayout::clear() {
    m_nodes.clear();
    m_edges.clear();
    m_displacements.clear();
}

void ForceDirectedLayout::calculate() {
    if (m_nodes.isEmpty()) return;

    // --- 调试日志 (每 60 帧打印一次，证明算法在跑) ---
    static int frameCounter = 0;
    if (frameCounter++ > 60) {
        qDebug() << "Layout: Physics running... Nodes:" << m_nodes.size() << "Edges:" << m_edges.size();
        frameCounter = 0;
    }

    // 1. 初始化位移
    for (VisualNode* node : m_nodes) {
        m_displacements[node] = QPointF(0, 0);
    }

    // 2. 计算斥力 (Repulsion) - 所有节点之间
    for (int i = 0; i < m_nodes.size(); ++i) {
        for (int j = i + 1; j < m_nodes.size(); ++j) {
            VisualNode* u = m_nodes[i];
            VisualNode* v = m_nodes[j];

            QVector2D vec(u->pos() - v->pos());
            double dist = vec.length();

            // 防止重叠除零
            if (dist < 1.0) {
                vec = QVector2D(1.0, 1.0);
                dist = 1.0;
            }

            double force = m_repulsion / dist;
            QVector2D displacement = vec.normalized() * force;

            m_displacements[u] += displacement.toPointF();
            m_displacements[v] -= displacement.toPointF();
        }
    }

    // 3. 计算引力 (Attraction) - 仅在连接的边之间
    for (VisualEdge* edge : m_edges) {
        // 注意：请确保 VisualEdge 类中有 getSourceNode() 和 getDestNode() 方法
        // 如果你的方法名是 sourceNode()，请在这里修改
        VisualNode* u = edge->getSourceNode();
        VisualNode* v = edge->getDestNode();

        if (!u || !v || u == v) continue;

        QVector2D vec(u->pos() - v->pos());
        double dist = vec.length();

        double force = (dist - m_idealLength) * m_stiffness;
        QVector2D displacement = vec.normalized() * force;

        m_displacements[u] -= displacement.toPointF();
        m_displacements[v] += displacement.toPointF();
    }

    // 4. 应用位移
    QPointF center(0, 0);
    for (VisualNode* node : m_nodes) {
        // 如果用户正在拖拽，不要更新位置
        if (node->scene() && node->scene()->mouseGrabberItem() == node) {
            continue;
        }

        // 向心力
        QVector2D vecToCenter(center - node->pos());
        m_displacements[node] += vecToCenter.toPointF() * m_centerAttraction;

        // 限制最大速度
        double len = QVector2D(m_displacements[node]).length();
        if (len > m_maxVelocity) {
            m_displacements[node] = (m_displacements[node] / len) * m_maxVelocity;
        }

        // 阻尼
        m_displacements[node] *= m_damping;

        // 更新位置 (如果位移极小就忽略，节省性能)
        if (len > 0.1) {
            node->setPos(node->pos() + m_displacements[node]);
        }
    }
}