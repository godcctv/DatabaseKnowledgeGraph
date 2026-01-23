#ifndef GRAPHEDITOR_H
#define GRAPHEDITOR_H

#include <QObject>
#include <QStack>
#include <memory>
#include "EditorCommand.h"
#include "../model/GraphNode.h"
#include "../model/GraphEdge.h"

/**
 * @brief 图操作业务逻辑类，负责封装数据库操作并提供撤销/重做功能
 */
class GraphEditor : public QObject {
    Q_OBJECT
public:
    explicit GraphEditor(QObject *parent = nullptr);
    ~GraphEditor();

    // --- 节点操作业务接口 ---
    bool addNode(GraphNode& node);
    bool deleteNode(int nodeId);
    bool updateNode(const GraphNode& oldNode, const GraphNode& newNode);

    // --- 关系操作业务接口 ---
    bool addRelationship(GraphEdge& edge);
    bool deleteRelationship(int edgeId);

    // --- 撤销/重做核心接口 ---
    void undo();
    void redo();

    // 检查栈状态（用于 UI 菜单按钮的启用/禁用）
    bool canUndo() const { return !m_undoStack.isEmpty(); }
    bool canRedo() const { return !m_redoStack.isEmpty(); }

    signals:
        // 信号：当业务层完成操作时，通知 UI 层（可视化视图和属性面板）进行同步
    void nodeAdded(const GraphNode& node);
    void nodeDeleted(int nodeId);
    void relationshipAdded(const GraphEdge& edge);
    void relationshipDeleted(int edgeId);

    // 通用信号：表示图数据发生了任何变动
    void graphChanged();

private:
    // 命令历史栈，存储智能指针以管理 EditorCommand 生命周期
    QStack<std::shared_ptr<EditorCommand>> m_undoStack;
    QStack<std::shared_ptr<EditorCommand>> m_redoStack;

    // 辅助函数：当用户发起新操作时，必须清空重做栈
    void clearRedoStack();
};

#endif // GRAPHEDITOR_H