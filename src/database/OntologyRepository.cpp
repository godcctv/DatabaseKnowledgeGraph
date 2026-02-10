#include "OntologyRepository.h"
#include "DatabaseConnection.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDateTime>

void OntologyRepository::initDatabase() {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    if (!db.isOpen()) return;

    QSqlQuery query(db);

    // 🔥 修复 1: 保持列名一致性，使用 ontology_id 🔥
    // 这样能兼容 init.sql 创建的表结构
    bool success = query.exec(
        "CREATE TABLE IF NOT EXISTS ontology ("
        "ontology_id INTEGER PRIMARY KEY AUTO_INCREMENT, "
        "name VARCHAR(100) UNIQUE, "
        "description TEXT, "
        "version VARCHAR(20) DEFAULT '1.0', "
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "updated_at DATETIME DEFAULT CURRENT_TIMESTAMP)"
    );

    if (!success) {
        // 如果表已存在且结构不同，这里也不会报错，但没关系，我们下面的代码会适配 ontology_id
        qDebug() << "Init ontology table info:" << query.lastError().text();
    }

    // 插入默认数据
    query.exec("SELECT COUNT(*) FROM ontology");
    if (query.next() && query.value(0).toInt() == 0) {
        addOntology("默认图谱", "系统初始化的示例图谱");
        addOntology("计算机网络", "协议、分层与硬件知识");
    }
}

QList<Ontology> OntologyRepository::getAllOntologies() {
    QList<Ontology> list;
    QSqlDatabase db = DatabaseConnection::getDatabase();
    if (!db.isOpen()) return list;

    QSqlQuery query(db);
    // 🔥 修复 2: 排序字段改为 ontology_id 🔥
    if (!query.exec("SELECT * FROM ontology ORDER BY ontology_id ASC")) {
        qDebug() << "Query ontologies failed:" << query.lastError().text();
        return list;
    }

    while (query.next()) {
        Ontology o;
        // 🔥 修复 3: 读取字段改为 ontology_id 🔥
        // 尝试读取 ontology_id，如果不存在则尝试读取 id (兼容性处理)
        QVariant idVal = query.value("ontology_id");
        if (!idVal.isValid()) idVal = query.value("id");

        o.id = idVal.toInt();
        o.name = query.value("name").toString();
        o.description = query.value("description").toString();
        o.version = query.value("version").toString();
        o.createdAt = query.value("created_at").toDateTime();
        o.updatedAt = query.value("updated_at").toDateTime();
        list.append(o);
    }
    return list;
}

bool OntologyRepository::addOntology(const QString& name, const QString& desc) {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare("INSERT INTO ontology (name, description, created_at, updated_at) "
                  "VALUES (:name, :desc, :now, :now)");
    query.bindValue(":name", name);
    query.bindValue(":desc", desc);
    query.bindValue(":now", QDateTime::currentDateTime());

    if (!query.exec()) {
        qDebug() << "Add Ontology Error:" << query.lastError().text();
        return false;
    }
    return true;
}

bool OntologyRepository::deleteOntology(int id) {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    if (!db.isOpen()) return false;

    db.transaction();
    QSqlQuery query(db);

    // 🔥 修复 4: Where 条件改为 ontology_id 🔥

    // 1. 删除关系 (relationship 表)
    query.prepare("DELETE FROM relationship WHERE ontology_id = :id");
    query.bindValue(":id", id);
    query.exec();

    // 2. 删除节点 (node 表)
    query.prepare("DELETE FROM node WHERE ontology_id = :id");
    query.bindValue(":id", id);
    query.exec();

    // 3. 删除本体 (ontology 表)
    query.prepare("DELETE FROM ontology WHERE ontology_id = :id");
    query.bindValue(":id", id);
    bool success = query.exec();

    if (success) db.commit();
    else db.rollback();

    return success;
}

Ontology OntologyRepository::getOntologyById(int id) {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);

    // 🔥 修复 5: Where 条件改为 ontology_id 🔥
    query.prepare("SELECT * FROM ontology WHERE ontology_id = :id");
    query.bindValue(":id", id);

    if (query.exec() && query.next()) {
        Ontology o;
        o.id = query.value("ontology_id").toInt();
        o.name = query.value("name").toString();
        o.description = query.value("description").toString();
        return o;
    }
    return Ontology();
}