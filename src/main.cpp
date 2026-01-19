#include <QCoreApplication>
#include <QDebug>
#include <QJsonObject>
#include "database/DatabaseConnection.h"
#include "database/OntologyRepository.h"
#include "database/NodeRepository.h"
#include "database/RelationshipRepository.h"
#include "database/AttributeRepository.h"

void runHeavyDutyTest() {
    qDebug() << "==================== 开始增强版压力测试 ====================";

    // 1. 获取或创建一个干净的测试本体
    QString testName = "压力测试项目";
    Ontology testOnt;

    // 先检查是否存在同名项目，存在就先删掉（为了保证每次测试都是从零开始）
    QList<Ontology> all = OntologyRepository::getAllOntologies();
    for(const auto& o : all) {
        if(o.name == testName) {
            OntologyRepository::deleteOntology(o.id);
            qDebug() << "🧹 清理了旧的测试项目";
        }
    }

    testOnt.name = testName;
    testOnt.version = "1.0";
    testOnt.description = "用于压力测试";
    OntologyRepository::addOntology(testOnt);
    int testOntId = testOnt.id;

    // 2. 测试插入数据（确保所有 NOT NULL 字段都有值）
    qDebug() << "👉 测试2: 模拟大规模节点生成...";
    for (int i = 0; i < 10; ++i) {
        GraphNode n;
        n.ontologyId = testOntId;
        n.name = QString("节点_%1").arg(i);
        n.nodeType = "核心概念";  // 必填字段1
        n.description = "测试描述"; // 必填字段2
        n.posX = i * 50.0f;
        n.posY = i * 50.0f;

        if(!NodeRepository::addNode(n)) {
            qCritical() << "❌ 节点" << i << "插入失败，原因:" << "请检查是否有其他非空字段";
        }
    }

    // 3. 读取验证
    QList<GraphNode> nodes = NodeRepository::getAllNodes(testOntId);
    qDebug() << "✅ 批量读取成功，当前节点总数:" << nodes.size();

    // 4. 测试重复插入（验证唯一性约束是否生效）
    qDebug() << "👉 测试4: 验证唯一性约束（预期应报错）...";
    GraphNode dup;
    dup.ontologyId = testOntId;
    dup.name = "节点_0"; // 重复的名字
    dup.nodeType = "核心概念";
    if (!NodeRepository::addNode(dup)) {
        qDebug() << "✅ 数据库成功拦截了重复数据，约束逻辑正确！";
    }

    qDebug() << "==================== 测试结束 ====================";
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // 数据库连接配置
    DatabaseConfig config;
    config.hostname = "192.168.137.129"; // 确保此IP与虚拟机一致
    config.username = "root";
    config.password = "123456";
    config.database = "DatabaseKnowledgeGraph";
    config.port = 3306;

    if (DatabaseConnection::connect(config)) {
        runHeavyDutyTest();
    } else {
        qCritical() << "无法启动测试: 数据库连接失败。";
    }

    // 测试完毕后，我们直接退出程序，不进入事件循环
    return 0;
}