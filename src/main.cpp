#include <QCoreApplication>
#include <QApplication>
#include <QDebug>
#include <QJsonObject>
#include <cassert>
#include "database/DatabaseConnection.h"
#include "database/NodeRepository.h"
#include "database/RelationshipRepository.h"
#include "database/OntologyRepository.h"
#include "business/GraphEditor.h"
#include "business/QueryEngine.h"
#include "ui/mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    // 依然需要先配置并连接数据库
    DatabaseConfig config;
    config.hostname = "192.168.137.129"; // 你的虚拟机IP
    config.username = "root";
    config.password = "123456";
    config.database = "DatabaseKnowledgeGraph";

    if (DatabaseConnection::connect(config)) {
        MainWindow w;
        w.show();
        return a.exec();
    }

    return -1;
}
