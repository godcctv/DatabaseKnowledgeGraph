#include "NodeRepository.h"
#include "DatabaseConnection.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QJsonDocument>
#include <QDebug>

bool NodeRepository::addNode(GraphNode& node) {
    int newId = -1;
    if (executeInsert(node, newId)) {
        node.id = newId; // 将数据库生成的自增ID写回对象
        return true;
    }
    return false;
}

bool NodeRepository::executeInsert(const GraphNode& node, int& outId) {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);

    // 对应你更新后的数据库字段：包含 pos_x, pos_y, color
    query.prepare("INSERT INTO node (ontology_id, node_type, name, description, pos_x, pos_y, color, properties) "
                  "VALUES (:oid, :type, :name, :desc, :x, :y, :color, :props)");

    query.bindValue(":oid", node.ontologyId);
    query.bindValue(":type", node.nodeType);
    query.bindValue(":name", node.name);
    query.bindValue(":desc", node.description);
    query.bindValue(":x", node.posX);
    query.bindValue(":y", node.posY);
    query.bindValue(":color", node.color);
    
    // 处理 JSON 字段
    QJsonDocument doc(node.properties);
    query.bindValue(":props", QString(doc.toJson(QJsonDocument::Compact)));

    if (!query.exec()) {
        qCritical() << "插入节点失败:" << query.lastError().text();
        return false;
    }

    outId = query.lastInsertId().toInt();
    return true;
}

bool NodeRepository::deleteNode(int nodeId) {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);
    query.prepare("DELETE FROM node WHERE node_id = :id");
    query.bindValue(":id", nodeId);
    
    return query.exec();
}

bool NodeRepository::updateNode(const GraphNode& node) {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);

    query.prepare("UPDATE node SET node_type = :type, name = :name, description = :desc, "
                  "pos_x = :x, pos_y = :y, color = :color, properties = :props "
                  "WHERE node_id = :id");

    query.bindValue(":type", node.nodeType);
    query.bindValue(":name", node.name);
    query.bindValue(":desc", node.description);
    query.bindValue(":x", node.posX);
    query.bindValue(":y", node.posY);
    query.bindValue(":color", node.color);
    query.bindValue(":id", node.id);
    
    QJsonDocument doc(node.properties);
    query.bindValue(":props", QString(doc.toJson(QJsonDocument::Compact)));

    return query.exec();
}

QList<GraphNode> NodeRepository::getAllNodes(int ontologyId) {
    QList<GraphNode> nodes;
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);

    query.prepare("SELECT * FROM node WHERE ontology_id = :oid");
    query.bindValue(":oid", ontologyId);

    if (query.exec()) {
        while (query.next()) {
            GraphNode node;
            node.id = query.value("node_id").toInt();
            node.ontologyId = query.value("ontology_id").toInt();
            node.nodeType = query.value("node_type").toString();
            node.name = query.value("name").toString();
            node.description = query.value("description").toString();
            node.posX = query.value("pos_x").toFloat();
            node.posY = query.value("pos_y").toFloat();
            node.color = query.value("color").toString();
            
            QByteArray jsonStr = query.value("properties").toByteArray();
            node.properties = QJsonDocument::fromJson(jsonStr).object();
            
            nodes.append(node);
        }
    }
    return nodes;
}

GraphNode NodeRepository::getNodeById(int nodeId) {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);
    query.prepare("SELECT * FROM node WHERE node_id = :id");
    query.bindValue(":id", nodeId);

    if (query.exec() && query.next()) {
        GraphNode node;
        node.id = query.value("node_id").toInt();
        // ... (其余字段填充逻辑同 getAllNodes)
        return node;
    }
    return GraphNode(); // 返回空对象
}