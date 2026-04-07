#ifndef GRAPHEDITOR_H
#define GRAPHEDITOR_H

#include <QObject>
#include "../model/GraphNode.h"
#include "../model/GraphEdge.h"

/**
 * @brief 图操作业务逻辑类，负责封装数据库操作
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
    bool updateRelationship(const GraphEdge& oldEdge, const GraphEdge& newEdge);

    signals:
        // 信号：当业务层完成操作时，通知 UI 层进行同步
        void nodeAdded(const GraphNode& node);
    void nodeDeleted(int nodeId);
    void relationshipAdded(const GraphEdge& edge);
    void relationshipDeleted(int edgeId);
    void nodeUpdated(const GraphNode& node);
    void relationshipUpdated(const GraphEdge& edge);

    // 通用信号：表示图数据发生了任何变动
    void graphChanged();
};

#endif // GRAPHEDITOR_H