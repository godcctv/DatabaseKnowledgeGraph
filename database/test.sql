-- 确保使用正确的数据库
USE DatabaseKnowledgeGraph;

-- ⚠️ 注意：此脚本假设你已经运行过 init.sql（表已清空）
-- 如果没有，请先运行 init.sql，或者手动清空表（DELETE FROM ontology;）

-- ==========================================
-- 1. 插入示例本体 (Ontology)
-- ==========================================
INSERT INTO ontology (ontology_id, name, description, version) VALUES
    (1, '数据库原理', '数据库系统核心知识领域', '1.0');

-- ==========================================
-- 2. 插入示例节点 (Node)
-- 包含 pos_x, pos_y, color 和 JSON 属性的演示
-- ==========================================
INSERT INTO node (node_id, ontology_id, node_type, name, description, pos_x, pos_y, color, properties) VALUES
                                                                                                           (1, 1, 'concept', '数据库系统', '数据库管理系统的基本概念', 0, 0, '#FF5733', '{"importance": "high"}'),
                                                                                                           (2, 1, 'concept', '关系模型', '基于关系代数的数据模型', 200, 0, '#33FF57', '{"complexity": "medium"}'),
                                                                                                           (3, 1, 'concept', 'SQL', '结构化查询语言', 0, 200, '#3357FF', '{"standard": "ANSI"}'),
                                                                                                           (4, 1, 'concept', '事务', '数据库操作的逻辑单位', 200, 200, '#F3FF33', '{"ACID": true}');

-- ==========================================
-- 3. 插入示例关系 (Relationship)
-- ==========================================
INSERT INTO relationship (relation_id, ontology_id, source_id, target_id, relation_type, weight) VALUES
                                                                                                     (1, 1, 1, 2, 'has_component', 1.0), -- 数据库系统 包含 关系模型
                                                                                                     (2, 1, 2, 3, 'uses', 0.8),          -- 关系模型 使用 SQL
                                                                                                     (3, 1, 1, 4, 'supports', 0.9);      -- 数据库系统 支持 事务

-- ==========================================
-- 4. 插入示例属性 (Attribute) - 🔥 关键修改 🔥
-- 适配新的 node_id / relation_id 分离结构
-- ==========================================

-- 4.1 给节点添加属性 (node_id 有值, relation_id 为 NULL)
INSERT INTO attribute (node_id, relation_id, attr_name, attr_value, attr_type) VALUES
                                                                                   (1, NULL, '别名', 'DBS', 'String'),
                                                                                   (1, NULL, '教学难度', '5星', 'String'),
                                                                                   (4, NULL, '隔离级别', 'Serializable', 'String');

-- 4.2 给关系添加属性 (node_id 为 NULL, relation_id 有值)
INSERT INTO attribute (node_id, relation_id, attr_name, attr_value, attr_type) VALUES
                                                                                   (NULL, 1, '连接强度', '强依赖', 'String'),
                                                                                   (NULL, 2, '历史版本', 'SQL-92', 'String');