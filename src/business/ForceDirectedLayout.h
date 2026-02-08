#ifndef FORCEDIRECTEDLAYOUT_H
#define FORCEDIRECTEDLAYOUT_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QPointF>
#include "../ui/VisualNode.h"
#include "../ui/VisualEdge.h"

class ForceDirectedLayout : public QObject {
    Q_OBJECT
public:
    explicit ForceDirectedLayout(QObject *parent = nullptr);

    void addNode(VisualNode* node);
    void addEdge(VisualEdge* edge);
    void removeNode(VisualNode* node);
    void removeEdge(VisualEdge* edge);
    void clear();

    // 核心计算函数
    void calculate();

private:
    QList<VisualNode*> m_nodes;
    QList<VisualEdge*> m_edges;

    // --- 物理参数 (必须在这里定义，.cpp 才能用) ---
    double m_stiffness;      // 弹性系数
    double m_repulsion;      // 斥力强度
    double m_damping;        // 阻尼
    double m_idealLength;    // 理想边长
    double m_centerAttraction; // 向心力
    double m_maxVelocity;    // 最大速度

    // 记录上一帧的力/位移
    QMap<VisualNode*, QPointF> m_displacements;
};

#endif // FORCEDIRECTEDLAYOUT_H