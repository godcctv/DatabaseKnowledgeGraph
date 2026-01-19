#ifndef DATABASECONNECTION_H
#define DATABASECONNECTION_H

#include <QSqlDatabase>
#include <QString>
#include <memory>

/**
 * @brief 数据库配置结构体
 */
class DatabaseConfig {
public:
    QString hostname;
    QString username;
    QString password;
    QString database;
    int port;

    // 默认构造函数，设置默认MySQL端口
    DatabaseConfig() : port(3306) {}
};

/**
 * @brief 数据库连接管理类（单例模式）
 */
class DatabaseConnection {
public:
    /**
     * @brief 初始化并建立连接
     * @param config 数据库配置信息
     * @return 成功返回 true，失败返回 false
     */
    static bool connect(const DatabaseConfig& config);

    /**
     * @brief 断开数据库连接并释放资源
     */
    static void disconnect();

    /**
     * @brief 获取当前可用的数据库对象
     * @return QSqlDatabase 实例
     */
    static QSqlDatabase getDatabase();

    /**
     * @brief 检查当前是否处于连接状态
     */
    static bool isConnected();

private:
    // 使用 std::unique_ptr 确保在程序退出时自动清理
    static std::unique_ptr<QSqlDatabase> instance;

    // 禁止外部实例化
    DatabaseConnection() = default;
};

#endif // DATABASECONNECTION_H