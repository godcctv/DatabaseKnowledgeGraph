#include "VisualEdge.h"
#include "VisualNode.h"   // 必须引用，否则找不到 srcNode 的方法
#include "mainwindow.h"   // 🔥 必须引用，否则找不到 MainWindow 的方法
#include <QPainter>
#include <QMenu>
#include <QtMath>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QDebug>

VisualEdge::VisualEdge(int id, int sourceId, int targetId, QString type, VisualNode* srcNode, VisualNode* destNode)
    : m_id(id), m_sourceId(sourceId), m_targetId(targetId), m_relationType(type), m_srcNode(srcNode), m_destNode(destNode)
{
    setZValue(-1); // 保证线在球的下面
    // 允许选中，这样才能变红，才能触发删除
    setFlags(QGraphicsItem::ItemIsSelectable);
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
    // 创建一个较宽的路径用于碰撞检测（让鼠标更容易点中细线）
    QPainterPath path;
    path.moveTo(line().p1());
    path.lineTo(line().p2());

    QPainterPathStroker stroker;
    stroker.setWidth(10); // 感应宽度 10 像素
    return stroker.createStroke(path);
}

void VisualEdge::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    if (!m_srcNode || !m_destNode) return;
    if (m_srcNode->collidesWithItem(m_destNode)) return; // 如果球重叠了就不画线

    QPen myPen = pen();
    myPen.setColor(isSelected() ? Qt::red : Qt::black); // 选中变红
    myPen.setWidth(2);
    painter->setPen(myPen);
    painter->setBrush(Qt::NoBrush);

    QPointF srcPos = m_srcNode->scenePos();
    QPointF dstPos = m_destNode->scenePos();
    QLineF line(srcPos, dstPos);

    // --- 绘制贝塞尔曲线 (解决重叠) ---
    QPainterPath path;
    path.moveTo(srcPos);

    if (m_offset == 0) {
        // 直线
        path.lineTo(dstPos);
    } else {
        // 曲线
        QPointF center = line.center();
        double dx = line.dx();
        double dy = line.dy();
        double length = line.length();

        if (length > 0) {
            // 计算垂直方向的偏移控制点
            double normX = -dy / length;
            double normY = dx / length;
            QPointF controlPoint(center.x() + normX * m_offset,
                                 center.y() + normY * m_offset);
            path.quadTo(controlPoint, dstPos);
        } else {
            path.lineTo(dstPos);
        }
    }
    painter->drawPath(path);

    // --- 绘制文字 ---
    if (!m_relationType.isEmpty()) {
        // 计算文字位置（曲线的中点）
        QPointF textPos = path.pointAtPercent(0.5);

        painter->save();
        painter->translate(textPos);
        // 让文字跟随线条角度旋转
        double angle = line.angle();
        painter->rotate(-angle);
        if (angle > 90 && angle < 270) {
             painter->rotate(180); // 防止文字倒着显示
        }

        // 绘制文字背景和文字
        painter->setBrush(Qt::white); // 白底
        painter->setPen(Qt::black);
        painter->drawRect(QRectF(-30, -10, 60, 20)); // 简单的文字框
        painter->drawText(QRectF(-30, -10, 60, 20), Qt::AlignCenter, m_relationType);
        painter->restore();
    }
}

void VisualEdge::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
    // 1. 自动选中当前线
    setSelected(true);

    // 2. 创建菜单
    QMenu menu;
    QAction *deleteAction = menu.addAction("删除关系");

    // 3. 弹出菜单
    QAction *selectedAction = menu.exec(event->screenPos());

    // 4. 处理点击
    if (selectedAction == deleteAction) {
        if (scene()) {
            // 遍历所有视图，找到所属的主窗口
            foreach (QGraphicsView *view, scene()->views()) {
                // 尝试把视图的窗口转换成 MainWindow
                // 🔥 这里需要 mainwindow.h 的完整定义，否则报错 incomplete type
                MainWindow *window = qobject_cast<MainWindow*>(view->window());
                if (window) {
                    // 调用 MainWindow 的 public 函数
                    window->onActionDeleteRelationshipTriggered();
                    break;
                }
            }
        }
    }
}