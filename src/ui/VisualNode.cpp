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
    int radius = 45;
    setRect(-radius, -radius, radius * 2, radius * 2);
    setPos(x, y);

    // 交互设置
    setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsScenePositionChanges);
    setCacheMode(DeviceCoordinateCache);    // 优化绘制性能
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
    textItem->setPos(-textRect.width() / 2, radius - 15);
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

    // 获取当前时间的毫秒数，用于驱动所有星体的自转和呼吸特效
    qint64 currentMs = QDateTime::currentMSecsSinceEpoch();

    qreal coreRadius = 18;
    int seed = m_id * 137;
    int styleType = seed % 3;    // 0: 恒星, 1: 带有星环的行星, 2: 黑洞
    int hue = (seed * 17) % 360; // 随机分配各种鲜艳的色相

    // 如果节点被选中，在最外围画一个科幻感的 HUD 扫描圈
    if (isSelected()) {
        painter->setPen(QPen(QColor(0, 229, 255, 180), 1.5, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(QPointF(0, 0), coreRadius * 2.5, coreRadius * 2.5);
    }

    if (styleType == 0) {
        // ================== 1. 恒星 (Star) ==================
        // 动态呼吸效应 (光晕放大缩小)
        double pulse = (sin(currentMs / 400.0 + seed) + 1.0) / 2.0;
        qreal glowRadius = coreRadius * 2.0 + pulse * 6.0;

        QRadialGradient grad(0, 0, glowRadius);
        QColor starColor = QColor::fromHsv(hue, 200, 255);

        grad.setColorAt(0, Qt::white);                               // 核心爆白
        grad.setColorAt(0.15, starColor.lighter(150));               // 内层炽热亮色
        grad.setColorAt(0.4, starColor);                             // 日冕层本色
        grad.setColorAt(0.7, QColor(starColor.red(), starColor.green(), starColor.blue(), 60));
        grad.setColorAt(1, Qt::transparent);                         // 边缘完全消散

        painter->setPen(Qt::NoPen);
        painter->setBrush(grad);
        painter->drawEllipse(QPointF(0, 0), glowRadius, glowRadius);

        // 动态恒星射线 (耀斑缓慢自转)
        painter->save();
        painter->rotate(currentMs / 40.0 + seed);
        painter->setPen(QPen(QColor(255, 255, 255, 60), 1.5));
        for (int i = 0; i < 6; ++i) {
            painter->drawLine(QPointF(-glowRadius*0.9, 0), QPointF(glowRadius*0.9, 0));
            painter->rotate(30);
        }
        painter->restore();

    } else if (styleType == 1) {
        // ================== 2. 行星 + 星环 (Planet with Rings) ==================
        QColor planetColor = QColor::fromHsv(hue, 160, 200);
        qreal tilt = (seed % 60) - 30; // 赋予随机的宇宙倾斜角度 (-30 到 30)

        painter->save();
        painter->rotate(tilt);

        // --- 1. 星环后半部分 (画在星球背面，解决突兀感的核心！) ---
        QRectF ringRect(-coreRadius * 2.6, -coreRadius * 0.6, coreRadius * 5.2, coreRadius * 1.2);
        QPen backRingPen(QColor(255, 255, 255, 50), 6);
        painter->setPen(backRingPen);
        // Qt的0度是3点钟，绘制上半圈(0~180度)代表后半部分
        painter->drawArc(ringRect, 0 * 16, 180 * 16);

        QPen backThinPen(planetColor.lighter(150), 1.5);
        painter->setPen(backThinPen);
        painter->drawArc(QRectF(-coreRadius * 2.2, -coreRadius * 0.45, coreRadius * 4.4, coreRadius * 0.9), 0 * 16, 180 * 16);

        painter->restore();

        // --- 2. 星球本体 (使用偏心渐变模拟单侧体积光照) ---
        QRadialGradient bodyGrad(-coreRadius * 0.3, -coreRadius * 0.3, coreRadius * 1.3);
        bodyGrad.setColorAt(0, planetColor.lighter(180)); // 迎光面高亮
        bodyGrad.setColorAt(0.5, planetColor);            // 明暗交界线
        bodyGrad.setColorAt(0.9, planetColor.darker(300));// 背光面极暗
        bodyGrad.setColorAt(1, QColor(0, 0, 0));

        painter->setPen(QPen(QColor(0, 0, 0, 150), 1));
        painter->setBrush(bodyGrad);
        painter->drawEllipse(QPointF(0, 0), coreRadius, coreRadius);

        // --- 3. 星环前半部分 (画在星球正面，完成3D遮挡穿插) ---
        painter->save();
        painter->rotate(tilt);

        QPen frontRingPen(QColor(255, 255, 255, 160), 6); // 正面光学更亮
        painter->setPen(frontRingPen);
        painter->drawArc(ringRect, 180 * 16, 180 * 16);   // 下半圈(180~360度)

        QPen frontThinPen(planetColor.lighter(200), 2);
        painter->setPen(frontThinPen);
        painter->drawArc(QRectF(-coreRadius * 2.2, -coreRadius * 0.45, coreRadius * 4.4, coreRadius * 0.9), 180 * 16, 180 * 16);

        painter->restore();

    } else {
        // ================== 3. 黑洞 (Gargantua Style Black Hole) ==================
        QColor diskColor = QColor::fromHsv(hue, 200, 255);
        qreal tilt = (seed % 40) - 20;

        painter->save();
        painter->rotate(tilt);

        // --- 1. 引力透镜光晕 (由于空间扭曲，我们在上下方都能看到吸积盘的光芒) ---
        // 动态呼吸光晕
        double holePulse = (sin(currentMs / 300.0 + seed) + 1.0) / 2.0;
        QRadialGradient lensGrad(0, 0, coreRadius * 2.6 + holePulse * 1.5);
        lensGrad.setColorAt(0.3, Qt::transparent);
        lensGrad.setColorAt(0.35, diskColor.lighter(150)); // 光子环边缘极亮
        lensGrad.setColorAt(0.6, QColor(diskColor.red(), diskColor.green(), diskColor.blue(), 120));
        lensGrad.setColorAt(1, Qt::transparent);
        painter->setPen(Qt::NoPen);
        painter->setBrush(lensGrad);
        painter->drawEllipse(QPointF(0, 0), coreRadius * 3.0, coreRadius * 3.0);

        // --- 2. 吸积盘后半部分 ---
        QRectF accretionRect(-coreRadius * 3.5, -coreRadius * 0.8, coreRadius * 7.0, coreRadius * 1.6);
        QPen backDiskPen(QColor(diskColor.red(), diskColor.green(), diskColor.blue(), 150), 8);
        painter->setPen(backDiskPen);
        painter->drawArc(accretionRect, 0 * 16, 180 * 16);

        painter->restore();

        // --- 3. 绝对黑体 (事件视界 Event Horizon) ---
        // 吞噬一切光线的极暗领域，带有一圈极细的白色光子逃逸层边界
        painter->setPen(QPen(QColor(255, 255, 255, 120), 1.0));
        painter->setBrush(Qt::black);
        painter->drawEllipse(QPointF(0, 0), coreRadius * 0.95, coreRadius * 0.95);

        // --- 4. 吸积盘前半部分 (正面挡住黑洞) ---
        painter->save();
        painter->rotate(tilt);

        QPen frontDiskPen(diskColor, 10);
        painter->setPen(frontDiskPen);
        painter->drawArc(accretionRect, 180 * 16, 180 * 16);

        // 吸积盘炽热核心明线
        QPen coreDiskPen(Qt::white, 2);
        painter->setPen(coreDiskPen);
        painter->drawArc(accretionRect, 180 * 16, 180 * 16);

        painter->restore();
    }
}