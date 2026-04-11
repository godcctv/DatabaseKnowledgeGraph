#ifndef USER_H
#define USER_H

#include <QString>

class User {
public:
    int id;
    QString username;
    QString password;
    bool isAdmin;
    bool canView;
    bool canEdit;
    bool canDelete;
    QString status;

    User() : id(-1), isAdmin(false) ,status("PENDING"){}
    bool isValid() const { return id > 0; }
};

#endif