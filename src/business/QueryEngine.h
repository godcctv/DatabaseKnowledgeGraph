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
    QList<GraphNode> queryByAttribute(int ontologyId, const QString& attrName, const QString& attrValue);

    // --- 4. 路径查询 ---
    QList<QList<int>> findAllPaths(int sourceId, int targetId, int maxDepth = 10);

private:
    // 辅助：构建邻接表
    QMap<int, QList<int>> buildAdjacencyList();

    void dfsFindPaths(int current, int targetId,
                      QMap<int, QList<int>>& adj,
                      QList<int>& currentPath,
                      QSet<int>& visited,
                      QList<QList<int>>& allPaths,
                      int maxDepth);
};

#endif // QUERYENGINE_H