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

QList<QList<int>> QueryEngine::findAllPaths(int sourceId, int targetId, int maxDepth) {
    QList<QList<int>> allPaths;
    if (sourceId == targetId) return allPaths;

    // 获取当前本体的所有关系来构建邻接表
    GraphNode node = getNodeById(sourceId);
    QList<GraphEdge> edges = getAllRelationships(node.ontologyId);
    QMap<int, QList<int>> adj;

    for (const auto& edge : edges) {
        // 构建无向图视角的邻接表
        adj[edge.sourceId].append(edge.targetId);
        adj[edge.targetId].append(edge.sourceId);
    }

    QList<int> currentPath;
    QSet<int> visited;

    // 调用深度优先搜索寻找所有路径
    dfsFindPaths(sourceId, targetId, adj, currentPath, visited, allPaths, maxDepth);

    return allPaths;
}

// DFS 回溯实现
void QueryEngine::dfsFindPaths(int current, int targetId,
                               QMap<int, QList<int>>& adj,
                               QList<int>& currentPath,
                               QSet<int>& visited,
                               QList<QList<int>>& allPaths,
                               int maxDepth) {
    // 将当前节点加入路径并标记为已访问
    currentPath.append(current);
    visited.insert(current);

    // 如果到达目标节点，保存这条路径
    if (current == targetId) {
        allPaths.append(currentPath);
    }
    // 如果还没到达目标且未超过最大深度，继续向下搜索
    // 注意：currentPath.size() - 1 代表当前的跳数（边数）
    else if (currentPath.size() - 1 < maxDepth) {
        if (adj.contains(current)) {
            for (int neighbor : adj[current]) {
                if (!visited.contains(neighbor)) {
                    dfsFindPaths(neighbor, targetId, adj, currentPath, visited, allPaths, maxDepth);
                }
            }
        }
    }

    // 回溯：撤销当前节点的访问状态，以便从其他分支再次访问
    currentPath.removeLast();
    visited.remove(current);
}