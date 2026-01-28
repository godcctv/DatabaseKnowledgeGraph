#ifndef EDITORCOMMAND_H
#define EDITORCOMMAND_H

#include "../model/GraphNode.h"
#include "../model/GraphEdge.h"
#include "../database/NodeRepository.h"
#include "../database/RelationshipRepository.h"

// 命令基类
class EditorCommand {
public:
    virtual ~EditorCommand() = default;
    virtual void execute() = 0;   // 正向执行
    virtual void undo() = 0;      // 撤销回退
};

// --- 节点操作命令 ---

class AddNodeCommand : public EditorCommand {
    GraphNode m_node;
public:
    AddNodeCommand(const GraphNode& node) : m_node(node) {}
    void execute() override { NodeRepository::addNode(m_node); }
    void undo() override { NodeRepository::deleteNode(m_node.id); }
};

class DeleteNodeCommand : public EditorCommand {
    GraphNode m_backup;
    QList<GraphEdge> m_relatedEdges;  // ===== 修复问题3: 保存级联删除的关系 =====

public:
    // 旧构造函数（兼容性）
    DeleteNodeCommand(const GraphNode& node) : m_backup(node) {}

    // 新构造函数（包含级联信息）
    DeleteNodeCommand(const GraphNode& node, const QList<GraphEdge>& edges)
        : m_backup(node), m_relatedEdges(edges) {}

    void execute() override {
        // 删除所有关联的关系
        for (const auto& edge : m_relatedEdges) {
            RelationshipRepository::deleteRelationship(edge.id);
        }
        // 删除节点本身
        NodeRepository::deleteNode(m_backup.id);
    }

    // 找到 DeleteNodeCommand 类中的 undo 函数并修改如下：
    void undo() override {
        // 1. 记录被删除时的旧 ID
        int oldNodeId = m_backup.id;

        // 2. 恢复节点。执行后，m_backup.id 会被更新为数据库生成的新 ID
        if (!NodeRepository::addNode(m_backup)) {
            return;
        }
        int newNodeId = m_backup.id;

        //  3. 恢复所有关联的关系
        for (const auto& edge : m_relatedEdges) {
            GraphEdge temp = edge;
            // 关键修复：如果关系中的起点或终点是刚才恢复的节点，则更新其 ID 为新 ID
            if (temp.sourceId == oldNodeId) temp.sourceId = newNodeId;
            if (temp.targetId == oldNodeId) temp.targetId = newNodeId;

            RelationshipRepository::addRelationship(temp);
        }
    }
};

class UpdateNodeCommand : public EditorCommand {
    GraphNode m_old;
    GraphNode m_new;
public:
    UpdateNodeCommand(const GraphNode& o, const GraphNode& n) : m_old(o), m_new(n) {}
    void execute() override { NodeRepository::updateNode(m_new); }
    void undo() override { NodeRepository::updateNode(m_old); }
};

// --- 关系操作命令 ---

class AddEdgeCommand : public EditorCommand {
    GraphEdge m_edge;
public:
    AddEdgeCommand(const GraphEdge& edge) : m_edge(edge) {}
    void execute() override { RelationshipRepository::addRelationship(m_edge); }
    void undo() override { RelationshipRepository::deleteRelationship(m_edge.id); }
};

// ===== 修复完整性: 添加缺失的DeleteEdgeCommand =====
class DeleteEdgeCommand : public EditorCommand {
    GraphEdge m_backup;
public:
    DeleteEdgeCommand(const GraphEdge& edge) : m_backup(edge) {}
    void execute() override { RelationshipRepository::deleteRelationship(m_backup.id); }
    void undo() override {
        GraphEdge temp = m_backup;
        RelationshipRepository::addRelationship(temp);
    }
};

#endif