#include "DatabaseConnection.h"
#include <QSqlError>
#include <QDebug>

// 静态成员变量初始化
std::unique_ptr<QSqlDatabase> DatabaseConnection::instance = nullptr;

bool DatabaseConnection::connect(const DatabaseConfig& config) {
    // 1. 如果 instance 不存在，则初始化 QSqlDatabase 实例
    if (!instance) {
        // 创建名为 "KnowledgeGraphConnection" 的连接，避免与默认连接冲突
        instance = std::make_unique<QSqlDatabase>(QSqlDatabase::addDatabase("QMYSQL", "KG_CONN"));
    }

    // 2. 设置连接参数
    instance->setHostName(config.hostname);
    instance->setUserName(config.username);
    instance->setPassword(config.password);
    instance->setDatabaseName(config.database);
    instance->setPort(config.port);

    // 3. 尝试打开连接
    if (!instance->open()) {
        qCritical() << "数据库连接失败！错误详情:" << instance->lastError().text();
        return false;
    }

    qInfo() << "成功连接到数据库:" << config.database << "于主机:" << config.hostname;
    return true;
}

QSqlDatabase DatabaseConnection::getDatabase() {
    // 检查 instance 是否有效且处于打开状态
    if (instance && instance->isOpen()) {
        return QSqlDatabase::database("KG_CONN");
    }
    qWarning() << "尝试获取未连接或已断开的数据库实例。";
    return QSqlDatabase();
}

bool DatabaseConnection::isConnected() {
    return instance && instance->isOpen();
}

void DatabaseConnection::disconnect() {
    if (instance) {
        if (instance->isOpen()) {
            instance->close();
        }
        // 手动移除数据库连接名
        QSqlDatabase::removeDatabase("KG_CONN");
        instance.reset(); // 释放内存
    }
}