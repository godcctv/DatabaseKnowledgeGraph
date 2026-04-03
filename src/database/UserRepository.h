#ifndef USERREPOSITORY_H
#define USERREPOSITORY_H

#include <QString>
#include "../model/User.h"

class UserRepository {
public:
    // 登录验证：成功返回有效 User 对象，失败返回无效对象
    static User login(const QString& username, const QString& password);
    static bool registerUser(const QString& username, const QString& password);

    static QList<User> getAllUsers();
    static bool addUser(const QString& username, const QString& password, bool isAdmin);
    static bool deleteUser(int id);
    static bool resetPassword(int id, const QString& newPassword);
    // 在 public 下新增一个方法声明
    static bool updatePermissions(int id, bool canView, bool canEdit, bool canDelete);
};

#endif // USERREPOSITORY_H