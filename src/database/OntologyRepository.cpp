#include "OntologyRepository.h"
#include "DatabaseConnection.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

bool OntologyRepository::addOntology(Ontology& ontology) {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);

    query.prepare("INSERT INTO ontology (name, description, version) "
                  "VALUES (:name, :desc, :ver)");
    query.bindValue(":name", ontology.name);
    query.bindValue(":desc", ontology.description);
    query.bindValue(":ver", ontology.version);

    if (!query.exec()) {
        qCritical() << "创建本体失败:" << query.lastError().text();
        return false;
    }

    // 获取数据库生成的自增 ID 并回填给对象
    ontology.id = query.lastInsertId().toInt();
    return true;
}

bool OntologyRepository::deleteOntology(int id) {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);
    query.prepare("DELETE FROM ontology WHERE ontology_id = :id");
    query.bindValue(":id", id);
    return query.exec();
}

bool OntologyRepository::updateOntology(const Ontology& ontology) {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);
    query.prepare("UPDATE ontology SET name = :name, description = :desc, version = :ver "
                  "WHERE ontology_id = :id");
    query.bindValue(":name", ontology.name);
    query.bindValue(":desc", ontology.description);
    query.bindValue(":ver", ontology.version);
    query.bindValue(":id", ontology.id);
    return query.exec();
}

QList<Ontology> OntologyRepository::getAllOntologies() {
    QList<Ontology> list;
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query("SELECT * FROM ontology ORDER BY updated_at DESC", db);

    while (query.next()) {
        Ontology o;
        o.id = query.value("ontology_id").toInt();
        o.name = query.value("name").toString();
        o.description = query.value("description").toString();
        o.version = query.value("version").toString();
        o.createdAt = query.value("created_at").toDateTime();
        o.updatedAt = query.value("updated_at").toDateTime();
        list.append(o);
    }
    return list;
}

Ontology OntologyRepository::getOntologyById(int id) {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);
    query.prepare("SELECT * FROM ontology WHERE ontology_id = :id");
    query.bindValue(":id", id);

    if (query.exec() && query.next()) {
        Ontology o;
        o.id = query.value("ontology_id").toInt();
        o.name = query.value("name").toString();
        o.description = query.value("description").toString();
        o.version = query.value("version").toString();
        return o;
    }
    return Ontology();
}