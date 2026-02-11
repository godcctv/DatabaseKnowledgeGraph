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
    QAction *deleteAction = menu.addAction("删除节点");
    QAction *selectedAction = menu.exec(event->screenPos());

    // 3. 执行删除指令
    if (selectedAction == deleteAction) {
        if (scene()) {
            foreach (QGraphicsView *view, scene()->views()) {
                MainWindow *window = qobject_cast<MainWindow*>(view->window());
                if (window) {
                    // 调用主窗口的删除操作
                    window->onActionDeleteTriggered();
                    break;
                }
            }
        }
    }
}
void VisualNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setRenderHint(QPainter::Antialiasing);

    qint64 currentMs = QDateTime::currentMSecsSinceEpoch();

    // 根据直接关联的节点数量，动态计算基础体积
    qreal coreRadius = 14 + getEdgeCount() * 2.0;
    if (coreRadius > 40) coreRadius = 40;

    int seed = m_id * 137;
    int styleType = seed % 3;    // 0: 恒星, 1: 行星, 2: 黑洞
    int hue = (seed * 17) % 360;

    // ========== 选中状态光圈 ==========
    if (isSelected()) {
        painter->save();
        painter->rotate(currentMs / 40.0);
        painter->setPen(QPen(QColor(0, 255, 255, 180), 1.5, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(QPointF(0, 0), coreRadius * 1.6, coreRadius * 1.6);
        painter->restore();
    }

    if (styleType == 0) {
        // ================== 1. 恒星 (Star - 极简呼吸) ==================
        double pulse = sin(currentMs / 500.0 + seed);
        qreal glowRadius = coreRadius * 1.5 + pulse * (coreRadius * 0.1);

        QColor starColor = QColor::fromHsv(hue, 200, 255);

        QRadialGradient outerGlow(0, 0, glowRadius);
        outerGlow.setColorAt(0, QColor(starColor.red(), starColor.green(), starColor.blue(), 80));
        outerGlow.setColorAt(1, Qt::transparent);
        painter->fillRect(QRectF(-glowRadius, -glowRadius, glowRadius*2, glowRadius*2), outerGlow);

        QRadialGradient coreGrad(0, 0, coreRadius);
        coreGrad.setColorAt(0, Qt::white);
        coreGrad.setColorAt(0.4, starColor.lighter(120));
        coreGrad.setColorAt(0.9, starColor);
        coreGrad.setColorAt(1, Qt::transparent);

        painter->setPen(Qt::NoPen);
        painter->setBrush(coreGrad);
        painter->drawEllipse(QPointF(0, 0), coreRadius, coreRadius);

    } else if (styleType == 1) {
        // ================== 2. 行星 (Planet - 干净明暗) ==================
        QColor planetColor = QColor::fromHsv(hue, 160, 200);
        qreal tilt = (seed % 60) - 30;

        painter->save();
        painter->rotate(tilt);

        QRadialGradient bodyGrad(-coreRadius * 0.3, -coreRadius * 0.3, coreRadius * 1.5);
        bodyGrad.setColorAt(0, planetColor.lighter(150));
        bodyGrad.setColorAt(0.5, planetColor);
        bodyGrad.setColorAt(0.85, planetColor.darker(250));
        bodyGrad.setColorAt(1, QColor(0, 0, 0));

        painter->setPen(QPen(QColor(0, 0, 0, 150), 1));
        painter->setBrush(bodyGrad);
        painter->drawEllipse(QPointF(0, 0), coreRadius, coreRadius);

        painter->restore();

    } else {
        // ================== 3. 黑洞 (Black Hole - 完美光子圈恢复版) ==================
        QColor glowColor = QColor::fromHsv(hue, 150, 255);
        double holePulse = sin(currentMs / 500.0 + seed);

        // 光晕拉大，给光子环充足展示空间
        qreal lensRadius = coreRadius * 1.5 + holePulse * (coreRadius * 0.15);

        QRadialGradient lensGrad(0, 0, lensRadius);

        // 精准计算白环位置，保证绝对视界的纯黑部分不遮挡最耀眼的光子球
        lensGrad.setColorAt(0.35, Qt::transparent);
        lensGrad.setColorAt(0.42, Qt::white);  // 极其明亮锐利的光子环！
        lensGrad.setColorAt(0.6, glowColor.lighter(120));
        lensGrad.setColorAt(1, Qt::transparent);

        painter->setPen(Qt::NoPen);
        painter->setBrush(lensGrad);
        painter->drawEllipse(QPointF(0, 0), lensRadius, lensRadius);

        // 纯净的绝对视界黑体，半径为 0.85 * coreRadius
        painter->setPen(QPen(QColor(255, 255, 255, 100), 1.0));
        painter->setBrush(Qt::black);
        painter->drawEllipse(QPointF(0, 0), coreRadius * 0.85, coreRadius * 0.85);
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