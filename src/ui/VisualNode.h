#ifndef VISUALNODE_H
#define VISUALNODE_H

#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QList>

// 继承自 QGraphicsEllipseItem，让我们自定义节点的行为
class VisualNode : public QGraphicsEllipseItem {
public:
    // 构造函数：传入ID、名字、类型和位置
    VisualNode(int id, QString name, QString type, qreal x, qreal y);

    // isSource: 如果是 true，说明这个节点是线的起点；false 则是终点
    void addEdge(QGraphicsLineItem* edge, bool isSource);

    void removeEdge(QGraphicsLineItem* edge);

    // 获取节点 ID
    int getId() const { return m_id; }

    // UserType 是 Qt 预留的起始值，+1 避免冲突
    enum { Type = UserType + 1 };
    int type() const override { return Type; }

protected:
    // 当节点发生改变时，这个函数会被自动调用
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
private:
    int m_id;
    QString m_name;
    QString m_nodeType;

    struct EdgeInfo {
        QGraphicsLineItem* line;
        bool isSource; // true=我是起点, false=我是终点
    };
    QList<EdgeInfo> m_edges;
};

#endif // VISUALNODE_H