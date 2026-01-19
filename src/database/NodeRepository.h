#ifndef NODEREPOSITORY_H
#define NODEREPOSITORY_H

#include <QList>
#include <QString>
#include "../model/GraphNode.h"

/**
 * @brief 节点仓库类，负责 node 表的所有数据库操作
 */
class NodeRepository {
public:
    // --- 增 ---
    static bool addNode(GraphNode& node);

    // --- 删 ---
    static bool deleteNode(int nodeId);

    // --- 改 ---
    static bool updateNode(const GraphNode& node);

    // --- 查 ---
    static GraphNode getNodeById(int nodeId);
    static QList<GraphNode> getAllNodes(int ontologyId);
    static QList<GraphNode> getNodesByType(int ontologyId, const QString& type);

private:
    // 内部辅助函数：执行具体的 SQL 绑定逻辑
    static bool executeInsert(const GraphNode& node, int& outId);
};

#endif // NODEREPOSITORY_H