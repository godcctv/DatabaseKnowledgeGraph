#include "QueryEngine.h"
#include "../database/NodeRepository.h"
#include "../database/RelationshipRepository.h"
#include "../database/AttributeRepository.h"
#include <QQueue>
#include <QSet>
#include <QDebug>

// --- 基础检索实现 ---

QList<GraphNode> QueryEngine::getAllNodes(int ontologyId) {
    // 调用仓库层获取指定本体下的所有节点
    return NodeRepository::getAllNodes(ontologyId);
}

QList<GraphEdge> QueryEngine::getAllRelationships(int ontologyId) {
    // 调用仓库层获取指定本体下的所有关系
    return RelationshipRepository::getEdgesByOntology(ontologyId);
}

GraphNode QueryEngine::getNodeById(int nodeId) {
    // 调用仓库层根据 ID 获取节点对象
    return NodeRepository::getNodeById(nodeId);
}

// --- 高级检索实现 ---

QList<GraphNode> QueryEngine::queryByAttribute(const QString& attrName, const QString& attrValue) {
    QList<GraphNode> results;

    QList<Attribute> attrs = AttributeRepository::getAllAttributesByType("NODE");

    for (const auto& attr : attrs) {
        if (attr.attrName == attrName && attr.attrValue == attrValue) {
            if (attr.nodeId > 0) {
                GraphNode node = NodeRepository::getNodeById(attr.nodeId);
                if (node.isValid()) results.append(node);
            }
        }
    }
    return results;
}

// --- 图算法查询实现 ---

QList<int> QueryEngine::findPath(int sourceId, int targetId) {
    if (sourceId == targetId) return {sourceId};

    // 使用 BFS（广度优先搜索）寻找最短路径
    QQueue<int> queue;
    queue.enqueue(sourceId);

    QMap<int, int> parentMap; // 记录路径：当前节点 -> 父节点
    QSet<int> visited;
    visited.insert(sourceId);

    bool found = false;
    while (!queue.isEmpty()) {
        int current = queue.dequeue();
        if (current == targetId) {
            found = true;
            break;
        }

        // 获取当前节点的所有邻居关系
        QList<GraphEdge> edges = RelationshipRepository::getEdgesByNode(current);
        for (const auto& edge : edges) {
            int neighbor = (edge.sourceId == current) ? edge.targetId : edge.sourceId;
            if (!visited.contains(neighbor)) {
                visited.insert(neighbor);
                parentMap[neighbor] = current;
                queue.enqueue(neighbor);
            }
        }
    }

    // 回溯构造路径
    QList<int> path;
    if (found) {
        int curr = targetId;
        while (curr != sourceId) {
            path.prepend(curr);
            curr = parentMap[curr];
        }
        path.prepend(sourceId);
    }
    return path;
}

void QueryEngine::getSubgraph(int nodeId, int depth, QList<GraphNode>& outNodes, QList<GraphEdge>& outEdges) {
    if (depth < 0) return;

    QSet<int> visitedNodes;
    QSet<int> visitedEdges;
    
    // 使用 BFS 进行层级遍历
    QQueue<std::pair<int, int>> queue; // 节点 ID, 当前深度
    queue.enqueue({nodeId, 0});

    while (!queue.isEmpty()) {
        auto [currId, currDepth] = queue.dequeue();
        
        // 记录节点
        if (!visitedNodes.contains(currId)) {
            GraphNode node = NodeRepository::getNodeById(currId);
            if (node.isValid()) {
                outNodes.append(node);
                visitedNodes.insert(currId);
            }
        }

        // 如果未达到最大深度，继续向外扩展
        if (currDepth < depth) {
            QList<GraphEdge> edges = RelationshipRepository::getEdgesByNode(currId);
            for (const auto& edge : edges) {
                if (!visitedEdges.contains(edge.id)) {
                    outEdges.append(edge);
                    visitedEdges.insert(edge.id);
                    
                    int neighbor = (edge.sourceId == currId) ? edge.targetId : edge.sourceId;
                    queue.enqueue({neighbor, currDepth + 1});
                }
            }
        }
    }
}