#include "NodeRepository.h"
#include "DatabaseConnection.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QJsonDocument>
#include <QDebug>

// --- 内部辅助函数声明 ---
static GraphNode mapQueryToNode(const QSqlQuery& query);

bool NodeRepository::addNode(GraphNode& node) {
    int newId = -1;
    if (executeInsert(node, newId)) {
        node.id = newId; // 将数据库生成的自增ID写回对象
        return true;
    }
    return false;
}

bool NodeRepository::executeInsert(const GraphNode& node, int& outId) {
    // ===== 问题5修复: 输入参数验证 =====
    if (node.ontologyId <= 0) {
        qWarning() << "NodeRepository: 无效的 ontologyId =" << node.ontologyId;
        return false;
    }
    if (node.name.trimmed().isEmpty()) {
        qWarning() << "NodeRepository: 节点名称不能为空";
        return false;
    }
    if (node.nodeType.isEmpty()) {
        qWarning() << "NodeRepository: 节点类型不能为空";
        return false;
    }

    QSqlDatabase db = DatabaseConnection::getDatabase();
    if (!db.isOpen()) {
        qCritical() << "NodeRepository: 数据库连接已关闭";
        return false;
    }

    QSqlQuery query(db);

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
    if (doc.isNull()) {
        qWarning() << "NodeRepository: 节点properties序列化失败";
        return false;
    }
    query.bindValue(":props", QString(doc.toJson(QJsonDocument::Compact)));

    if (!query.exec()) {
        qCritical() << "NodeRepository: 插入节点失败:" << query.lastError().text();
        return false;
    }

    outId = query.lastInsertId().toInt();
    if (outId <= 0) {
        qCritical() << "NodeRepository: 获取自增ID失败";
        return false;
    }

    return true;
}

bool NodeRepository::deleteNode(int nodeId) {
    // ===== 问题4修复: 完整的返回值检查 =====
    if (nodeId <= 0) {
        qWarning() << "NodeRepository: 无效的nodeId =" << nodeId;
        return false;
    }

    QSqlDatabase db = DatabaseConnection::getDatabase();
    if (!db.isOpen()) {
        qCritical() << "NodeRepository: 数据库连接已关闭";
        return false;
    }

    QSqlQuery query(db);
    query.prepare("DELETE FROM node WHERE node_id = :id");
    query.bindValue(":id", nodeId);

    if (!query.exec()) {
        qCritical() << "NodeRepository: 删除失败:" << query.lastError().text();
        return false;
    }

    // 关键: 检查实际删除的行数
    if (query.numRowsAffected() != 1) {
        qWarning() << "NodeRepository: 删除节点失败 - 期望删除1行，实际删除"
                   << query.numRowsAffected() << "行";
        return false;
    }

    return true;
}

bool NodeRepository::updateNode(const GraphNode& node) {
    // ===== 问题5修复: 参数验证 =====
    if (node.id <= 0) {
        qWarning() << "NodeRepository: 无效的node.id =" << node.id;
        return false;
    }
    if (node.name.trimmed().isEmpty()) {
        qWarning() << "NodeRepository: 节点名称不能为空";
        return false;
    }
    if (node.ontologyId <= 0) {
        qWarning() << "NodeRepository: 无效的ontologyId =" << node.ontologyId;
        return false;
    }

    QSqlDatabase db = DatabaseConnection::getDatabase();
    if (!db.isOpen()) {
        qCritical() << "NodeRepository: 数据库连接已关闭";
        return false;
    }

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
    if (doc.isNull()) {
        qWarning() << "NodeRepository: 节点properties序列化失败";
        return false;
    }
    query.bindValue(":props", QString(doc.toJson(QJsonDocument::Compact)));

    if (!query.exec()) {
        qCritical() << "NodeRepository: 更新失败:" << query.lastError().text();
        return false;
    }

    // ===== 问题4修复: 检查实际更新行数 =====
    if (query.numRowsAffected() != 1) {
        qWarning() << "NodeRepository: 更新失败 - 期望更新1行，实际更新"
                   << query.numRowsAffected() << "行";
        return false;
    }

    return true;
}

QList<GraphNode> NodeRepository::getAllNodes(int ontologyId) {
    QList<GraphNode> nodes;

    if (ontologyId <= 0) {
        qWarning() << "NodeRepository: 无效的ontologyId =" << ontologyId;
        return nodes;
    }

    QSqlDatabase db = DatabaseConnection::getDatabase();
    if (!db.isOpen()) {
        qCritical() << "NodeRepository: 数据库连接已关闭";
        return nodes;
    }

    QSqlQuery query(db);

    query.prepare("SELECT * FROM node WHERE ontology_id = :oid");
    query.bindValue(":oid", ontologyId);

    if (!query.exec()) {
        qCritical() << "NodeRepository: 查询所有节点失败:" << query.lastError().text();
        return nodes;
    }

    while (query.next()) {
        nodes.append(mapQueryToNode(query));
    }

    return nodes;
}

// ===== 问题1修复: 实现完整的字段映射 =====
GraphNode NodeRepository::getNodeById(int nodeId) {
    if (nodeId <= 0) {
        qWarning() << "NodeRepository: 无效的nodeId =" << nodeId;
        return GraphNode();
    }

    QSqlDatabase db = DatabaseConnection::getDatabase();
    if (!db.isOpen()) {
        qCritical() << "NodeRepository: 数据库连接已关闭";
        return GraphNode();
    }

    QSqlQuery query(db);
    query.prepare("SELECT * FROM node WHERE node_id = :id");
    query.bindValue(":id", nodeId);

    if (!query.exec()) {
        qCritical() << "NodeRepository: 查询节点失败:" << query.lastError().text();
        return GraphNode();
    }

    if (query.next()) {
        return mapQueryToNode(query);
    }

    // 未找到则返回无效对象 (id = -1)
    qDebug() << "NodeRepository: 未找到节点ID =" << nodeId;
    return GraphNode();
}

QList<GraphNode> NodeRepository::getNodesByType(int ontologyId, const QString& type) {
    QList<GraphNode> nodes;

    if (ontologyId <= 0 || type.isEmpty()) {
        qWarning() << "NodeRepository: 无效的参数 ontologyId =" << ontologyId << ", type =" << type;
        return nodes;
    }

    QSqlDatabase db = DatabaseConnection::getDatabase();
    if (!db.isOpen()) {
        qCritical() << "NodeRepository: 数据库连接已关闭";
        return nodes;
    }

    QSqlQuery query(db);

    query.prepare("SELECT * FROM node WHERE ontology_id = :oid AND node_type = :type");
    query.bindValue(":oid", ontologyId);
    query.bindValue(":type", type);

    if (!query.exec()) {
        qCritical() << "NodeRepository: 按类型查询节点失败:" << query.lastError().text();
        return nodes;
    }

    while (query.next()) {
        nodes.append(mapQueryToNode(query));
    }

    return nodes;
}

/**
 * @brief 核心映射函数：将QSqlQuery结果映射到GraphNode对象
 * 消除代码重复，保证字段映射的一致性（问题1的关键修复）
 */
static GraphNode mapQueryToNode(const QSqlQuery& query) {
    GraphNode node;
    node.id = query.value("node_id").toInt();
    node.ontologyId = query.value("ontology_id").toInt();
    node.nodeType = query.value("node_type").toString();
    node.name = query.value("name").toString();
    node.description = query.value("description").toString();
    node.posX = query.value("pos_x").toFloat();
    node.posY = query.value("pos_y").toFloat();
    node.color = query.value("color").toString();

    // 安全的JSON反序列化
    QByteArray jsonBytes = query.value("properties").toByteArray();
    if (!jsonBytes.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(jsonBytes);
        if (!doc.isNull()) {
            node.properties = doc.object();
        } else {
            qWarning() << "NodeRepository: JSON反序列化失败，节点ID =" << node.id;
        }
    }

    return node;
}