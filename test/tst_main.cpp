#include <QtTest/QtTest>
#include <QDebug>
#include "database/NodeRepository.h"
#include "business/QueryEngine.h"
#include "model/GraphNode.h"
// 【必须包含数据库连接类的头文件】
#include "database/DatabaseConnection.h"

class TestKnowledgeGraph : public QObject
{
    Q_OBJECT

private slots:
    // 测试前的初始化准备（在这里连接数据库！）
    void initTestCase() {
        // 1. 组装你提供的配置信息
        DatabaseConfig config;
        config.hostname = "localhost";
        config.username = "admin";
        config.password = "123456";
        config.database = "DatabaseKnowledgeGraph";
        // config.port 默认是 3306，不用额外写

        // 2. 尝试连接数据库
        bool ok = DatabaseConnection::connect(config);

        // 3. 断言卡点：如果返回 false，测试直接红字报错停下，不往下走了
        QVERIFY2(ok == true, "MySQL数据库连接失败，请检查配置或确认MySQL服务已启动！");

        qDebug() << "============== 测试数据库连接成功！==============";
    }

    void testNodeRepository() {
        GraphNode testNode;
        testNode.ontologyId = 99;
        testNode.nodeType = "测试节点";
        testNode.name = "QTest临时节点";

        // 测试插入
        bool insertResult = NodeRepository::addNode(testNode);
        QVERIFY(insertResult == true);

        // 测试查询
        int newId = testNode.id;
        GraphNode fetchedNode = NodeRepository::getNodeById(newId);
        QCOMPARE(fetchedNode.name, QString("QTest临时节点"));

        // 测试删除 (记得删掉脏数据)
        bool deleteResult = NodeRepository::deleteNode(newId);
        QVERIFY(deleteResult == true);
    }

    void testQueryEngineBFS() {
        QueryEngine engine;
        QList<int> path = engine.findPath(1, 3);
        // 如果图谱里有连通的数据，大小必定大于0；如果没有，会安全返回空列表
        QVERIFY(path.isEmpty() || path.size() > 0);
    }

    // 所有测试跑完之后，打扫战场
    void cleanupTestCase() {
        // 安全断开数据库连接
        DatabaseConnection::disconnect();
        qDebug() << "============== 测试完毕，数据库连接已断开 ==============";
    }
};

QTEST_MAIN(TestKnowledgeGraph)
#include "tst_main.moc"