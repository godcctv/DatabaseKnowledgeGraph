#ifndef USERREPOSITORY_H
#define USERREPOSITORY_H

#include <QString>
#include "../model/User.h"

class UserRepository {
public:
    // 登录验证：成功返回有效 User 对象，失败返回无效对象
    static User login(const QString& username, const QString& password);
    static bool registerUser(const QString& username, const QString& password);
};

#endif // USERREPOSITORY_H