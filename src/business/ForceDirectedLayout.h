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
    void calculate();

private:
    QList<VisualNode*> m_nodes;
    QList<VisualEdge*> m_edges;

    // 参数
    double m_stiffness = 0.08;      // 弹性系数
    double m_repulsion = 10000.0;   // 斥力强度
    double m_damping = 0.85;        // 阻尼
    double m_idealLength = 150.0;   // 理想边长
    double m_centerAttraction = 0.05; // 向心力
    double m_maxVelocity = 50.0;    // 速度限制

    QMap<VisualNode*, QPointF> m_displacements;
};

#endif