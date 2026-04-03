-- 1. 切换到目标数据库
USE DatabaseKnowledgeGraph;

-- 2. 清理旧测试数据（可选，由于 init.sql 已经 DROP TABLE，这里主要是为了多次运行 test.sql 时保持干净）
SET FOREIGN_KEY_CHECKS = 0;
TRUNCATE TABLE attribute;
TRUNCATE TABLE relationship;
TRUNCATE TABLE node;
TRUNCATE TABLE ontology;
TRUNCATE TABLE users;
SET FOREIGN_KEY_CHECKS = 1;

-- 3. 插入一个本体 (Ontology)
INSERT INTO ontology (name, description, version)
VALUES ('计算机科学图谱', '关于编程语言与数据库的关系映射', '1.1');

-- 获取刚刚生成的 ontology_id
SET @ont_id = LAST_INSERT_ID();

-- 4. 插入节点 (Nodes)
INSERT INTO node (ontology_id, node_type, name, description, color) VALUES
                                                                        (@ont_id, 'Language', 'C++', '一种高性能的编译型语言', '#ff5733'),
                                                                        (@ont_id, 'Database', 'MySQL', '流行的开源关系型数据库', '#3498db'),
                                                                        (@ont_id, 'Concept', '数据库管理系统', '用于存储和管理数据的软件', '#2ecc71');

-- 获取节点的 ID 用于建立关系
SET @node_cpp = (SELECT node_id FROM node WHERE name = 'C++' AND ontology_id = @ont_id);
SET @node_mysql = (SELECT node_id FROM node WHERE name = 'MySQL' AND ontology_id = @ont_id);
SET @node_dbms = (SELECT node_id FROM node WHERE name = '数据库管理系统' AND ontology_id = @ont_id);

-- 5. 插入关系 (Relationships)
INSERT INTO relationship (ontology_id, source_id, target_id, relation_type, weight) VALUES
                                                                                        (@ont_id, @node_cpp, @node_mysql, '连接/操作', 0.8),
                                                                                        (@ont_id, @node_mysql, @node_dbms, '属于', 1.0);

-- 获取关系的 ID 用于添加属性
SET @rel_id = (SELECT relation_id FROM relationship WHERE source_id = @node_cpp AND target_id = @node_mysql);

-- 6. 插入属性 (Attributes)
-- 为节点添加属性
INSERT INTO attribute (node_id, attr_name, attr_value, attr_type) VALUES
                                                                      (@node_cpp, 'Creator', 'Bjarne Stroustrup', 'String'),
                                                                      (@node_mysql, 'Port', '3306', 'Integer');

-- 为关系添加属性
INSERT INTO attribute (relation_id, attr_name, attr_value, attr_type) VALUES
    (@rel_id, 'Driver', 'MySQL Connector/C++', 'String');

-- 7. 插入用户 (Users)
INSERT INTO users (username, password, is_admin, can_edit) VALUES
                                                               ('admin', 'admin123', TRUE, TRUE),
                                                               ('viewer', 'guest123', FALSE, FALSE);

-- ==========================================
-- 验证查询部分
-- ==========================================

SELECT '--- 验证：节点与本体关联 ---' AS '';
SELECT n.name AS 节点名, o.name AS 所属本体, n.node_type
FROM node n
         JOIN ontology o ON n.ontology_id = o.ontology_id;

SELECT '--- 验证：关系映射 (Source -> Relation -> Target) ---' AS '';
SELECT
    s.name AS 起点,
    r.relation_type AS 关系类型,
    t.name AS 终点
FROM relationship r
         JOIN node s ON r.source_id = s.node_id
         JOIN node t ON r.target_id = t.node_id;

SELECT '--- 验证：属性分布 ---' AS '';
SELECT
    COALESCE(n.name, '关系属性') AS 归属对象,
    attr_name AS 属性名,
    attr_value AS 属性值
FROM attribute a
         LEFT JOIN node n ON a.node_id = n.node_id;

-- 8. 级联删除测试 (可选注释掉)
-- DELETE FROM ontology WHERE ontology_id = @ont_id;
-- SELECT '--- 验证级联删除：执行后 node 表行数（应为0） ---' AS '';
-- SELECT COUNT(*) FROM node;