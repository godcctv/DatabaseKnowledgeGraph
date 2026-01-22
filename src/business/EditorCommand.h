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
public:
    DeleteNodeCommand(const GraphNode& node) : m_backup(node) {}
    void execute() override { NodeRepository::deleteNode(m_backup.id); }
    void undo() override { NodeRepository::addNode(m_backup); }
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

#endif