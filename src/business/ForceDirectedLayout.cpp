#include "ForceDirectedLayout.h"
#include <QtMath>
#include <QVector2D>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QDebug>

ForceDirectedLayout::ForceDirectedLayout(QObject *parent) : QObject(parent) {
    // --- 物理参数初始化 ---
    m_stiffness = 0.08;
    m_repulsion = 800.0;
    m_damping = 0.85;
    m_idealLength = 120.0;
    m_centerAttraction = 0.04;
    m_maxVelocity = 30.0;
    m_orbitSpeed = 1.0;
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


    // 初始化位移
    for (VisualNode* node : m_nodes) {
        m_displacements[node] = QPointF(0, 0);
    }

    // 计算斥力  - 所有节点之间
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

    // 计算引力- 仅在连接的边之间
    for (VisualEdge* edge : m_edges) {
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

    for (VisualNode* u : m_nodes) {
        VisualNode* maxMassNeighbor = nullptr;
        int maxMass = -1;

        // 寻找相连的、质量最大的邻居节点
        for (VisualEdge* edge : m_edges) {
            VisualNode* neighbor = nullptr;
            if (edge->getSourceNode() == u) {
                neighbor = edge->getDestNode();
            } else if (edge->getDestNode() == u) {
                neighbor = edge->getSourceNode();
            }

            if (neighbor) {
                int neighborMass = neighbor->getMass();
                if (neighborMass > maxMass) {
                    maxMass = neighborMass;
                    maxMassNeighbor = neighbor;
                }
            }
        }

        // 引力捕获并公转
        if (maxMassNeighbor && maxMass > u->getMass()) {
            QVector2D vec(u->pos() - maxMassNeighbor->pos());
            double dist = vec.length();
            if (dist > 1.0) {
                QVector2D tangent(-vec.y(), vec.x());
                tangent.normalize();

                // 使用动态参数
                m_displacements[u] += (tangent * m_orbitSpeed).toPointF();
            }
        }
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