#include <QCoreApplication> // 修改这里
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>

int main(int argc, char *argv[]) {
    // 1. 使用核心应用类，不需要图形界面
    QCoreApplication a(argc, argv);

    qDebug() << "=== 数据库驱动联调检查 ===";

    // 2. 检查 QMYSQL 驱动
    if (!QSqlDatabase::isDriverAvailable("QMYSQL")) {
        qDebug() << "❌ 错误：找不到 QMYSQL 驱动！";
        qDebug() << "可用驱动列表：" << QSqlDatabase::drivers();
    } else {
        qDebug() << "✅ 成功：QMYSQL 驱动已就绪！";

        // 3. 测试连接
        QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
        db.setHostName("127.0.0.1");
        db.setDatabaseName("mysql");
        db.setUserName("root");
        db.setPassword("123456"); // ⬅️ 确保这是你的密码

        if (db.open()) {
            qDebug() << "✅ 成功：已连接到 MySQL！";
            db.close();
        } else {
            qDebug() << "⚠️ 无法登录：" << db.lastError().text();
        }
    }

    // 直接退出，不进入事件循环
    return 0;
}