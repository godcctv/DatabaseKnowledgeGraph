#include "UserRepository.h"
#include "DatabaseConnection.h"
#include <QSqlQuery>
#include <QVariant>
#include <QSqlError>
#include <QDebug>

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
    } else {
        qDebug() << "Login query failed or user not found:" << query.lastError().text();
    }
    
    return user;
}

bool UserRepository::registerUser(const QString& username, const QString& password) {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);
    query.prepare("INSERT INTO users (username, password, is_admin) VALUES (:username, :password, FALSE)");
    query.bindValue(":username", username);
    query.bindValue(":password", password);
    return query.exec();
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
            users.append(u);
        }
    }
    return users;
}

bool UserRepository::addUser(const QString& username, const QString& password, bool isAdmin) {
    QSqlDatabase db = DatabaseConnection::getDatabase();
    QSqlQuery query(db);
    query.prepare("INSERT INTO users (username, password, is_admin) VALUES (:username, :password, :is_admin)");
    query.bindValue(":username", username);
    query.bindValue(":password", password);
    query.bindValue(":is_admin", isAdmin);
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