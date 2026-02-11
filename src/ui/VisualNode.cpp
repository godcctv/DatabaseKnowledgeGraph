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

    qreal coreRadius = 18;
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
        // ================== 1. 恒星 (Star - 质感等离子体) ==================
        double pulse = sin(currentMs / 600.0 + seed);
        qreal glowRadius = coreRadius * 1.8 + pulse * 1.5;

        QColor starColor = QColor::fromHsv(hue, 220, 255);

        // 1. 日冕层 (Corona) - 使用指数级衰减的渐变，真实感更强
        QRadialGradient coronaGrad(0, 0, glowRadius);
        coronaGrad.setColorAt(0, QColor(starColor.red(), starColor.green(), starColor.blue(), 120));
        coronaGrad.setColorAt(0.4, QColor(starColor.red(), starColor.green(), starColor.blue(), 40));
        coronaGrad.setColorAt(0.7, QColor(starColor.red(), starColor.green(), starColor.blue(), 10));
        coronaGrad.setColorAt(1, Qt::transparent);
        painter->setPen(Qt::NoPen);
        painter->setBrush(coronaGrad);
        painter->drawEllipse(QPointF(0, 0), glowRadius, glowRadius);

        // 2. 星球本体与光球层 (Photosphere)
        QRadialGradient coreGrad(0, 0, coreRadius * 1.1);
        coreGrad.setColorAt(0, Qt::white);                               // 核心绝对白炽
        coreGrad.setColorAt(0.3, starColor.lighter(150));                // 过渡带
        coreGrad.setColorAt(0.8, starColor);                             // 边缘真色
        coreGrad.setColorAt(1, Qt::transparent);
        painter->setBrush(coreGrad);
        painter->drawEllipse(QPointF(0, 0), coreRadius * 1.1, coreRadius * 1.1);

        // 3. 表面对流层动态效果 (缓慢自转的微妙亮斑，增加生命力但极度朴素)
        painter->save();
        painter->rotate(currentMs / 40.0 + seed);
        QConicalGradient plasmaGrad(0, 0, 0);
        plasmaGrad.setColorAt(0, QColor(255, 255, 255, 0));
        plasmaGrad.setColorAt(0.25, QColor(255, 255, 255, 50));
        plasmaGrad.setColorAt(0.5, QColor(255, 255, 255, 0));
        plasmaGrad.setColorAt(0.75, QColor(255, 255, 255, 50));
        plasmaGrad.setColorAt(1, QColor(255, 255, 255, 0));
        painter->setBrush(plasmaGrad);
        painter->drawEllipse(QPointF(0, 0), coreRadius * 0.95, coreRadius * 0.95);
        painter->restore();

    } else if (styleType == 1) {
        // ================== 2. 行星 (Planet - 体积光照与菲涅尔高光) ==================
        QColor planetColor = QColor::fromHsv(hue, 160, 200);
        qreal tilt = (seed % 60) - 30;

        painter->save();
        painter->rotate(tilt);

        // 1. 大气层外发光 (Atmospheric Scattering)
        QRadialGradient atmoGrad(-coreRadius * 0.2, -coreRadius * 0.2, coreRadius * 1.3);
        QColor atmoColor = planetColor.lighter(130);
        atmoGrad.setColorAt(0.4, QColor(atmoColor.red(), atmoColor.green(), atmoColor.blue(), 60));
        atmoGrad.setColorAt(0.8, QColor(atmoColor.red(), atmoColor.green(), atmoColor.blue(), 15));
        atmoGrad.setColorAt(1, Qt::transparent);

        painter->setPen(Qt::NoPen);
        painter->setBrush(atmoGrad);
        painter->drawEllipse(QPointF(0, 0), coreRadius * 1.3, coreRadius * 1.3);

        // 2. 行星本体 (逼真的体积阴影，呈现完美的球体质感)
        QRadialGradient bodyGrad(-coreRadius * 0.35, -coreRadius * 0.35, coreRadius * 1.4);
        bodyGrad.setColorAt(0, planetColor.lighter(150)); // 高光点
        bodyGrad.setColorAt(0.3, planetColor);            // 受光面
        bodyGrad.setColorAt(0.65, planetColor.darker(300));// 晨昏线，急剧变暗
        bodyGrad.setColorAt(0.9, QColor(5, 5, 10));       // 暗面，融入背景
        bodyGrad.setColorAt(1, QColor(0, 0, 0));

        painter->setPen(QPen(QColor(0, 0, 0, 100), 0.5)); // 边缘抗锯齿微调
        painter->setBrush(bodyGrad);
        painter->drawEllipse(QPointF(0, 0), coreRadius, coreRadius);

        // 3. 菲涅尔高光 (Fresnel Rim Light) 增加晶莹剔透的通透感
        QRadialGradient rimGrad(-coreRadius * 0.5, -coreRadius * 0.5, coreRadius * 1.1);
        rimGrad.setColorAt(0, QColor(255, 255, 255, 80));
        rimGrad.setColorAt(0.4, Qt::transparent);
        painter->setBrush(rimGrad);
        painter->drawEllipse(QPointF(0, 0), coreRadius, coreRadius);

        painter->restore();

    } else {
        // ================== 3. 黑洞 (Black Hole - 极简光子圈，保持原样) ==================
        QColor glowColor = QColor::fromHsv(hue, 150, 255);

        double holePulse = sin(currentMs / 500.0 + seed);
        qreal lensRadius = coreRadius * 1.5 + holePulse * 1.5;

        // 微弱的引力透镜发光
        QRadialGradient lensGrad(0, 0, lensRadius);
        lensGrad.setColorAt(0.4, Qt::transparent);
        lensGrad.setColorAt(0.5, Qt::white);
        lensGrad.setColorAt(0.65, glowColor.lighter(120));
        lensGrad.setColorAt(1, Qt::transparent);

        painter->setPen(Qt::NoPen);
        painter->setBrush(lensGrad);
        painter->drawEllipse(QPointF(0, 0), lensRadius, lensRadius);

        // 纯净的绝对视界黑体
        painter->setPen(QPen(QColor(255, 255, 255, 80), 1.0));
        painter->setBrush(Qt::black);
        painter->drawEllipse(QPointF(0, 0), coreRadius * 0.85, coreRadius * 0.85);
    }
}