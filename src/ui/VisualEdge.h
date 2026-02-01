#ifndef VISUALEDGE_H
#define VISUALEDGE_H

#include <QGraphicsLineItem>
#include <QPen>
#include <QGraphicsSceneContextMenuEvent>

// 前向声明
class VisualNode;

class VisualEdge : public QGraphicsLineItem {
public:
    enum { Type = UserType + 2 }; // 自定义类型 ID

    // 构造函数：需要知道 ID，起点和终点节点
    VisualEdge(int id, int sourceId, int targetId, QString type, VisualNode* srcNode, VisualNode* destNode);

    int getId() const { return m_id; }
    int type() const override { return Type; }

    // 更新线条位置（当节点移动时调用）
    void updatePosition();
    VisualNode* getSourceNode() const { return m_srcNode; }
    VisualNode* getDestNode() const { return m_destNode; }

protected:
    // 1. 重写绘制：画出箭头
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    // 2. 重写形状：让点击区域变宽（防抖动，容易选中）
    QPainterPath shape() const override;

    // 3. 右键菜单事件
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;

private:
    int m_id;
    int m_sourceId;
    int m_targetId;
    QString m_relationType;

    VisualNode* m_srcNode;
    VisualNode* m_destNode;
};

#endif // VISUALEDGE_H