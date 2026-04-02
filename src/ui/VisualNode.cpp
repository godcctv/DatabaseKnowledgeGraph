#include "VisualNode.h"
#include "VisualEdge.h"
#include "mainwindow.h"
#include <QBrush>
#include <QPen>
#include <QRadialGradient>
#include <QGraphicsTextItem>
#include <QGraphicsDropShadowEffect>
#include <QFont>
#include <QCursor>
#include <QMenu>
#include <QGraphicsView>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneMouseEvent>
#include <QDateTime>
#include <QtMath>

VisualNode::VisualNode(int id, QString name, QString type, qreal x, qreal y)
    : m_id(id), m_name(name), m_nodeType(type)
{
    int radius = 35;
    setRect(-radius, -radius, radius * 2, radius * 2);
    setPos(x, y);

    // 交互设置
    setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsScenePositionChanges);
   setCacheMode(QGraphicsItem::NoCache);
    setZValue(1); // 保证星球在连线上方
    setCursor(Qt::PointingHandCursor);
    setData(0, id);

    // 文本设置 (名字保持不变)
    QGraphicsTextItem *textItem = new QGraphicsTextItem(name, this);
    textItem->setFont(QFont("Microsoft YaHei", 9, QFont::Bold)); // 使用微软雅黑
    textItem->setDefaultTextColor(QColor(240, 240, 240)); // 几乎全白

    // 给文字加个黑边投影，防止在星球的亮色背景上看不清
    QGraphicsDropShadowEffect *textShadow = new QGraphicsDropShadowEffect();
    textShadow->setBlurRadius(2);
    textShadow->setOffset(1, 1);
    textShadow->setColor(Qt::black);
    textItem->setGraphicsEffect(textShadow);

    // 把文字放在星球的下方，避免遮挡星球本体
    QRectF textRect = textItem->boundingRect();
    textItem->setPos(-textRect.width() / 2, 30);
}

void VisualNode::addEdge(QGraphicsLineItem* edge, bool isSource) {
    m_edges.append({edge, isSource});
    prepareGeometryChange();
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

void VisualNode::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
    // 1. 自动选中当前右键的节点
    setSelected(true);

    // 2. 创建并显示菜单
    QMenu menu;
    QAction *editAction = menu.addAction("修改节点");
    QAction *deleteAction = menu.addAction("删除节点");
    QAction *selectedAction = menu.exec(event->screenPos());

    if (selectedAction && scene()) { // 只要用户选了选项，并且在场景中
        foreach (QGraphicsView *view, scene()->views()) {
            MainWindow *window = qobject_cast<MainWindow*>(view->window());
            if (window) {
                if (selectedAction == deleteAction) {
                    window->onActionDeleteTriggered();
                } else if (selectedAction == editAction) {
                    window->onActionEditNodeTriggered(m_id);
                }
                break;
            }
        }
    }
}
void VisualNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setRenderHint(QPainter::Antialiasing);

    // 动态调整大小
    qreal coreRadius = 20 + getEdgeCount() * 1.5;
    if (coreRadius > 45) coreRadius = 45;

    // Nord 极客风的极光/冰雪调色板 (低饱和度)
    QStringList nordColors = {
        "#BF616A", // 红
        "#D08770", // 橙
        "#EBCB8B", // 黄
        "#A3BE8C", // 绿
        "#B48EAD", // 紫
        "#88C0D0", // 冰蓝
        "#81A1C1"  // 灰蓝
    };
    QColor baseColor(nordColors[m_id % nordColors.size()]);

    // ========== 1. 扁平化节点本体 ==========
    painter->setBrush(baseColor);
    painter->setPen(QPen(QColor("#ECEFF4"), 2)); // 白灰色实线描边
    painter->drawEllipse(QPointF(0, 0), coreRadius, coreRadius);

    // ========== 2. 选中状态指示器 ==========
    if (isSelected()) {
        painter->setBrush(Qt::NoBrush);
        // 使用标志性的冰蓝高亮色 (#88C0D0)
        painter->setPen(QPen(QColor("#88C0D0"), 3, Qt::SolidLine));
        painter->drawEllipse(QPointF(0, 0), coreRadius + 6, coreRadius + 6);
    }
}
int VisualNode::getMass() const {
    int seed = m_id * 137;
    int style = seed % 3; // 不用哈希，仅依赖ID生成样式
    int baseMass = 0;

    // 赋予不同类型初始基础质量
    if (style == 2) {
        baseMass = 10000; // 黑洞：质量天花板
    } else if (style == 0) {
        baseMass = 3000;  // 恒星：质量中等
    } else {
        baseMass = 500;   // 行星：质量最小
    }

    // 关系越多，质量越大（模拟质量累积）
    int edgeWeight = (style == 2) ? 1000 : 200;
    return baseMass + getEdgeCount() * edgeWeight;
}

void VisualNode::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
    if (scene()) {
        foreach (QGraphicsView *view, scene()->views()) {
            MainWindow *window = qobject_cast<MainWindow*>(view->window());
            if (window) {
                // 调用主窗口的公开方法来显示详情
                window->showNodeDetails(m_id);
                break;
            }
        }
    }
    QGraphicsEllipseItem::mouseDoubleClickEvent(event);
}

void VisualNode::updateData(QString newName, QString newType) {
    m_name = newName;
    m_nodeType = newType;
    // 遍历子项找到文字并更新
    for (QGraphicsItem* item : childItems()) {
        if (QGraphicsTextItem* textItem = dynamic_cast<QGraphicsTextItem*>(item)) {
            textItem->setPlainText(newName);
            // 重新居中对齐文字
            QRectF textRect = textItem->boundingRect();
            textItem->setPos(-textRect.width() / 2, 30);
            break;
        }
    }
    update(); // 重绘
}