#include "GraphEditor.h"
#include <QDebug>

GraphEditor::GraphEditor(QObject *parent) : QObject(parent) {}

GraphEditor::~GraphEditor() {
    clearRedoStack();
}

// --- 节点业务 ---

bool GraphEditor::addNode(GraphNode& node) {
    // ===== 问题5修复: 参数验证 =====
    if (node.ontologyId <= 0 || node.name.isEmpty() || node.nodeType.isEmpty()) {
        qWarning() << "GraphEditor: 无效的节点参数，无法添加";
        return false;
    }

    if (NodeRepository::addNode(node)) {
        auto cmd = std::make_shared<AddNodeCommand>(node);
        m_undoStack.push(cmd);
        clearRedoStack(); // 产生新分支，清空重做栈

        emit nodeAdded(node);
        emit graphChanged();
        qInfo() << "GraphEditor: 节点添加成功，ID =" << node.id;
        return true;
    }
    return false;
}

bool GraphEditor::deleteNode(int nodeId) {
    // 1. 备份数据用于撤销
    GraphNode backup = NodeRepository::getNodeById(nodeId);
    if (!backup.isValid()) return false;

    // 2. 备份关联关系（仅用于撤销栈存储）
    auto relatedEdges = RelationshipRepository::getEdgesByNode(nodeId);

    // 3. 执行删除节点操作
    if (!NodeRepository::deleteNode(nodeId)) {
        return false;
    }

    // 4. 推送至撤销栈
    auto cmd = std::make_shared<DeleteNodeCommand>(backup, relatedEdges);
    m_undoStack.push(cmd);
    clearRedoStack();

    emit nodeDeleted(nodeId);
    emit graphChanged();
    return true;
}

bool GraphEditor::updateNode(const GraphNode& oldNode, const GraphNode& newNode) {
    // ===== 问题5修复: 参数验证 =====
    if (oldNode.id <= 0 || newNode.id <= 0) {
        qWarning() << "GraphEditor: 无效的节点ID，无法更新";
        return false;
    }
    if (oldNode.id != newNode.id) {
        qWarning() << "GraphEditor: 节点ID不匹配，无法更新";
        return false;
    }
    if (newNode.name.isEmpty() || newNode.nodeType.isEmpty()) {
        qWarning() << "GraphEditor: 节点名称或类型为空，无法更新";
        return false;
    }

    if (NodeRepository::updateNode(newNode)) {
        auto cmd = std::make_shared<UpdateNodeCommand>(oldNode, newNode);
        m_undoStack.push(cmd);
        clearRedoStack();

        emit graphChanged();
        qInfo() << "GraphEditor: 节点更新成功，ID =" << newNode.id;
        return true;
    }
    return false;
}

// --- 关系业务 ---

bool GraphEditor::addRelationship(GraphEdge& edge) {
    // ===== 问题5修复: 参数验证 =====
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
        auto cmd = std::make_shared<AddEdgeCommand>(edge);
        m_undoStack.push(cmd);
        clearRedoStack();

        emit relationshipAdded(edge);
        emit graphChanged();
        qInfo() << "GraphEditor: 关系添加成功，ID =" << edge.id;
        return true;
    }
    return false;
}

bool GraphEditor::deleteRelationship(int edgeId) {
    // ===== 完整性修复: 实现缺失的deleteRelationship方法 =====
    if (edgeId <= 0) {
        qWarning() << "GraphEditor: 无效的关系ID，无法删除";
        return false;
    }

    // 备份关系数据用于撤销
    GraphEdge backup = RelationshipRepository::getRelationshipById(edgeId);
    if (backup.id <= 0) {
        qWarning() << "GraphEditor: 无法找到要删除的关系，ID =" << edgeId;
        return false;
    }

    if (RelationshipRepository::deleteRelationship(edgeId)) {
        auto cmd = std::make_shared<DeleteEdgeCommand>(backup);
        m_undoStack.push(cmd);
        clearRedoStack();

        emit relationshipDeleted(edgeId);
        emit graphChanged();
        qInfo() << "GraphEditor: 关系删除成功，ID =" << edgeId;
        return true;
    }
    return false;
}

// --- 撤销/重做核心 ---

void GraphEditor::undo() {
    if (m_undoStack.isEmpty()) return;

    auto cmd = m_undoStack.pop();
    cmd->undo(); // 执行反向逻辑
    m_redoStack.push(cmd);

    emit graphChanged();
    qDebug() << "Editor: Undo operation executed.";
}

void GraphEditor::redo() {
    if (m_redoStack.isEmpty()) return;

    auto cmd = m_redoStack.pop();
    cmd->execute(); // 重新执行正向逻辑
    m_undoStack.push(cmd);

    emit graphChanged();
    qDebug() << "Editor: Redo operation executed.";
}

void GraphEditor::clearRedoStack() {
    m_redoStack.clear();
}