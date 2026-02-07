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
    QPainterPath path;
    QLineF l = line(); // 获取当前的线段
    path.moveTo(l.p1());

    if (m_offset == 0) {
        // 直线情况
        path.lineTo(l.p2());
    } else {
        // 曲线情况：必须复制 paint() 里的计算逻辑
        QPointF center = l.center();
        double dx = l.dx();
        double dy = l.dy();
        double length = l.length();

        if (length > 0) {
            // 计算完全相同的控制点
            double normX = -dy / length;
            double normY = dx / length;
            QPointF controlPoint(center.x() + normX * m_offset,
                                 center.y() + normY * m_offset);

            // 使用和绘制时一样的贝塞尔路径
            path.quadTo(controlPoint, l.p2());
        } else {
            path.lineTo(l.p2());
        }
    }

    // 创建一个较宽的“点击感应区”（10像素宽）
    QPainterPathStroker stroker;
    stroker.setWidth(10);
    return stroker.createStroke(path);
}

void VisualEdge::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    if (!m_srcNode || !m_destNode) return;
    if (m_srcNode->collidesWithItem(m_destNode)) return; // 如果球重叠了就不画线

    QColor normalColor("#A0A0A0");   // 浅灰色 (未选中)
    QColor selectedColor("#00E5FF"); // 青色高亮 (选中)

    QPen myPen = pen();
    myPen.setColor(isSelected() ? selectedColor : normalColor);
    myPen.setWidthF(1.5); //稍微细一点，更精致
    myPen.setStyle(Qt::DashLine);

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
        painter->setBrush(QColor(30, 30, 30, 200)); // 半透明深黑
        painter->setPen(QPen(normalColor, 1));      // 边框
        QRectF bgRect(-30, -10, 60, 20);
        painter->drawRoundedRect(bgRect, 10, 10);   // 圆角矩形

        // 文字颜色
        painter->setPen(Qt::white);
        painter->drawText(bgRect, Qt::AlignCenter, m_relationType);

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

QRectF VisualEdge::boundingRect() const {
    // 1. 获取线条/曲线的基础边界
    QRectF rect = shape().boundingRect();

    // 2. 如果有文字，必须把文字框也算进去！
    if (!m_relationType.isEmpty()) {
        // 重新计算文字的位置（逻辑必须和 paint 中保持一致）
        QPainterPath path;
        QLineF l = line();
        path.moveTo(l.p1());

        if (m_offset == 0) {
            path.lineTo(l.p2());
        } else {
            // 曲线逻辑
            QPointF center = l.center();
            double dx = l.dx();
            double dy = l.dy();
            double length = l.length();
            if (length > 0) {
                double normX = -dy / length;
                double normY = dx / length;
                QPointF controlPoint(center.x() + normX * m_offset, center.y() + normY * m_offset);
                path.quadTo(controlPoint, l.p2());
            } else {
                path.lineTo(l.p2());
            }
        }

        // 获取中点
        QPointF textPos = path.pointAtPercent(0.5);

        double textWidth = 80;  // 稍微宽一点
        double textHeight = 40; // 稍微高一点
        QRectF textRect(textPos.x() - textWidth/2, textPos.y() - textHeight/2, textWidth, textHeight);


        rect = rect.united(textRect);
    }

    return rect;
}