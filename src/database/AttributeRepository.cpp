#include "AttributeRepository.h"
#include "DatabaseConnection.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

bool AttributeRepository::addAttribute(Attribute& attr) {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);

    query.prepare("INSERT INTO attribute (node_id, relation_id, attr_name, attr_value, attr_type) "
                  "VALUES (:nid, :rid, :name, :val, :atyp)");

    if (attr.nodeId > 0) {
        query.bindValue(":nid", attr.nodeId);
        query.bindValue(":rid", QVariant(QVariant::Int)); // 写入 SQL NULL
    } else if (attr.relationId > 0) {
        query.bindValue(":nid", QVariant(QVariant::Int)); // 写入 SQL NULL
        query.bindValue(":rid", attr.relationId);
    } else {
        qWarning() << "AttributeRepository: 属性必须关联到一个节点或关系";
        return false;
    }

    query.bindValue(":name", attr.attrName);
    query.bindValue(":val", attr.attrValue);
    query.bindValue(":atyp", attr.attrType);

    if (!query.exec()) {
        qCritical() << "添加属性失败:" << query.lastError().text();
        return false;
    }

    attr.id = query.lastInsertId().toInt();
    return true;
}

QList<Attribute> AttributeRepository::getAttributesForEntity(const QString& entityType, int entityId) {
    QList<Attribute> attributes;
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);

    if (entityType == "NODE") {
        query.prepare("SELECT * FROM attribute WHERE node_id = :id");
    } else if (entityType == "RELATION") {
        query.prepare("SELECT * FROM attribute WHERE relation_id = :id");
    } else {
        qWarning() << "AttributeRepository: 未知的实体类型" << entityType;
        return attributes;
    }

    query.bindValue(":id", entityId);

    if (query.exec()) {
        while (query.next()) {
            Attribute attr;
            attr.id = query.value("attr_id").toInt();
            // 从数据库读取 node_id 和 relation_id
            // value().toInt() 如果是 NULL 会返回 0，这里我们可以视为 -1 或 0 处理
            attr.nodeId = query.value("node_id").isNull() ? -1 : query.value("node_id").toInt();
            attr.relationId = query.value("relation_id").isNull() ? -1 : query.value("relation_id").toInt();

            attr.attrName = query.value("attr_name").toString();
            attr.attrValue = query.value("attr_value").toString();
            attr.attrType = query.value("attr_type").toString();
            attributes.append(attr);
        }
    } else {
        qDebug() << "查询属性失败:" << query.lastError().text();
    }
    return attributes;
}

QList<Attribute> AttributeRepository::getAllAttributesByType(const QString& entityType) {
    QList<Attribute> attributes;
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);

    if (entityType == "NODE") {
        query.prepare("SELECT * FROM attribute WHERE node_id IS NOT NULL");
    } else {
        query.prepare("SELECT * FROM attribute WHERE relation_id IS NOT NULL");
    }

    if (query.exec()) {
        while (query.next()) {
            Attribute attr;
            attr.id = query.value("attr_id").toInt();
            attr.nodeId = query.value("node_id").isNull() ? -1 : query.value("node_id").toInt();
            attr.relationId = query.value("relation_id").isNull() ? -1 : query.value("relation_id").toInt();
            attr.attrName = query.value("attr_name").toString();
            attr.attrValue = query.value("attr_value").toString();
            attr.attrType = query.value("attr_type").toString();
            attributes.append(attr);
        }
    }
    return attributes;
}

bool AttributeRepository::deleteAttribute(int attrId) {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);
    query.prepare("DELETE FROM attribute WHERE attr_id = :id");
    query.bindValue(":id", attrId);
    return query.exec();
}

bool AttributeRepository::updateAttribute(const Attribute& attr) {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);
    query.prepare("UPDATE attribute SET attr_name = :name, attr_value = :val, attr_type = :atyp "
                  "WHERE attr_id = :id");
    query.bindValue(":name", attr.attrName);
    query.bindValue(":val", attr.attrValue);
    query.bindValue(":atyp", attr.attrType);
    query.bindValue(":id", attr.id);
    return query.exec();
}

bool AttributeRepository::deleteAttributesByEntity(const QString& entityType, int entityId) {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);

    if (entityType == "NODE") {
        query.prepare("DELETE FROM attribute WHERE node_id = :id");
    } else {
        query.prepare("DELETE FROM attribute WHERE relation_id = :id");
    }

    query.bindValue(":id", entityId);
    return query.exec();
}
