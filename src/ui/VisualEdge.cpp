#include "VisualEdge.h"
#include "VisualNode.h"
#include "mainwindow.h" // 为了调用删除接口
#include <QPainter>
#include <QMenu>
#include <QtMath>
#include <QDebug>

VisualEdge::VisualEdge(int id, int sourceId, int targetId, QString type, VisualNode* srcNode, VisualNode* destNode)
    : m_id(id), m_sourceId(sourceId), m_targetId(targetId), m_relationType(type), m_srcNode(srcNode), m_destNode(destNode)
{
    setZValue(-1); // 保证线在球的下面
    setPen(QPen(Qt::black, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    setFlags(QGraphicsItem::ItemIsSelectable); // 允许选中
    updatePosition();
}

void VisualEdge::updatePosition() {
    if (m_srcNode && m_destNode) {
        // 连接两个球的中心
        QLineF line(m_srcNode->scenePos(), m_destNode->scenePos());
        setLine(line);
    }
}

QPainterPath VisualEdge::shape() const {
    // 🔥 核心技巧：创建一个只有路径的“虚胖”形状用于碰撞检测
    // 虽然线只有 2px 宽，但我们告诉 Qt 这个物体有 10px 宽
    QPainterPath path;

    // 修正：直接使用 line() 的点，不需要 mapFromScene
    // 因为对于 LineItem 来说，line() 定义的就是本地坐标系下的形状
    path.moveTo(line().p1());
    path.lineTo(line().p2());
    
    QPainterPathStroker stroker;
    stroker.setWidth(10); // 感应宽度 10 像素
    return stroker.createStroke(path);
}

void VisualEdge::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    if (m_srcNode->collidesWithItem(m_destNode)) return; // 如果球重叠了就不画线

    QPen myPen = pen();
    myPen.setColor(isSelected() ? Qt::red : Qt::black); // 选中变红
    painter->setPen(myPen);
    painter->setBrush(isSelected() ? Qt::red : Qt::black);

    // 1. 画线
    QLineF centerLine(m_srcNode->scenePos(), m_destNode->scenePos());
    
    // 2. 计算箭头位置（要在目标球的边缘，不能插到球心里去）
    // 假设球半径是 25 (从 VisualNode 代码得知)
    double angle = std::atan2(-centerLine.dy(), centerLine.dx());
    // 目标点减去半径距离
    QPointF arrowP1 = centerLine.p2() - QPointF(sin(angle + M_PI / 2) * 0, cos(angle + M_PI / 2) * 0);
    // 这里简单处理：直接画到圆心，因为球体会盖住线头。
    // 如果想要完美的箭头，需要减去 VisualNode 的半径。
    
    painter->drawLine(centerLine);

    // 3. 画中间的文字 (关系类型)
    if (!m_relationType.isEmpty()) {
        QPointF midPoint = (centerLine.p1() + centerLine.p2()) / 2;
        painter->save();
        painter->translate(midPoint);
        painter->rotate(-centerLine.angle()); // 让文字跟着线旋转
        // 如果线反着画，文字会倒过来，这里可以加判断翻转文字
        if (centerLine.angle() > 90 && centerLine.angle() < 270) {
             painter->rotate(180);
        }
        painter->drawText(QRectF(-50, -20, 100, 20), Qt::AlignCenter, m_relationType);
        painter->restore();
    }
}

void VisualEdge::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
    // 选中自己
    setSelected(true);

    QMenu menu;
    QAction *deleteAction = menu.addAction("删除关系");
    
    QAction *selectedAction = menu.exec(event->screenPos());
    
    if (selectedAction == deleteAction) {

    }
}