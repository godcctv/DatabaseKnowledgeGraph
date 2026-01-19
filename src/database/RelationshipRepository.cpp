#include "RelationshipRepository.h"
#include "DatabaseConnection.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QJsonDocument>
#include <QDebug>

bool RelationshipRepository::addRelationship(GraphEdge& edge) {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);

    // 字段对应：ontology_id, source_id, target_id, relation_type, weight, properties
    query.prepare("INSERT INTO relationship (ontology_id, source_id, target_id, relation_type, weight, properties) "
                  "VALUES (:oid, :sid, :tid, :type, :weight, :props)");

    query.bindValue(":oid", edge.ontologyId);
    query.bindValue(":sid", edge.sourceId);
    query.bindValue(":tid", edge.targetId);
    query.bindValue(":type", edge.relationType);
    query.bindValue(":weight", edge.weight);
    
    // 将 QJsonObject 转为字符串存入数据库
    QJsonDocument doc(edge.properties);
    query.bindValue(":props", QString(doc.toJson(QJsonDocument::Compact)));

    if (!query.exec()) {
        qCritical() << "创建关系失败:" << query.lastError().text();
        return false;
    }

    edge.id = query.lastInsertId().toInt();
    return true;
}

bool RelationshipRepository::deleteRelationship(int relationId) {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);
    query.prepare("DELETE FROM relationship WHERE relation_id = :id");
    query.bindValue(":id", relationId);
    return query.exec();
}

bool RelationshipRepository::updateRelationship(const GraphEdge& edge) {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);

    query.prepare("UPDATE relationship SET relation_type = :type, weight = :weight, properties = :props "
                  "WHERE relation_id = :id");

    query.bindValue(":type", edge.relationType);
    query.bindValue(":weight", edge.weight);
    query.bindValue(":id", edge.id);
    
    QJsonDocument doc(edge.properties);
    query.bindValue(":props", QString(doc.toJson(QJsonDocument::Compact)));

    return query.exec();
}

QList<GraphEdge> RelationshipRepository::getEdgesByOntology(int ontologyId) {
    QList<GraphEdge> edges;
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);

    query.prepare("SELECT * FROM relationship WHERE ontology_id = :oid");
    query.bindValue(":oid", ontologyId);

    if (query.exec()) {
        while (query.next()) {
            GraphEdge e;
            e.id = query.value("relation_id").toInt();
            e.ontologyId = query.value("ontology_id").toInt();
            e.sourceId = query.value("source_id").toInt();
            e.targetId = query.value("target_id").toInt();
            e.relationType = query.value("relation_type").toString();
            e.weight = query.value("weight").toFloat();
            
            QByteArray jsonData = query.value("properties").toByteArray();
            e.properties = QJsonDocument::fromJson(jsonData).object();
            
            edges.append(e);
        }
    }
    return edges;
}

QList<GraphEdge> RelationshipRepository::getEdgesByNode(int nodeId) {
    QList<GraphEdge> edges;
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);

    // 查询该节点作为起点或终点的所有关系
    query.prepare("SELECT * FROM relationship WHERE source_id = :id OR target_id = :id");
    query.bindValue(":id", nodeId);

    if (query.exec()) {
        while (query.next()) {
            GraphEdge e;
            e.id = query.value("relation_id").toInt();
            e.sourceId = query.value("source_id").toInt();
            e.targetId = query.value("target_id").toInt();
            e.relationType = query.value("relation_type").toString();
            edges.append(e);
        }
    }
    return edges;
}