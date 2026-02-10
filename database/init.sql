-- 确保使用正确的数据库
CREATE DATABASE IF NOT EXISTS DatabaseKnowledgeGraph;
USE DatabaseKnowledgeGraph;

-- 按照外键依赖的反向顺序删除旧表，避免报错
DROP TABLE IF EXISTS attribute;
DROP TABLE IF EXISTS relationship;
DROP TABLE IF EXISTS node;
DROP TABLE IF EXISTS ontology;

-- 1. 本体表
CREATE TABLE ontology (
                          ontology_id INT PRIMARY KEY AUTO_INCREMENT,
                          name VARCHAR(255) NOT NULL UNIQUE,
                          description TEXT,
                          version VARCHAR(50) NOT NULL DEFAULT '1.0',
                          created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                          updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 2. 节点表
CREATE TABLE node (
                      node_id INT PRIMARY KEY AUTO_INCREMENT,
                      ontology_id INT NOT NULL,
                      node_type VARCHAR(100) NOT NULL,
                      name VARCHAR(255) NOT NULL,
                      description TEXT,
                      pos_x FLOAT DEFAULT 0,
                      pos_y FLOAT DEFAULT 0,
                      color VARCHAR(20) DEFAULT '#3498db',
                      properties LONGTEXT,
                      created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                      FOREIGN KEY (ontology_id) REFERENCES ontology(ontology_id) ON DELETE CASCADE,
                      UNIQUE KEY unique_node (ontology_id, name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 3. 关系表
CREATE TABLE relationship (
                              relation_id INT PRIMARY KEY AUTO_INCREMENT,
                              ontology_id INT NOT NULL,
                              source_id INT NOT NULL,
                              target_id INT NOT NULL,
                              relation_type VARCHAR(100) NOT NULL,
                              weight FLOAT DEFAULT 1.0,
                              properties LONGTEXT,
                              FOREIGN KEY (ontology_id) REFERENCES ontology(ontology_id) ON DELETE CASCADE,
                              FOREIGN KEY (source_id) REFERENCES node(node_id) ON DELETE CASCADE,
                              FOREIGN KEY (target_id) REFERENCES node(node_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 4. 属性表
CREATE TABLE attribute (
                           attr_id INT PRIMARY KEY AUTO_INCREMENT,
                           node_id INT DEFAULT NULL,
                           relation_id INT DEFAULT NULL,
                           attr_name VARCHAR(255) NOT NULL,
                           attr_value TEXT,
                           attr_type VARCHAR(50),
                           FOREIGN KEY (node_id) REFERENCES node(node_id) ON DELETE CASCADE,
                           FOREIGN KEY (relation_id) REFERENCES relationship(relation_id) ON DELETE CASCADE,
                           CONSTRAINT chk_entity_source CHECK (
                               (node_id IS NOT NULL AND relation_id IS NULL) OR
                               (node_id IS NULL AND relation_id IS NOT NULL)
                               )
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;