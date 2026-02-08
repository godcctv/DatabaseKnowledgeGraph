#include "ForceDirectedLayout.h"
#include <QtMath>
#include <QVector2D>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QDebug>

ForceDirectedLayout::ForceDirectedLayout(QObject *parent) : QObject(parent) {}

void ForceDirectedLayout::addNode(VisualNode* node) {
    if (!m_nodes.contains(node)) {
        m_nodes.append(node);
        // 初始化位移为0
        m_displacements[node] = QPointF(0, 0);
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

    // 1. 初始化所有节点的受力（位移向量）为 0
    for (VisualNode* node : m_nodes) {
        m_displacements[node] = QPointF(0, 0);
    }

    // ============================
    // 2. 计算斥力 (Repulsion) - 库仑力
    // 作用于所有节点对之间，让节点互相远离
    // ============================
    for (int i = 0; i < m_nodes.size(); ++i) {
        for (int j = i + 1; j < m_nodes.size(); ++j) {
            VisualNode* u = m_nodes[i];
            VisualNode* v = m_nodes[j];

            QVector2D vec(u->pos() - v->pos());
            double dist = vec.length();

            // 防止重叠导致的除零错误，给一个极小随机扰动
            if (dist < 0.1) {
                vec = QVector2D(1.0, 1.0);
                dist = 0.1;
            }

            // 公式：F = k / dist (简单反比) 或 F = k / dist^2 (物理库仑力)
            // 这里使用 F = k / dist 效果比较平滑
            double force = m_repulsion / dist;
            QVector2D displacement = vec.normalized() * force;

            m_displacements[u] += displacement.toPointF();
            m_displacements[v] -= displacement.toPointF();
        }
    }

    // ============================
    // 3. 计算引力 (Attraction) - 弹簧力
    // 仅作用于有边连接的节点之间
    // ============================
    for (VisualEdge* edge : m_edges) {
        VisualNode* u = edge->getSourceNode();
        VisualNode* v = edge->getDestNode();

        if (!u || !v || u == v) continue;

        QVector2D vec(u->pos() - v->pos()); // 指向 u 的向量（u 相对于 v）
        double dist = vec.length();

        // 胡克定律：F = k * (当前长度 - 理想长度)
        // 如果 dist > idealLength，产生拉力 (拉近)
        // 如果 dist < idealLength，产生推力 (推远，防止重叠)
        double force = (dist - m_idealLength) * m_stiffness;

        QVector2D displacement = vec.normalized() * force;

        // u 被拉向 v (反向)
        m_displacements[u] -= displacement.toPointF();
        // v 被拉向 u (正向)
        m_displacements[v] += displacement.toPointF();
    }

    // ============================
    // 4. 计算向心力 (Central Gravity)
    // 类似于重力，让孤立节点也稍微向屏幕中心靠拢，防止飞出视野
    // ============================
    QPointF center(0, 0); // 场景中心
    for (VisualNode* node : m_nodes) {
        QVector2D vecToCenter(center - node->pos());
        // 距离越远，向心力越大
        m_displacements[node] += vecToCenter.toPointF() * m_centerAttraction;
    }

    // ============================
    // 5. 应用位移 (Integration)
    // ============================
    for (VisualNode* node : m_nodes) {
        // 【关键】检测用户交互
        // 如果鼠标正在拖拽这个节点，则布局算法暂时放弃对该节点的控制
        if (node->scene() && node->scene()->mouseGrabberItem() == node) {
            continue;
        }

        QPointF disp = m_displacements[node];

        // 限制最大速度 (防止这一帧突然飞太远，造成爆炸效果)
        double len = QVector2D(disp).length();
        if (len > m_maxVelocity) {
            disp = (disp / len) * m_maxVelocity;
        }

        QPointF newPos = node->pos() + disp * m_damping;
        node->setPos(newPos);
    }
}