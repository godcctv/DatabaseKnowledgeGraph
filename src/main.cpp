#include <QCoreApplication>
#include <QDebug>
#include "database/DatabaseConnection.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // 1. 初始化配置信息
    DatabaseConfig config;
    config.hostname = "192.168.137.129";
    config.username = "root";
    config.password = "123456";
    config.database = "DatabaseKnowledgeGraph";
    config.port = 3306;

    // 2. 尝试建立连接
    qDebug() << "正在尝试连接数据库...";
    if (DatabaseConnection::connect(config)) {
        qDebug() << "-----------------------------------------";
        qDebug() << "测试成功: 数据库已就绪！";

        // 验证连接对象是否真的可用
        QSqlDatabase db = DatabaseConnection::getDatabase();
        if (db.isOpen()) {
            qDebug() << "确认状态: 连接名 [KG_CONN] 处于开启状态。";
            qDebug() << "当前驱动:" << db.driverName();
        }
        qDebug() << "-----------------------------------------";
    } else {
        qDebug() << "测试失败: 请检查网络、防火墙或 MySQL 服务状态。";
    }

    // 毕设初期阶段，我们可以执行完测试直接退出，或者保持事件循环
    // return a.exec();
    return 0;
}