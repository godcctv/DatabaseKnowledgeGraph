#include "AttributeRepository.h"
#include "DatabaseConnection.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

bool AttributeRepository::addAttribute(Attribute& attr) {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);

    // 对应 SQL: entity_type, entity_id, attr_name, attr_value, attr_type
    query.prepare("INSERT INTO attribute (entity_type, entity_id, attr_name, attr_value, attr_type) "
                  "VALUES (:type, :eid, :name, :val, :atyp)");
    
    query.bindValue(":type", attr.entityType); // 传入 "NODE" 或 "RELATION"
    query.bindValue(":eid", attr.entityId);
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

    query.prepare("SELECT * FROM attribute WHERE entity_type = :type AND entity_id = :eid");
    query.bindValue(":type", entityType);
    query.bindValue(":eid", entityId);

    if (query.exec()) {
        while (query.next()) {
            Attribute attr;
            attr.id = query.value("attr_id").toInt();
            attr.entityType = query.value("entity_type").toString();
            attr.entityId = query.value("entity_id").toInt();
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
    query.prepare("DELETE FROM attribute WHERE entity_type = :type AND entity_id = :eid");
    query.bindValue(":type", entityType);
    query.bindValue(":eid", entityId);
    return query.exec();
}