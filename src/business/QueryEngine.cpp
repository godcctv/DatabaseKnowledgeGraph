#include "QueryEngine.h"
#include "../database/NodeRepository.h"
#include "../database/RelationshipRepository.h"
#include <QQueue>
#include <QSet>
#include <QMap>

QueryEngine::QueryEngine(QObject *parent) : QObject(parent) {}

QList<GraphNode> QueryEngine::getAllNodes(int ontologyId) {
    return NodeRepository::getAllNodes(ontologyId);
}

QList<GraphEdge> QueryEngine::getAllRelationships(int ontologyId) {
    return RelationshipRepository::getAllRelationships(ontologyId);
}

GraphNode QueryEngine::getNodeById(int nodeId) {
    return NodeRepository::getNodeById(nodeId);
}

QList<GraphEdge> QueryEngine::getRelatedRelationships(int nodeId) {
    // 获取本体1的所有关系，然后筛选 (实际项目中应由数据库直接筛选)
    QList<GraphEdge> allEdges = RelationshipRepository::getAllRelationships(1);
    QList<GraphEdge> result;
    for (const auto& edge : allEdges) {
        if (edge.sourceId == nodeId || edge.targetId == nodeId) {
            result.append(edge);
        }
    }
    return result;
}

QList<GraphNode> QueryEngine::queryByAttribute(const QString& attrName, const QString& attrValue) {
    // 文档设计中属性是单独的表，但为了简化，我们这里暂时搜索 Node 的 name 或 description
    // 或者解析 properties JSON 字段。
    // 这里实现一个简化的：如果 attrName 是 "name"，就搜名字

    QList<GraphNode> allNodes = getAllNodes(1);
    QList<GraphNode> result;

    for (const auto& node : allNodes) {
        if (attrName == "name") {
            if (node.name.contains(attrValue, Qt::CaseInsensitive)) {
                result.append(node);
            }
        } else if (attrName == "type") {
            if (node.nodeType.contains(attrValue, Qt::CaseInsensitive)) {
                result.append(node);
            }
        }
        // 可以在这里扩展 JSON 属性的搜索
    }
    return result;
}

QList<int> QueryEngine::findPath(int sourceId, int targetId) {
    QList<int> path;
    if (sourceId == targetId) return path;

    // 1. 获取所有边构建图
    QList<GraphEdge> edges = getAllRelationships(1);
    QMap<int, QList<int>> adj;
    for (const auto& edge : edges) {
        adj[edge.sourceId].append(edge.targetId);
        adj[edge.targetId].append(edge.sourceId); // 无向图视角的路径
    }

    // 2. BFS
    QQueue<int> queue;
    queue.enqueue(sourceId);
    QSet<int> visited;
    visited.insert(sourceId);
    QMap<int, int> predecessors; // 记录前驱节点

    bool found = false;
    while (!queue.isEmpty()) {
        int current = queue.dequeue();
        if (current == targetId) {
            found = true;
            break;
        }

        if (adj.contains(current)) {
            for (int neighbor : adj[current]) {
                if (!visited.contains(neighbor)) {
                    visited.insert(neighbor);
                    predecessors[neighbor] = current;
                    queue.enqueue(neighbor);
                }
            }
        }
    }

    // 3. 回溯路径
    if (found) {
        int curr = targetId;
        path.prepend(curr);
        while (curr != sourceId) {
            curr = predecessors[curr];
            path.prepend(curr);
        }
    }
    return path;
}