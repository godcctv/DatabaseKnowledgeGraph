#ifndef QUERYENGINE_H
#define QUERYENGINE_H

#include <QList>
#include <QString>
#include <QSet>
#include "../model/GraphNode.h"
#include "../model/GraphEdge.h"

/**
 * @brief 查询引擎类，负责图数据的检索与分析逻辑
 */
class QueryEngine {
public:
    // --- 基础检索 ---
    // 获取指定本体（项目）下的所有节点
    QList<GraphNode> getAllNodes(int ontologyId);
    // 获取指定本体（项目）下的所有关系
    QList<GraphEdge> getAllRelationships(int ontologyId);
    // 根据 ID 获取单个节点详情
    GraphNode getNodeById(int nodeId);

    // --- 高级检索 ---
    // 属性查询：根据自定义属性名和属性值筛选节点
    QList<GraphNode> queryByAttribute(const QString& attrName, const QString& attrValue);

    // --- 图算法查询 ---
    // 路径查询：寻找两个节点之间的连接路径（返回节点 ID 列表）
    QList<int> findPath(int sourceId, int targetId);
    // 子图查询：以某个节点为中心，获取指定深度内的关联子图
    void getSubgraph(int nodeId, int depth, QList<GraphNode>& outNodes, QList<GraphEdge>& outEdges);
};

#endif