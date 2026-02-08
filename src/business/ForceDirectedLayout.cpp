#include "ForceDirectedLayout.h"
#include <QtMath>
#include <QVector2D>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QDebug>

ForceDirectedLayout::ForceDirectedLayout(QObject *parent) : QObject(parent) {
    // 初始化参数
    m_stiffness = 0.08;      // 弹性系数 (拉力)
    m_repulsion = 20000.0;   // 斥力强度 (推力)
    m_damping = 0.90;        // 阻尼 (0.0-1.0, 越小停得越快)
    m_idealLength = 150.0;   // 理想边长
    m_centerAttraction = 0.02; // 向心力
    m_maxVelocity = 50.0;    // 最大速度
}

void ForceDirectedLayout::addNode(VisualNode* node) {
    if (!m_nodes.contains(node)) {
        m_nodes.append(node);
        m_displacements[node] = QPointF(0, 0);
        qDebug() << "Layout: Added node" << node->getName();
    }
}

void ForceDirectedLayout::addEdge(VisualEdge* edge) {
    if (!m_edges.contains(edge)) {
        m_edges.append(edge);
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

    // --- 调试输出: 证明算法正在运行 ---
    // 每 60 帧打印一次，避免刷屏
    static int frameCounter = 0;
    if (frameCounter++ > 60) {
        qDebug() << "Layout: Physics running for" << m_nodes.size() << "nodes...";
        frameCounter = 0;
    }

    // 1. 初始化位移
    for (VisualNode* node : m_nodes) {
        m_displacements[node] = QPointF(0, 0);
    }

    // 2. 计算斥力 (Repulsion) - 让所有节点互相远离
    for (int i = 0; i < m_nodes.size(); ++i) {
        for (int j = i + 1; j < m_nodes.size(); ++j) {
            VisualNode* u = m_nodes[i];
            VisualNode* v = m_nodes[j];

            QVector2D vec(u->pos() - v->pos());
            double dist = vec.length();

            if (dist < 1.0) {
                vec = QVector2D(1.0, 1.0); // 防止重叠除零
                dist = 1.0;
            }

            double force = m_repulsion / dist;
            QVector2D displacement = vec.normalized() * force;

            m_displacements[u] += displacement.toPointF();
            m_displacements[v] -= displacement.toPointF();
        }
    }

    // 3. 计算引力 (Attraction) - 拉近有连线的节点
    for (VisualEdge* edge : m_edges) {
        VisualNode* u = edge->getSourceNode();
        VisualNode* v = edge->getDestNode();

        if (!u || !v || u == v) continue;

        QVector2D vec(u->pos() - v->pos());
        double dist = vec.length();

        // 弹簧力：距离越远拉力越大，距离过近则产生推力
        double force = (dist - m_idealLength) * m_stiffness;

        QVector2D displacement = vec.normalized() * force;

        m_displacements[u] -= displacement.toPointF();
        m_displacements[v] += displacement.toPointF();
    }

    // 4. 应用位移
    QPointF center(0, 0);
    for (VisualNode* node : m_nodes) {
        // 如果正在拖拽，跳过物理计算
        if (node->scene() && node->scene()->mouseGrabberItem() == node) {
            continue;
        }

        // 向心力 (防止飞出屏幕)
        QVector2D vecToCenter(center - node->pos());
        m_displacements[node] += vecToCenter.toPointF() * m_centerAttraction;

        // 限制最大速度
        double len = QVector2D(m_displacements[node]).length();
        if (len > m_maxVelocity) {
            m_displacements[node] = (m_displacements[node] / len) * m_maxVelocity;
        }

        // 阻尼 (让它停下来)
        m_displacements[node] *= m_damping;

        // 更新位置
        if (len > 0.1) {
            node->setPos(node->pos() + m_displacements[node]);
        }
    }
}