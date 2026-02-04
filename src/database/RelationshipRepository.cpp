#include "RelationshipRepository.h"
#include "DatabaseConnection.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QJsonDocument>
#include <QDebug>

// --- 内部辅助函数声明 ---
static GraphEdge mapQueryToEdge(const QSqlQuery& query);

bool RelationshipRepository::addRelationship(GraphEdge& edge) {
    // ===== 问题5修复: 输入参数验证 =====
    if (edge.ontologyId <= 0 || edge.sourceId <= 0 || edge.targetId <= 0) {
        qWarning() << "RelationshipRepository: 无效的参数"
                   << "ontologyId =" << edge.ontologyId
                   << "sourceId =" << edge.sourceId
                   << "targetId =" << edge.targetId;
        return false;
    }
    if (edge.sourceId == edge.targetId) {
        qWarning() << "RelationshipRepository: 源节点和目标节点不能相同";
        return false;
    }
    if (edge.relationType.isEmpty()) {
        qWarning() << "RelationshipRepository: 关系类型不能为空";
        return false;
    }

    QSqlDatabase db = DatabaseConnection::getDatabase();
    if (!db.isOpen()) {
        qCritical() << "RelationshipRepository: 数据库连接已关闭";
        return false;
    }

    QSqlQuery query(db);

    query.prepare("INSERT INTO relationship (ontology_id, source_id, target_id, relation_type, weight, properties) "
                  "VALUES (:oid, :sid, :tid, :type, :weight, :props)");

    query.bindValue(":oid", edge.ontologyId);
    query.bindValue(":sid", edge.sourceId);
    query.bindValue(":tid", edge.targetId);
    query.bindValue(":type", edge.relationType);
    query.bindValue(":weight", edge.weight);

    // 处理 JSON 字段
    QJsonDocument doc(edge.properties);
    if (doc.isNull()) {
        qWarning() << "RelationshipRepository: 关系properties序列化失败";
        return false;
    }
    query.bindValue(":props", QString(doc.toJson(QJsonDocument::Compact)));

    if (!query.exec()) {
        qCritical() << "RelationshipRepository: 创建关系失败:" << query.lastError().text();
        return false;
    }

    edge.id = query.lastInsertId().toInt();
    if (edge.id <= 0) {
        qCritical() << "RelationshipRepository: 获取自增ID失败";
        return false;
    }

    return true;
}

bool RelationshipRepository::deleteRelationship(int relationId) {
    // ===== 问题4修复: 完整的返回值检查 =====
    if (relationId <= 0) {
        qWarning() << "RelationshipRepository: 无效的relationId =" << relationId;
        return false;
    }

    QSqlDatabase db = DatabaseConnection::getDatabase();
    if (!db.isOpen()) {
        qCritical() << "RelationshipRepository: 数据库连接已关闭";
        return false;
    }

    QSqlQuery query(db);
    query.prepare("DELETE FROM relationship WHERE relation_id = :id");
    query.bindValue(":id", relationId);

    if (!query.exec()) {
        qCritical() << "RelationshipRepository: 删除失败:" << query.lastError().text();
        return false;
    }

    // 关键: 检查实际删除的行数
    if (query.numRowsAffected() != 1) {
        qWarning() << "RelationshipRepository: 删除关系失败 - 期望删除1行，实际删除"
                   << query.numRowsAffected() << "行";
        return false;
    }

    return true;
}

bool RelationshipRepository::updateRelationship(const GraphEdge& edge) {
    // ===== 问题5修复: 参数验证 =====
    if (edge.id <= 0) {
        qWarning() << "RelationshipRepository: 无效的edge.id =" << edge.id;
        return false;
    }
    if (edge.relationType.isEmpty()) {
        qWarning() << "RelationshipRepository: 关系类型不能为空";
        return false;
    }

    QSqlDatabase db = DatabaseConnection::getDatabase();
    if (!db.isOpen()) {
        qCritical() << "RelationshipRepository: 数据库连接已关闭";
        return false;
    }

    QSqlQuery query(db);

    query.prepare("UPDATE relationship SET relation_type = :type, weight = :weight, properties = :props "
                  "WHERE relation_id = :id");

    query.bindValue(":type", edge.relationType);
    query.bindValue(":weight", edge.weight);
    query.bindValue(":id", edge.id);

    QJsonDocument doc(edge.properties);
    if (doc.isNull()) {
        qWarning() << "RelationshipRepository: 关系properties序列化失败";
        return false;
    }
    query.bindValue(":props", QString(doc.toJson(QJsonDocument::Compact)));

    if (!query.exec()) {
        qCritical() << "RelationshipRepository: 更新失败:" << query.lastError().text();
        return false;
    }

    // ===== 问题4修复: 检查实际更新行数 =====
    if (query.numRowsAffected() != 1) {
        qWarning() << "RelationshipRepository: 更新失败 - 期望更新1行，实际更新"
                   << query.numRowsAffected() << "行";
        return false;
    }

    return true;
}

QList<GraphEdge> RelationshipRepository::getEdgesByOntology(int ontologyId) {
    QList<GraphEdge> edges;

    if (ontologyId <= 0) {
        qWarning() << "RelationshipRepository: 无效的ontologyId =" << ontologyId;
        return edges;
    }

    QSqlDatabase db = DatabaseConnection::getDatabase();
    if (!db.isOpen()) {
        qCritical() << "RelationshipRepository: 数据库连接已关闭";
        return edges;
    }

    QSqlQuery query(db);

    query.prepare("SELECT * FROM relationship WHERE ontology_id = :oid");
    query.bindValue(":oid", ontologyId);

    if (!query.exec()) {
        qCritical() << "RelationshipRepository: 查询关系失败:" << query.lastError().text();
        return edges;
    }

    while (query.next()) {
        edges.append(mapQueryToEdge(query));
    }

    return edges;
}

QList<GraphEdge> RelationshipRepository::getEdgesByNode(int nodeId) {
    QList<GraphEdge> edges;

    if (nodeId <= 0) {
        qWarning() << "RelationshipRepository: 无效的nodeId =" << nodeId;
        return edges;
    }

    QSqlDatabase db = DatabaseConnection::getDatabase();
    if (!db.isOpen()) {
        qCritical() << "RelationshipRepository: 数据库连接已关闭";
        return edges;
    }

    QSqlQuery query(db);

    // 查询该节点作为起点或终点的所有关系
    query.prepare("SELECT * FROM relationship WHERE source_id = :id OR target_id = :id");
    query.bindValue(":id", nodeId);

    if (!query.exec()) {
        qCritical() << "RelationshipRepository: 查询关系失败:" << query.lastError().text();
        return edges;
    }

    while (query.next()) {
        edges.append(mapQueryToEdge(query));
    }

    return edges;
}

GraphEdge RelationshipRepository::getRelationshipById(int relationId) {
    if (relationId <= 0) {
        qWarning() << "RelationshipRepository: 无效的relationId =" << relationId;
        return GraphEdge();
    }

    QSqlDatabase db = DatabaseConnection::getDatabase();
    if (!db.isOpen()) {
        qCritical() << "RelationshipRepository: 数据库连接已关闭";
        return GraphEdge();
    }

    QSqlQuery query(db);
    query.prepare("SELECT * FROM relationship WHERE relation_id = :id");
    query.bindValue(":id", relationId);

    if (!query.exec()) {
        qCritical() << "RelationshipRepository: 查询关系失败:" << query.lastError().text();
        return GraphEdge();
    }

    if (query.next()) {
        return mapQueryToEdge(query);
    }

    // 未找到则返回无效对象
    qDebug() << "RelationshipRepository: 未找到关系ID =" << relationId;
    return GraphEdge();
}


static GraphEdge mapQueryToEdge(const QSqlQuery& query) {
    GraphEdge edge;
    edge.id = query.value("relation_id").toInt();
    edge.ontologyId = query.value("ontology_id").toInt();
    edge.sourceId = query.value("source_id").toInt();
    edge.targetId = query.value("target_id").toInt();
    edge.relationType = query.value("relation_type").toString();
    edge.weight = query.value("weight").toFloat();

    // 安全的JSON反序列化
    QByteArray jsonData = query.value("properties").toByteArray();
    if (!jsonData.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(jsonData);
        if (!doc.isNull()) {
            edge.properties = doc.object();
        } else {
            qWarning() << "RelationshipRepository: JSON反序列化失败，关系ID =" << edge.id;
        }
    }

    return edge;
}

// src/database/RelationshipRepository.cpp

QList<GraphEdge> RelationshipRepository::getAllRelationships(int ontologyId) {
    QList<GraphEdge> edges;

    QSqlDatabase db = DatabaseConnection::getDatabase();

    QSqlQuery query(db);
    query.prepare("SELECT relation_id, source_id, target_id, relation_type, weight FROM relationship WHERE ontology_id = ?");
    query.addBindValue(ontologyId);

    if (query.exec()) {
        while (query.next()) {
            GraphEdge edge;
            edge.id = query.value("relation_id").toInt();
            edge.sourceId = query.value("source_id").toInt();
            edge.targetId = query.value("target_id").toInt();
            edge.relationType = query.value("relation_type").toString();
            edge.weight = query.value("weight").toFloat();
            edge.ontologyId = ontologyId;
            edges.append(edge);
        }
    } else {
        // 如果出错，打印出来方便调试
        qCritical() << "加载关系失败 (SQL错误):" << query.lastError().text();
    }
    return edges;
}

bool RelationshipRepository::relationshipExists(int sourceId, int targetId, const QString& type) {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);

    // 查询是否存在 源ID、目标ID、类型 都完全一样的记录
    query.prepare("SELECT count(*) FROM relationship WHERE source_id = ? AND target_id = ? AND relation_type = ?");
    query.addBindValue(sourceId);
    query.addBindValue(targetId);
    query.addBindValue(type);

    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    return false;
}