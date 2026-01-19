-- 插入示例本体
INSERT INTO ontology (name, description, version) VALUES
    ('数据库原理', '数据库系统核心知识领域', '1.0');

-- 插入示例节点
INSERT INTO node (ontology_id, node_type, name, description) VALUES
                                                                 (1, 'concept', '数据库系统', '数据库管理系统的基本概念'),
                                                                 (1, 'concept', '关系模型', '关系数据模型'),
                                                                 (1, 'concept', 'SQL', '结构化查询语言'),
                                                                 (1, 'concept', '事务', '数据库事务');

-- 插入示例关系
INSERT INTO relationship (ontology_id, source_id, target_id, relation_type) VALUES
                                                                                (1, 1, 2, 'has_component'),
                                                                                (1, 2, 3, 'uses'),
                                                                                (1, 1, 4, 'has_component');