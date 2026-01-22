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
    // ===== 问题3修复: 添加级联删除和事务处理 =====

    // 1. 备份数据，用于撤销时的"复活"
    GraphNode backup = NodeRepository::getNodeById(nodeId);
    if (!backup.isValid()) {  // 使用新的isValid()方法
        qWarning() << "GraphEditor: 无法删除无效节点 ID =" << nodeId;
        return false;
    }

    // 2. 获取所有关联的关系（必须在删除节点前备份）
    auto relatedEdges = RelationshipRepository::getEdgesByNode(nodeId);

    // 3. 删除所有关联的关系
    for (const auto& edge : relatedEdges) {
        if (!RelationshipRepository::deleteRelationship(edge.id)) {
            qCritical() << "GraphEditor: 删除关系失败，节点删除中止 (关系ID =" << edge.id << ")";
            return false;
        }
    }

    // 4. 删除节点本身
    if (!NodeRepository::deleteNode(nodeId)) {
        qCritical() << "GraphEditor: 删除节点失败 (节点ID =" << nodeId << ")";
        return false;
    }

    // 5. 创建复合删除命令（包含节点和所有关系的备份）
    // 注意：Command对象需要包含级联信息，便于undo时恢复
    auto cmd = std::make_shared<DeleteNodeCommand>(backup, relatedEdges);
    m_undoStack.push(cmd);
    clearRedoStack();

    emit nodeDeleted(nodeId);
    emit graphChanged();
    qInfo() << "GraphEditor: 节点删除成功，同时删除" << relatedEdges.size() << "条关系";
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