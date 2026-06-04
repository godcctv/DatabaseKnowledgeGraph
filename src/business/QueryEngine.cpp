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

QList<GraphNode> QueryEngine::queryByAttribute(int ontologyId, const QString& attrName, const QString& attrValue) {

    // 修复1: 使用传入的 ontologyId 替换硬编码的 1
    QList<GraphNode> allNodes = getAllNodes(ontologyId);
    QList<GraphNode> result;

    for (const auto& node : allNodes) {
        bool isMatch = false;

        // 修复2: 扩展基础字段匹配
        if (attrName == "name" && node.name.contains(attrValue, Qt::CaseInsensitive)) {
            isMatch = true;
        } else if (attrName == "type" && node.nodeType.contains(attrValue, Qt::CaseInsensitive)) {
            isMatch = true;
        } else if (attrName == "description" && node.description.contains(attrValue, Qt::CaseInsensitive)) {
            isMatch = true;
        }
        // 修复3: 扩展 JSON 属性的搜索
        else if (node.properties.contains(attrName)) {
            QJsonValue jsonVal = node.properties.value(attrName);
            // 将 JSON 值转换为字符串进行模糊匹配
            QString valStr;
            if (jsonVal.isString()) {
                valStr = jsonVal.toString();
            } else if (jsonVal.isDouble()) {
                valStr = QString::number(jsonVal.toDouble());
            } else if (jsonVal.isBool()) {
                valStr = jsonVal.toBool() ? "true" : "false";
            }

            if (valStr.contains(attrValue, Qt::CaseInsensitive)) {
                isMatch = true;
            }
        }

        // 如果符合条件则加入结果集
        if (isMatch) {
            result.append(node);
        }
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