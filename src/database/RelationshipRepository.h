#ifndef RELATIONSHIPREPOSITORY_H
#define RELATIONSHIPREPOSITORY_H

#include <QList>
#include "../model/GraphEdge.h"

class RelationshipRepository {
public:
    // --- 增 ---
    static bool addRelationship(GraphEdge& edge);

    // --- 删 ---
    static bool deleteRelationship(int relationId);

    // --- 改 ---
    static bool updateRelationship(const GraphEdge& edge);

    // --- 查 ---
    // 获取整个本体（项目）下的所有边，用于全图渲染
    static QList<GraphEdge> getEdgesByOntology(int ontologyId);
    
    // 获取与某个节点相关的所有边（起点或终点），用于局部查询
    static QList<GraphEdge> getEdgesByNode(int nodeId);

    // 根据ID获取单条关系
    static GraphEdge getRelationshipById(int relationId);
};

#endif