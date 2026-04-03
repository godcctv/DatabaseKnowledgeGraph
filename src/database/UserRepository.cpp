#include "UserRepository.h"
#include "DatabaseConnection.h"
#include <QSqlQuery>
#include <QVariant>
#include <QSqlError>
#include <QDebug>


bool UserRepository::registerUser(const QString& username, const QString& password) {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);
    query.prepare("INSERT INTO users (username, password, is_admin) VALUES (:username, :password, FALSE)");
    query.bindValue(":username", username);
    query.bindValue(":password", password);
    return query.exec();
}

User UserRepository::login(const QString& username, const QString& password) {
    User user;
    QSqlDatabase db = DatabaseConnection::getDatabase();
    if (!db.isOpen()) return user;

    QSqlQuery query(db);
    query.prepare("SELECT * FROM users WHERE username = :username AND password = :password");
    query.bindValue(":username", username);
    query.bindValue(":password", password);

    if (query.exec() && query.next()) {
        user.id = query.value("user_id").toInt();
        user.username = query.value("username").toString();
        user.password = query.value("password").toString();
        user.isAdmin = query.value("is_admin").toBool();
        // 读取权限
        user.canView = query.value("can_view").toBool();
        user.canEdit = query.value("can_edit").toBool();
        user.canDelete = query.value("can_delete").toBool();
    }
    return user;
}

QList<User> UserRepository::getAllUsers() {
    QList<User> users;
    QSqlDatabase db = DatabaseConnection::getDatabase();
    if (!db.isOpen()) return users;

    QSqlQuery query(db);
    if (query.exec("SELECT * FROM users ORDER BY user_id ASC")) {
        while (query.next()) {
            User u;
            u.id = query.value("user_id").toInt();
            u.username = query.value("username").toString();
            u.password = "******";
            u.isAdmin = query.value("is_admin").toBool();
            u.canView = query.value("can_view").toBool();
            u.canEdit = query.value("can_edit").toBool();
            u.canDelete = query.value("can_delete").toBool();
            users.append(u);
        }
    }
    return users;
}

bool UserRepository::addUser(const QString& username, const QString& password, bool isAdmin) {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);
    // 管理员默认拥有所有权限，普通用户默认只有查看权限
    query.prepare("INSERT INTO users (username, password, is_admin, can_view, can_edit, can_delete) "
                  "VALUES (:username, :password, :is_admin, :can_view, :can_edit, :can_delete)");
    query.bindValue(":username", username);
    query.bindValue(":password", password);
    query.bindValue(":is_admin", isAdmin);
    query.bindValue(":can_view", true);
    query.bindValue(":can_edit", isAdmin);
    query.bindValue(":can_delete", isAdmin);
    return query.exec();
}

// === 新增更新权限的方法 ===
bool UserRepository::updatePermissions(int id, bool canView, bool canEdit, bool canDelete) {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);
    query.prepare("UPDATE users SET can_view = :v, can_edit = :e, can_delete = :d WHERE user_id = :id");
    query.bindValue(":v", canView);
    query.bindValue(":e", canEdit);
    query.bindValue(":d", canDelete);
    query.bindValue(":id", id);
    return query.exec();
}

bool UserRepository::deleteUser(int id) {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);
    query.prepare("DELETE FROM users WHERE user_id = :id");
    query.bindValue(":id", id);
    return query.exec();
}

bool UserRepository::resetPassword(int id, const QString& newPassword) {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);
    query.prepare("UPDATE users SET password = :password WHERE user_id = :id");
    query.bindValue(":password", newPassword);
    query.bindValue(":id", id);
    return query.exec();
}