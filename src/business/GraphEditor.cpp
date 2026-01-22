#include "GraphEditor.h"
#include <QDebug>

GraphEditor::GraphEditor(QObject *parent) : QObject(parent) {}

GraphEditor::~GraphEditor() {
    clearRedoStack();
}

// --- 节点业务 ---

bool GraphEditor::addNode(GraphNode& node) {
    if (NodeRepository::addNode(node)) {
        auto cmd = std::make_shared<AddNodeCommand>(node);
        m_undoStack.push(cmd);
        clearRedoStack(); // 产生新分支，清空重做栈

        emit nodeAdded(node);
        emit graphChanged();
        return true;
    }
    return false;
}

bool GraphEditor::deleteNode(int nodeId) {
    // 1. 备份数据，用于撤销时的“复活”
    GraphNode backup = NodeRepository::getNodeById(nodeId);
    if (backup.id == -1) return false;

    if (NodeRepository::deleteNode(nodeId)) {
        auto cmd = std::make_shared<DeleteNodeCommand>(backup);
        m_undoStack.push(cmd);
        clearRedoStack();

        emit nodeDeleted(nodeId);
        emit graphChanged();
        return true;
    }
    return false;
}

bool GraphEditor::updateNode(const GraphNode& oldNode, const GraphNode& newNode) {
    if (NodeRepository::updateNode(newNode)) {
        auto cmd = std::make_shared<UpdateNodeCommand>(oldNode, newNode);
        m_undoStack.push(cmd);
        clearRedoStack();

        emit graphChanged();
        return true;
    }
    return false;
}

// --- 关系业务 ---

bool GraphEditor::addRelationship(GraphEdge& edge) {
    if (RelationshipRepository::addRelationship(edge)) {
        auto cmd = std::make_shared<AddEdgeCommand>(edge);
        m_undoStack.push(cmd);
        clearRedoStack();

        emit relationshipAdded(edge);
        emit graphChanged();
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