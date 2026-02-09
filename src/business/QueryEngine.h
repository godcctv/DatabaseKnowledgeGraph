#ifndef QUERYENGINE_H
#define QUERYENGINE_H

#include <QObject>
#include <QList>
#include "../model/GraphNode.h"
#include "../model/GraphEdge.h"

class QueryEngine : public QObject
{
    Q_OBJECT
public:
    explicit QueryEngine(QObject *parent = nullptr);

    // --- 1. 全图查询 ---
    QList<GraphNode> getAllNodes(int ontologyId);
    QList<GraphEdge> getAllRelationships(int ontologyId);

    // --- 2. 单节点查询辅助 ---
    GraphNode getNodeById(int nodeId);
    // 获取与指定节点相连的所有边
    QList<GraphEdge> getRelatedRelationships(int nodeId);

    // --- 3. 属性查询 ---
    QList<GraphNode> queryByAttribute(const QString& attrName, const QString& attrValue);

    // --- 4. 路径查询 ---
    QList<int> findPath(int sourceId, int targetId);

private:
    // 辅助：构建邻接表
    QMap<int, QList<int>> buildAdjacencyList();
};

#endif // QUERYENGINE_H