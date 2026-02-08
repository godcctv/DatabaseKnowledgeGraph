#ifndef FORCEDIRECTEDLAYOUT_H
#define FORCEDIRECTEDLAYOUT_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QPointF>
#include <QRectF>

// 引用 UI 类
#include "../ui/VisualNode.h"
#include "../ui/VisualEdge.h"

class ForceDirectedLayout : public QObject {
    Q_OBJECT
public:
    explicit ForceDirectedLayout(QObject *parent = nullptr);

    // 管理节点和边
    void addNode(VisualNode* node);
    void addEdge(VisualEdge* edge);
    void removeNode(VisualNode* node);
    void removeEdge(VisualEdge* edge);
    void clear();

    // 核心计算步骤（由 Timer 调用）
    void calculate();

    // 参数调整接口（留给未来 UI 调节用）
    void setStiffness(double k) { m_stiffness = k; }
    void setRepulsion(double r) { m_repulsion = r; }
    void setDamping(double d) { m_damping = d; }

private:
    QList<VisualNode*> m_nodes;
    QList<VisualEdge*> m_edges;

    // --- 物理参数调优 ---
    double m_stiffness = 0.05;    // 弹簧劲度系数 (引力)
    double m_repulsion = 20000.0; // 库仑力常数 (斥力)
    double m_damping = 0.90;      // 阻尼系数 (0.0-1.0)，模拟空气阻力，防止震荡
    double m_idealLength = 150.0; // 理想边长 (弹簧的自然长度)
    double m_centerAttraction = 0.02; // 向心力 (防止图漂移)
    double m_maxVelocity = 50.0;  // 最大移动速度限制 (防止爆炸)

    // 记录上一帧的力/位移
    QMap<VisualNode*, QPointF> m_displacements;
};

#endif // FORCEDIRECTEDLAYOUT_H