#include "VisualNode.h"
#include "VisualEdge.h"
#include <QBrush>
#include <QPen>
#include <QRadialGradient>
#include <QGraphicsTextItem>
#include <QGraphicsDropShadowEffect>
#include <QFont>
#include <QCursor>

VisualNode::VisualNode(int id, QString name, QString type, qreal x, qreal y)
    : m_id(id), m_name(name), m_nodeType(type)
{
    // 1. 设置几何形状
    setRect(-25, -25, 50, 50);
    
    // 2. 设置位置
    setPos(x, y);
    // 3. 设置 3D 外观
    QColor baseColor(Qt::cyan);
    if (type == "Concept") baseColor = QColor("#2ecc71");
    else if (type == "Entity") baseColor = QColor("#3498db");

    // 光照效果
    QRadialGradient gradient(-10, -10, 25, -10, -10); // 稍微偏移光照点
    gradient.setColorAt(0, baseColor.lighter(150));
    gradient.setColorAt(0.3, baseColor);
    gradient.setColorAt(1, baseColor.darker(150));
    setBrush(QBrush(gradient));
    setPen(Qt::NoPen);

    // 4. 添加阴影
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(15);
    shadow->setOffset(5, 5);
    shadow->setColor(QColor(0, 0, 0, 100));
    setGraphicsEffect(shadow);

    // 设置可交互
    setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsScenePositionChanges);
    setCursor(Qt::PointingHandCursor);
    setData(0, id); // 兼容旧代码查找

    // 因为设置了 parentItem (this)，文字会自动跟随球体移动！
    QGraphicsTextItem *textItem = new QGraphicsTextItem(name, this);
    textItem->setDefaultTextColor(Qt::white);
    
    // 居中文字 (简单算法：向左上偏移文字宽高的一半)
    QRectF textRect = textItem->boundingRect();
    textItem->setPos(-textRect.width() / 2, -textRect.height() / 2);
    
    // 给文字加一点阴影，防止看不清
    QGraphicsDropShadowEffect *textShadow = new QGraphicsDropShadowEffect();
    textShadow->setBlurRadius(1);
    textShadow->setOffset(1, 1);
    textShadow->setColor(Qt::black);
    textItem->setGraphicsEffect(textShadow);
}

void VisualNode::addEdge(QGraphicsLineItem* edge, bool isSource) {
    m_edges.append({edge, isSource});
}

QVariant VisualNode::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == ItemPositionChange && scene()) {
        for (auto& edgeInfo : m_edges) {
            VisualEdge* vEdge = dynamic_cast<VisualEdge*>(edgeInfo.line);
            if (vEdge) {
                vEdge->updatePosition();
            } else {
                QLineF line = edgeInfo.line->line();
                if (edgeInfo.isSource) line.setP1(value.toPointF());
                else line.setP2(value.toPointF());
                edgeInfo.line->setLine(line);
            }
        }
    }
    return QGraphicsEllipseItem::itemChange(change, value);
}

void VisualNode::removeEdge(QGraphicsLineItem* edge) {
    // 使用 erase-remove 惯用语从列表中移除死掉的线
    // 注意：这里的 m_edges 是我们在 VisualNode.h 里定义的那个 struct 列表
    for (int i = 0; i < m_edges.size(); ++i) {
        if (m_edges[i].line == edge) {
            m_edges.removeAt(i);
            break; // 找到并移除后退出
        }
    }
}