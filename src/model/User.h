#ifndef USER_H
#define USER_H

#include <QString>

class User {
public:
    int id;
    QString username;
    QString password;
    bool isAdmin;

    User() : id(-1), isAdmin(false) {}
    bool isValid() const { return id > 0; }
};

#endif