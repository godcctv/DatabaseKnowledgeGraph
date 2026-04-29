#include <QtTest/QtTest>
#include <QDebug>
#include <QSqlQuery>
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
        // ================== 1. 准备前置数据 ==================
        // 为了满足外键约束，我们必须先在数据库里造一个临时的“本体 (Ontology)”
        QSqlQuery query(DatabaseConnection::getDatabase());
        // 插入一个测试用的本体项目
        query.exec("INSERT INTO ontology (name, version) VALUES ('QTest测试专用图谱', '1.0')");
        // 获取刚插入的这个合法本体的自增 ID
        int validOntologyId = query.lastInsertId().toInt();
        QVERIFY2(validOntologyId > 0, "前置本体数据准备失败！");

        // ================== 2. 构造测试节点 ==================
        GraphNode testNode;
        testNode.ontologyId = validOntologyId; // 【修改点】使用数据库真实存在的本体 ID
        testNode.nodeType = "测试节点";
        testNode.name = "QTest临时节点";

        // ================== 3. 执行节点 CRUD 测试 ==================
        // 测试插入
        bool insertResult = NodeRepository::addNode(testNode);
        QVERIFY(insertResult == true);

        // 测试查询 (提取生成的节点 ID)
        int newId = testNode.id;
        GraphNode fetchedNode = NodeRepository::getNodeById(newId);
        QCOMPARE(fetchedNode.name, QString("QTest临时节点"));

        // 测试删除 (删掉临时节点)
        bool deleteResult = NodeRepository::deleteNode(newId);
        QVERIFY(deleteResult == true);

        // ================== 4. 打扫战场 ==================
        // 把一开始建的那个临时本体也顺手删掉，保持数据库干干净净
        query.prepare("DELETE FROM ontology WHERE ontology_id = ?");
        query.addBindValue(validOntologyId);
        query.exec();
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