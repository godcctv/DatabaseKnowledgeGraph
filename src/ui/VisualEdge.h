#ifndef VISUALEDGE_H
#define VISUALEDGE_H

#include <QGraphicsLineItem>
#include <QPen>
#include <QGraphicsSceneContextMenuEvent>

// 前向声明，告诉编译器 VisualNode 是个类，稍后再说细节
class VisualNode;

// 继承自 QGraphicsLineItem
class VisualEdge : public QGraphicsLineItem {
public:
    enum { Type = UserType + 2 }; // 自定义类型 ID

    // 构造函数
    VisualEdge(int id, int sourceId, int targetId, QString type, VisualNode* srcNode, VisualNode* destNode);

    int getId() const { return m_id; }
    int type() const override { return Type; }

    QRectF boundingRect() const override;
    // 更新线条位置
    void updatePosition();

    void setOffset(qreal value) {
        if (m_offset != value) {
            prepareGeometryChange();
            m_offset = value;
        }
    }

    VisualNode* getSourceNode() const { return m_srcNode; }
    VisualNode* getDestNode() const { return m_destNode; }

protected:
    // 重写绘制逻辑
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    // 重写形状逻辑 (加宽点击区域)
    QPainterPath shape() const override;
    // 右键菜单
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;

private:
    int m_id;
    int m_sourceId;
    int m_targetId;
    QString m_relationType;

    VisualNode* m_srcNode;
    VisualNode* m_destNode;

    qreal m_offset = 0;
};

#endif // VISUALEDGE_H