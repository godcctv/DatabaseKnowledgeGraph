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