#include <QCoreApplication>
#include <QDebug>
#include <QJsonObject>
#include <cassert>
#include "database/DatabaseConnection.h"
#include "database/NodeRepository.h"
#include "database/RelationshipRepository.h"
#include "database/OntologyRepository.h"
#include "business/GraphEditor.h"
#include "business/QueryEngine.h"


void runBusinessLogicTest() {
    qDebug() << "\n==================== 开始业务逻辑深度测试 ====================";
    GraphEditor editor;
    QueryEngine queryEngine;

    // 1. 环境准备
    QList<Ontology> onts = OntologyRepository::getAllOntologies();
    if (onts.isEmpty()) return;
    int testOntId = onts.first().id;

    // 🚩 关键修复：清理旧数据，防止 Duplicate Entry
    QList<GraphNode> existingNodes = NodeRepository::getAllNodes(testOntId);
    for (const auto& n : existingNodes) {
        if (n.name == "节点1" || n.name == "节点2" || n.name == "节点3") {
            NodeRepository::deleteNode(n.id);
        }
    }

    // --- 场景 6: 测试撤销/重做 ---
    GraphNode node1;
    node1.ontologyId = testOntId;
    node1.name = "节点1";
    node1.nodeType = "Logic";

    if (editor.addNode(node1)) {
        editor.undo();
        // 验证撤销...
        editor.redo();

        // 🚩 关键修复：Redo 后 ID 会变，必须同步
        QList<GraphNode> currentNodes = NodeRepository::getAllNodes(testOntId);
        for (const auto& n : currentNodes) {
            if (n.name == "节点1") { node1 = n; break; }
        }
    }

    // --- 场景 7: 测试路径查询 ---
    GraphNode node2, node3;
    node2.ontologyId = testOntId; node2.name = "节点2"; node2.nodeType = "Logic";
    node3.ontologyId = testOntId; node3.name = "节点3"; node3.nodeType = "Logic";

    // 确保节点添加成功后再进行后续操作
    if (editor.addNode(node2) && editor.addNode(node3)) {
        GraphEdge edge1, edge2;
        edge1.ontologyId = testOntId; edge1.sourceId = node1.id; edge1.targetId = node2.id;
        edge1.relationType = "Connect";

        edge2.ontologyId = testOntId; edge2.sourceId = node2.id; edge2.targetId = node3.id;
        edge2.relationType = "Connect";

        if (editor.addRelationship(edge1) && editor.addRelationship(edge2)) {
            QList<int> path = queryEngine.findPath(node1.id, node3.id);
            // 验证路径...
        }
    }
}
/**
 * @brief 运行逻辑健壮性测试
 * 专门验证报告中提到的：空指针处理、级联删除、参数验证及 JSON 安全性
 */
void runRobustnessTest() {
    qDebug() << "\n" << "==================== 开始逻辑健壮性测试 ====================";
    GraphEditor editor;

    // 1. 准备测试环境：获取一个有效的本体 ID
    QList<Ontology> onts = OntologyRepository::getAllOntologies();
    if (onts.isEmpty()) {
        qCritical() << "❌ 测试中止: 数据库中没有本体，请先创建。";
        return;
    }
    int testOntId = onts.first().id;

    // --- 场景 1: 验证字段完整性与有效性判断 (修复问题 1 & 1a) ---
    qDebug() << "👉 场景1: 测试 getNodeById() 的字段完整映射与 isValid()...";
    GraphNode nodeA;
    nodeA.ontologyId = testOntId;
    nodeA.name = "完整性测试节点";
    nodeA.nodeType = "Test";
    nodeA.description = "测试描述";
    nodeA.posX = 100.5f;
    nodeA.properties["status"] = "online";

    if (editor.addNode(nodeA)) {
        GraphNode retrieved = NodeRepository::getNodeById(nodeA.id);
        if (retrieved.isValid() && !retrieved.properties.isEmpty() && retrieved.posX > 100.0f) {
            qDebug() << "✅ 修复成功: 节点所有字段（含坐标和JSON）已完整映射。";
        } else {
            qCritical() << "❌ 修复失败: 获取到的对象不完整或 isValid() 判定错误。";
        }
    }

    // --- 场景 2: 验证级联删除逻辑 (修复问题 1b) ---
    qDebug() << "\n👉 场景2: 测试级联删除（删除节点及其关联关系）...";
    // 创建节点 B 并建立与 A 的关系
    GraphNode nodeB;
    nodeB.ontologyId = testOntId;
    nodeB.name = "关联节点B";
    nodeB.nodeType = "Test";
    editor.addNode(nodeB);

    GraphEdge edge;
    edge.ontologyId = testOntId;
    edge.sourceId = nodeA.id;
    edge.targetId = nodeB.id;
    edge.relationType = "Dependency";
    editor.addRelationship(edge);

    // 执行级联删除
    if (editor.deleteNode(nodeA.id)) {
        // 验证数据库中该关系是否已消失
        auto edges = RelationshipRepository::getEdgesByNode(nodeA.id);
        if (edges.isEmpty()) {
            qDebug() << "✅ 修复成功: 删除节点时，关联关系已同步清理。";
        }
    }

    // --- 场景 3: 验证输入参数校验 (修复问题 5) ---
    qDebug() << "\n👉 场景3: 测试无效输入拦截...";
    GraphNode invalidNode;
    invalidNode.name = ""; // 名字为空
    invalidNode.ontologyId = -1; // 无效 ID

    if (!editor.addNode(invalidNode)) {
        qDebug() << "✅ 修复成功: 业务层成功拦截了无效的节点数据。";
    }

    // --- 场景 4: 验证更新操作的返回值检查 (修复问题 1c) ---
    qDebug() << "\n👉 场景4: 测试 updateNode() 的 numRowsAffected 检查...";
    GraphNode phantomNode;
    phantomNode.id = 999999; // 不存在的 ID
    phantomNode.name = "幽灵节点";
    phantomNode.ontologyId = testOntId;

    if (!NodeRepository::updateNode(phantomNode)) {
        qDebug() << "✅ 修复成功: 更新不存在的节点返回 false，未产生误报。";
    }

    // --- 场景 5: 验证 JSON 编码安全性与中文处理 (修复问题 6) ---
    qDebug() << "\n👉 场景5: 测试中文 JSON 属性存储安全性...";
    GraphNode chineseNode;
    chineseNode.ontologyId = testOntId;
    chineseNode.name = "中文测试";
    chineseNode.nodeType = "Encoding";
    chineseNode.properties["备注"] = "这是一段带有特殊字符'\"的中文描述"; //

    if (editor.addNode(chineseNode)) {
        GraphNode retrieved = NodeRepository::getNodeById(chineseNode.id);
        if (retrieved.properties["备注"].toString() == chineseNode.properties["备注"].toString()) {
            qDebug() << "✅ 修复成功: 中文及特殊字符在 JSON 中序列化正常。";
        }
    }

    qDebug() << "\n==================== 逻辑健壮性测试全部完成 ====================\n";
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    DatabaseConfig config;
    config.hostname = "192.168.137.129";
    config.username = "root";
    config.password = "123456";
    config.database = "DatabaseKnowledgeGraph";
    config.port = 3306;

    if (DatabaseConnection::connect(config)) {
        runBusinessLogicTest();
    } else {
        qCritical() << "无法启动测试: 数据库连接失败。";
    }

    return 0; // 直接退出，不进入事件循环
}