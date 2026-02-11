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
    GraphNode node = getNodeById(nodeId);
    QList<GraphEdge> allEdges = RelationshipRepository::getAllRelationships(node.ontologyId);

    QList<GraphEdge> result;
    for (const auto& edge : allEdges) {
        if (edge.sourceId == nodeId || edge.targetId == nodeId) {
            result.append(edge);
        }
    }
    return result;
}

QList<GraphNode> QueryEngine::queryByAttribute(const QString& attrName, const QString& attrValue) {

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

    GraphNode node = getNodeById(sourceId);
    QList<GraphEdge> edges = getAllRelationships(node.ontologyId);
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