#include "GraphEditor.h"
#include "../database/NodeRepository.h"
#include "../database/RelationshipRepository.h"
#include <QDebug>

GraphEditor::GraphEditor(QObject *parent) : QObject(parent) {}

GraphEditor::~GraphEditor() = default;

// --- 节点业务 ---

bool GraphEditor::addNode(GraphNode& node) {
    if (node.ontologyId <= 0 || node.name.isEmpty() || node.nodeType.isEmpty()) {
        qWarning() << "GraphEditor: 无效的节点参数，无法添加";
        return false;
    }

    if (NodeRepository::addNode(node)) {
        emit nodeAdded(node);
        emit graphChanged();
        qInfo() << "GraphEditor: 节点添加成功，ID =" << node.id;
        return true;
    }
    return false;
}

bool GraphEditor::deleteNode(int nodeId) {
    if (NodeRepository::deleteNode(nodeId)) {
        emit nodeDeleted(nodeId);
        emit graphChanged();
        return true;
    }
    return false;
}

bool GraphEditor::updateNode(const GraphNode& oldNode, const GraphNode& newNode) {
    if (oldNode.id <= 0 || newNode.id <= 0 || oldNode.id != newNode.id) {
        qWarning() << "GraphEditor: 节点ID不匹配或无效，无法更新";
        return false;
    }
    if (newNode.name.isEmpty() || newNode.nodeType.isEmpty()) {
        qWarning() << "GraphEditor: 节点名称或类型为空，无法更新";
        return false;
    }

    if (NodeRepository::updateNode(newNode)) {
        emit nodeUpdated(newNode);
        emit graphChanged();
        qInfo() << "GraphEditor: 节点更新成功，ID =" << newNode.id;
        return true;
    }
    return false;
}

// --- 关系业务 ---

bool GraphEditor::addRelationship(GraphEdge& edge) {
    if (edge.ontologyId <= 0 || edge.sourceId <= 0 || edge.targetId <= 0) {
        qWarning() << "GraphEditor: 无效的关系参数，无法添加";
        return false;
    }
    if (RelationshipRepository::relationshipExists(edge.sourceId, edge.targetId, edge.relationType)) {
        qWarning() << "GraphEditor: 关系已存在，拒绝添加 ->" << edge.relationType;
        return false;
    }
    if (edge.sourceId == edge.targetId) {
        qWarning() << "GraphEditor: 源节点和目标节点不能相同，无法添加关系";
        return false;
    }
    if (edge.relationType.isEmpty()) {
        qWarning() << "GraphEditor: 关系类型为空，无法添加";
        return false;
    }

    if (RelationshipRepository::addRelationship(edge)) {
        emit relationshipAdded(edge);
        emit graphChanged();
        qInfo() << "GraphEditor: 关系添加成功，ID =" << edge.id;
        return true;
    }
    return false;
}

bool GraphEditor::deleteRelationship(int edgeId) {
    if (edgeId <= 0) {
        qWarning() << "GraphEditor: 无效的关系ID，无法删除";
        return false;
    }

    if (RelationshipRepository::deleteRelationship(edgeId)) {
        emit relationshipDeleted(edgeId);
        emit graphChanged();
        qInfo() << "GraphEditor: 关系删除成功，ID =" << edgeId;
        return true;
    }
    return false;
}

bool GraphEditor::updateRelationship(const GraphEdge& oldEdge, const GraphEdge& newEdge) {
    if (oldEdge.id <= 0 || newEdge.id <= 0) return false;

    if (RelationshipRepository::updateRelationship(newEdge)) {
        emit relationshipUpdated(newEdge);
        emit graphChanged();
        qInfo() << "GraphEditor: 关系更新成功，ID =" << newEdge.id;
        return true;
    }
    return false;
}