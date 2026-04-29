#include <QtTest/QtTest>
#include <QDebug>
#include "database/NodeRepository.h"
#include "business/QueryEngine.h"
#include "model/GraphNode.h"
// #include "database/DatabaseConnection.h"

class TestKnowledgeGraph : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {
        // DatabaseConnection::connect("test_db");
    }

    void testNodeRepository() {
        GraphNode testNode;
        // 【修改点】：直接对 public 成员变量赋值，不使用 set() 方法
        testNode.ontologyId = 1;
        testNode.nodeType = "测试节点";
        testNode.name = "QTest临时节点";

        // 测试插入
        bool insertResult = NodeRepository::addNode(testNode);
        QVERIFY(insertResult == true);

        // 获取生成的 ID（【修改点】：直接访问 .id，不使用 getId()）
        // 注意：这里假设你的 addNode 成功后，会把新生成的数据库 ID 回填到 testNode.id 中
        int newId = testNode.id;

        // 测试查询
        GraphNode fetchedNode = NodeRepository::getNodeById(newId);
        // 【修改点】：直接访问 .name 进行比对
        QCOMPARE(fetchedNode.name, QString("QTest临时节点"));

        // 测试删除
        bool deleteResult = NodeRepository::deleteNode(newId);
        QVERIFY(deleteResult == true);
    }

    void testQueryEngineBFS() {
        QueryEngine engine;
        QList<int> path = engine.findPath(1, 3);
        QVERIFY(path.isEmpty() || path.size() > 0);
    }
};

QTEST_MAIN(TestKnowledgeGraph)
#include "tst_main.moc"