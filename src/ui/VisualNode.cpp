#include "VisualNode.h"
#include "VisualEdge.h"
#include <QBrush>
#include <QPen>
#include <QRadialGradient>
#include <QGraphicsTextItem>
#include <QGraphicsDropShadowEffect>
#include <QFont>
#include <QCursor>

// src/ui/VisualNode.cpp

VisualNode::VisualNode(int id, QString name, QString type, qreal x, qreal y)
    : m_id(id), m_name(name), m_nodeType(type)
{
    // 1. 设置几何大小
    int radius = 25;
    setRect(-radius, -radius, radius * 2, radius * 2);
    setPos(x, y);

    // 2. 确定主色调 (使用更现代的配色)
    QColor baseColor;
    if (type == "Concept") baseColor = QColor("#00C853");      // 鲜艳的翡翠绿
    else if (type == "Entity") baseColor = QColor("#2979FF");  // 鲜艳的谷歌蓝
    else baseColor = QColor("#FF6D00");                        // 活力橙

    // 3. 现代填充风格：线性微渐变 (比径向渐变更显平滑)
    // 从左上到右下，模拟柔和光照
    QLinearGradient gradient(-radius, -radius, radius, radius);
    gradient.setColorAt(0, baseColor.lighter(120));
    gradient.setColorAt(1, baseColor);
    setBrush(QBrush(gradient));

    // 4. 边框：白色半透明光圈 (增强深色背景下的对比度)
    QPen pen(QColor(255, 255, 255, 200));
    pen.setWidthF(1.5); // 使用浮点宽度更细腻
    setPen(pen);

    // 5. 阴影：发光效果 (Glow Effect) 而不是单纯的黑色投影
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(20); // 大半径模糊
    shadow->setOffset(0, 0);   // 居中发光
    shadow->setColor(baseColor.lighter(120)); // 使用同色系光晕，而不是黑色
    shadow->setEnabled(true);
    setGraphicsEffect(shadow);

    // 6. 交互设置
    setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsScenePositionChanges);
    setCursor(Qt::PointingHandCursor);
    setData(0, id);

    // 7. 文本设置
    QGraphicsTextItem *textItem = new QGraphicsTextItem(name, this);
    textItem->setFont(QFont("Microsoft YaHei", 9, QFont::Bold)); // 使用微软雅黑
    textItem->setDefaultTextColor(QColor(240, 240, 240)); // 几乎全白

    // 文本阴影 (黑色描边效果，防止文字在浅色背景看不清)
    QGraphicsDropShadowEffect *textShadow = new QGraphicsDropShadowEffect();
    textShadow->setBlurRadius(2);
    textShadow->setOffset(1, 1);
    textShadow->setColor(Qt::black);
    textItem->setGraphicsEffect(textShadow);

    // 文本居中逻辑
    QRectF textRect = textItem->boundingRect();
    textItem->setPos(-textRect.width() / 2, radius + 5); // 放在圆圈下方，而不是中间，更整洁
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