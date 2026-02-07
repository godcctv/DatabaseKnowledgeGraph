//#include <QCoreApplication>
#include <QApplication>
#include <QDebug>
#include <QJsonObject>
#include <cassert>
#include <QFile>
#include <QTextStream>
#include <QStyleFactory>
#include "database/DatabaseConnection.h"
#include "database/NodeRepository.h"
#include "database/RelationshipRepository.h"
#include "database/OntologyRepository.h"
#include "database/AttributeRepository.h"
#include "business/GraphEditor.h"
#include "business/QueryEngine.h"
#include "model/GraphNode.h"
#include "model/GraphEdge.h"
#include "model/Attribute.h"
#include "ui/mainwindow.h"

void runAttributeLogicTest();

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("Fusion"));

    QFile file(":/style.qss");
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qDebug() << "Warning: style.qss not found at src/ui/style.qss";
    } else {
        QTextStream stream(&file);
        a.setStyleSheet(stream.readAll());
        file.close();
    }
    // 依然需要先配置并连接数据库
    DatabaseConfig config;
    config.hostname = "localhost";
    config.username = "admin";
    config.password = "123456";
    config.database = "DatabaseKnowledgeGraph";

    if (DatabaseConnection::connect(config)) {
        //runAttributeLogicTest();

        MainWindow w;
        w.show();
        return a.exec();
       // return 0;

    }

    return -1;
}


void runAttributeLogicTest() {
    qDebug() << "\n==================== 开始属性表逻辑测试 ====================";

    // 1. 准备环境：我们需要一个有效的 ontology_id 和 node_id
    int testOntologyId = 1; // 假设 ID 为 1 的本体存在（由 test.sql 插入）

    // --- 测试场景 A: 节点属性 ---
    qDebug() << "👉 [场景 A] 测试节点属性...";

    // A1. 创建一个测试节点
    GraphNode node;
    node.ontologyId = testOntologyId;
    node.name = "属性测试节点_" + QString::number(QDateTime::currentMSecsSinceEpoch()); // 随机名防止冲突
    node.nodeType = "Test";

    if (NodeRepository::addNode(node)) {
        qDebug() << "   Step 1: 节点创建成功, ID =" << node.id;

        // A2. 给节点添加属性
        Attribute nodeAttr;
        nodeAttr.nodeId = node.id;       // 关键：设置 nodeId
        nodeAttr.relationId = -1;        // 关键：relationId 置空
        nodeAttr.attrName = "重要度";
        nodeAttr.attrValue = "高";
        nodeAttr.attrType = "String";

        if (AttributeRepository::addAttribute(nodeAttr)) {
            qDebug() << "   Step 2: 节点属性添加成功, AttrID =" << nodeAttr.id;
        } else {
            qCritical() << "❌ 节点属性添加失败！";
        }

        // A3. 验证查询 (使用 "NODE" 开关)
        QList<Attribute> results = AttributeRepository::getAttributesForEntity("NODE", node.id);
        bool found = false;
        for (const auto& attr : results) {
            if (attr.attrName == "重要度" && attr.attrValue == "高") {
                found = true;
                qDebug() << "   Step 3: 查询验证成功！读出的 nodeId =" << attr.nodeId;
                break;
            }
        }
        if (!found) qCritical() << "❌ 查询失败：未找到刚插入的节点属性";
    }

    // --- 测试场景 B: 关系属性 ---
    qDebug() << "\n👉 [场景 B] 测试关系属性...";

    // B1. 创建两个节点用于建立关系
    GraphNode n1, n2;
    n1.ontologyId = testOntologyId; n1.name = "N1_" + QString::number(qrand()); n1.nodeType="T";
    n2.ontologyId = testOntologyId; n2.name = "N2_" + QString::number(qrand()); n2.nodeType="T";
    NodeRepository::addNode(n1);
    NodeRepository::addNode(n2);

    // B2. 创建关系
    GraphEdge edge;
    edge.ontologyId = testOntologyId;
    edge.sourceId = n1.id;
    edge.targetId = n2.id;
    edge.relationType = "TestLink";

    if (RelationshipRepository::addRelationship(edge)) {
        qDebug() << "   Step 1: 关系创建成功, ID =" << edge.id;

        // B3. 给关系添加属性
        Attribute relAttr;
        relAttr.nodeId = -1;             // 关键：nodeId 置空
        relAttr.relationId = edge.id;    // 关键：设置 relationId
        relAttr.attrName = "连接强度";
        relAttr.attrValue = "0.95";
        relAttr.attrType = "Float";

        if (AttributeRepository::addAttribute(relAttr)) {
            qDebug() << "   Step 2: 关系属性添加成功, AttrID =" << relAttr.id;
        }

        // B4. 验证查询 (使用 "RELATION" 开关)
        QList<Attribute> results = AttributeRepository::getAttributesForEntity("RELATION", edge.id);
        bool found = false;
        for (const auto& attr : results) {
            // 验证 attrName 和 attrValue，同时也验证 relationId 是否正确回填
            if (attr.attrName == "连接强度" && attr.relationId == edge.id) {
                found = true;
                qDebug() << "   Step 3: 查询验证成功！读出的 relationId =" << attr.relationId;
                break;
            }
        }
        if (!found) qCritical() << "❌ 查询失败：未找到刚插入的关系属性";

        // B5. 清理测试数据 (级联删除测试)
        NodeRepository::deleteNode(node.id);
        NodeRepository::deleteNode(n1.id); // 删除 N1 会自动删除 edge，以及 edge 的属性
        NodeRepository::deleteNode(n2.id);
        qDebug() << "   Step 4: 测试数据已清理";
    }

    qDebug() << "==================== 测试结束 ====================\n";
}