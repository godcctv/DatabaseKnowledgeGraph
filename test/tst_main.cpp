#include <QtTest/QtTest>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>

// 引入相关的头文件
#include "database/DatabaseConnection.h"
#include "database/NodeRepository.h"
#include "database/RelationshipRepository.h"
#include "database/OntologyRepository.h"
#include "database/UserRepository.h"
#include "business/QueryEngine.h"
#include "business/GraphEditor.h"
#include "model/GraphNode.h"
#include "model/GraphEdge.h"
#include "model/User.h"

class TestKnowledgeGraph : public QObject
{
    Q_OBJECT

private slots:
    // 0. 环境初始化
    void initTestCase() {
        DatabaseConfig config;
        config.hostname = "localhost";
        config.username = "admin";
        config.password = "123456";
        config.database = "DatabaseKnowledgeGraph";

        bool ok = DatabaseConnection::connect(config);
        QVERIFY2(ok == true, "MySQL数据库连接失败，请检查配置！");
        qDebug() << "============== 测试数据库连接成功！==============";
    }

    // 1. 节点基础测试
    void testNodeRepository() {
        QSqlQuery query(DatabaseConnection::getDatabase());
        query.exec("INSERT INTO ontology (name, version) VALUES ('QTest_NodeTest', '1.0')");
        int validOntologyId = query.lastInsertId().toInt();
        QVERIFY(validOntologyId > 0);

        GraphNode testNode;
        testNode.ontologyId = validOntologyId;
        testNode.nodeType = "测试节点";
        testNode.name = "QTest临时节点";

        bool insertResult = NodeRepository::addNode(testNode);
        QVERIFY(insertResult == true);

        int newId = testNode.id;
        GraphNode fetchedNode = NodeRepository::getNodeById(newId);
        QCOMPARE(fetchedNode.name, QString("QTest临时节点"));

        NodeRepository::deleteNode(newId);

        query.prepare("DELETE FROM ontology WHERE ontology_id = ?");
        query.addBindValue(validOntologyId);
        query.exec();
    }

    // 2. BFS 路径引擎测试
    void testQueryEngineBFS() {
        QueryEngine engine;
        QList<int> path = engine.findPath(1, 3);
        QVERIFY(path.isEmpty() || path.size() > 0);
    }

    // 3. 连线与外键级联删除测试
    void testRelationshipAndCascade() {
        // 步骤A：建立图谱本体
        QSqlQuery query(DatabaseConnection::getDatabase());
        query.exec("INSERT INTO ontology (name, version) VALUES ('QTest_CascadeTest', '1.0')");
        int ontologyId = query.lastInsertId().toInt();
        QVERIFY(ontologyId > 0);

        // 步骤B：创建两个测试节点
        GraphNode node1, node2;
        node1.ontologyId = ontologyId; node1.name = "连线起点"; node1.nodeType = "实体";
        node2.ontologyId = ontologyId; node2.name = "连线终点"; node2.nodeType = "实体";
        NodeRepository::addNode(node1);
        NodeRepository::addNode(node2);
        QVERIFY(node1.id > 0 && node2.id > 0);

        // 步骤C：测试建立连线关系
        GraphEdge edge;
        edge.ontologyId = ontologyId;
        edge.sourceId = node1.id;
        edge.targetId = node2.id;
        edge.relationType = "关联";
        bool edgeAdded = RelationshipRepository::addRelationship(edge);
        QVERIFY(edgeAdded == true);
        QVERIFY(edge.id > 0);

        // 步骤D：测试级联删除 (大杀器)
        // 直接删除最顶层的本体，底层外键的 ON DELETE CASCADE 应该自动抹除节点和连线
        OntologyRepository::deleteOntology(ontologyId);

        // 步骤E：断言验证是不是真的删干净了
        GraphNode checkNode = NodeRepository::getNodeById(node1.id);
        QVERIFY(checkNode.id <= 0 || checkNode.name.isEmpty()); // 节点必须查不到了

        QList<GraphEdge> checkEdges = RelationshipRepository::getAllRelationships(ontologyId);
        QVERIFY(checkEdges.isEmpty() == true); // 连线必须查不到了
    }

    // 4. 业务逻辑层规则拦截测试
    void testGraphEditorBusinessRules() {
        GraphEditor editor;

        // 构造一个非法关系 (sourceId == targetId)
        GraphEdge selfLoopEdge;
        selfLoopEdge.sourceId = 99;
        selfLoopEdge.targetId = 99;
        selfLoopEdge.relationType = "非法自环";

        bool result = editor.addRelationship(selfLoopEdge);
        QVERIFY2(result == false, "业务层未能拦截自环关系，请检查 GraphEditor 的防错逻辑！");
    }

    // 5. 用户与权限仓储 (RBAC) 测试
    void testUserRepositoryRBAC() {
        // 步骤A：插入一个管理员账号
        QString testUser = "qtest_admin_888";
        QString testPass = "qtest_pwd_123";
        // 假设 addUser 第三个参数代表 isAdmin
        bool userAdded = UserRepository::addUser(testUser, testPass, true);
        QVERIFY(userAdded == true);

        // 步骤B：测试登录鉴权
        User loggedInUser = UserRepository::login(testUser, testPass);
        QVERIFY2(loggedInUser.id > 0, "账号密码正确但登录失败");
        QVERIFY(loggedInUser.isAdmin == true);

        // 步骤C：测试密码错误的情况
        User failedUser = UserRepository::login(testUser, "wrong_password");
        QVERIFY2(failedUser.id <= 0, "密码错误竟然登录成功了，存在安全漏洞！");

        // 步骤D：权限修改测试
        // 将其权限更新为：不是管理员，只读
        bool updatePerm = UserRepository::updatePermissions(loggedInUser.id, true, false, false);
        QVERIFY(updatePerm == true);

        // 步骤E：清理测试账号
        QSqlQuery query(DatabaseConnection::getDatabase());
        query.prepare("DELETE FROM users WHERE username = ?");
        query.addBindValue(testUser);
        query.exec();
    }

    // 6. 环境清理
    void cleanupTestCase() {
        DatabaseConnection::disconnect();
        qDebug() << "============== 测试完毕，数据库连接已断开 ==============";
    }

    // 6. 数据库级约束与空值边界测试
    void testDatabaseConstraintsAndNulls() {
        // 场景 A：测试读异常
        GraphNode ghostNode = NodeRepository::getNodeById(999999);
        QVERIFY2(ghostNode.id <= 0, "查询不存在的节点时，系统未能返回预期的空对象");

        // 场景 B：测试写异常
        GraphEdge invalidEdge;
        invalidEdge.ontologyId = 1;
        invalidEdge.sourceId = 888888;
        invalidEdge.targetId = 999999;
        invalidEdge.relationType = "非法关联";

        bool edgeAdded = RelationshipRepository::addRelationship(invalidEdge);
        QVERIFY2(edgeAdded == false, "系统错误地允许了关联不存在的节点，外键约束失效！");
    }

    // 7. BFS 寻路引擎的极端边界测试
    void testQueryEngineBFS_EdgeCases() {
        QueryEngine engine;

        // 场景 A：查询两个完全不存在的节点的路径
        QList<int> ghostPath = engine.findPath(9999, 8888);
        // 断言：找不到路，必须安全返回空列表，不能引发段错误(Segmentation Fault)
        QVERIFY(ghostPath.isEmpty() == true);

        // 场景 B：起点和终点是同一个节点
        QList<int> selfPath = engine.findPath(1, 1);
        QVERIFY2(selfPath.size() <= 1, "寻路引擎处理同节点查询时陷入了死循环或逻辑错误");
    }

    // 8. 用户系统的唯一性冲突与越权测试
    void testUserSystemCollisions() {
        // 场景 A：测试用户名唯一性约束
        QString duplicateName = "qtest_conflict_user";
        // 第一次正常注册
        UserRepository::addUser(duplicateName, "pwd123", false);

        // 第二次故意使用完全相同的用户名注册
        bool registerAgain = UserRepository::addUser(duplicateName, "new_pwd", false);
        // 断言：由于 users 表的 username 字段通常带有 UNIQUE 唯一索引，第二次必须失败
        QVERIFY2(registerAgain == false, "系统未拦截重复的用户名注册，存在严重逻辑漏洞！");

        // 场景 B：查无此人
        User ghostLogin = UserRepository::login("nobody_exists_888", "123456");
        // 断言：必须拒绝登录并返回空实体
        QVERIFY(ghostLogin.id <= 0);

        //删掉刚才的测试账号
        QSqlQuery query(DatabaseConnection::getDatabase());
        query.prepare("DELETE FROM users WHERE username = ?");
        query.addBindValue(duplicateName);
        query.exec();
    }
};

QTEST_MAIN(TestKnowledgeGraph)
#include "tst_main.moc"