// src/main.cpp
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QMessageBox>
#include <QStyleFactory>
#include <QRandomGenerator>
#include "database/DatabaseConnection.h"
#include "database/OntologyRepository.h"
#include "database/NodeRepository.h"         // 新增
#include "database/RelationshipRepository.h" // 新增

#include "ui/mainwindow.h"
#include "ui/ProjectSelectionDialog.h"
#include "ui/logindialog.h"
#include "model/User.h"
#include "ui/usermanagementdialog.h"

QString loadStyleSheet() {
    QStringList paths = {
        ":/style.qss",                          // 资源路径
        "src/ui/style.qss",                     // 源码路径
        "../DatabaseKnowledgeGraph/src/ui/style.qss", // 构建目录路径
        "style.qss"                             // 运行目录
    };

    for (const QString& path : paths) {
        QFile file(path);
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            qDebug() << "Success: Loaded style from" << path;
            QTextStream stream(&file);
            return stream.readAll();
        }
    }

    qWarning() << "Warning: style.qss not found! Using internal fallback style.";

    // 内置保底 Nord 极客风样式 (包含提示框 QMessageBox 样式)
    return R"(
        QWidget { background-color: #2E3440; color: #D8DEE9; font-family: "Microsoft YaHei", sans-serif; font-size: 10pt; outline: none; }
        QMenuBar { background-color: #2E3440; border-bottom: 1px solid #434C5E; }
        QMenuBar::item { background: transparent; padding: 8px 16px; color: #ECEFF4; }
        QMenuBar::item:selected { background-color: #3B4252; }
        QMenu { background-color: #3B4252; border: 1px solid #4C566A; padding: 4px; }
        QMenu::item { padding: 6px 24px; }
        QMenu::item:selected { background-color: #88C0D0; color: #2E3440; border-radius: 2px; }
        QToolBar { background-color: #2E3440; border-bottom: 1px solid #434C5E; spacing: 8px; padding: 5px; }
        QToolButton { background-color: transparent; border-radius: 4px; padding: 6px 12px; color: #D8DEE9; }
        QToolButton:hover { background-color: #3B4252; border: 1px solid #4C566A; }
        QToolButton:checked, QToolButton:pressed { background-color: #434C5E; color: #88C0D0; border: 1px solid #88C0D0; }
        QLineEdit, QComboBox, QTextEdit, QSpinBox { background-color: #3B4252; border: 1px solid #4C566A; border-radius: 4px; padding: 6px; color: #ECEFF4; }
        QLineEdit:focus, QComboBox:focus, QTextEdit:focus, QSpinBox:focus { border: 1px solid #88C0D0; }
        QTreeWidget { background-color: #2E3440; border: none; border-left: 1px solid #434C5E; }
        QHeaderView::section { background-color: #3B4252; color: #88C0D0; padding: 10px; border: none; border-bottom: 1px solid #4C566A; font-weight: bold;}
        QTreeWidget::item { height: 32px; border-bottom: 1px solid #3B4252; }
        QTreeWidget::item:hover { background-color: #3B4252; }
        QTreeWidget::item:selected { background-color: #434C5E; color: #88C0D0; border-left: 3px solid #88C0D0; }
        QScrollBar:vertical { border: none; background: #2E3440; width: 10px; margin: 0px; }
        QScrollBar::handle:vertical { background: #4C566A; min-height: 20px; border-radius: 5px; }
        QScrollBar::handle:vertical:hover { background: #88C0D0; }
        QPushButton { background-color: #4C566A; color: #ECEFF4; border: 1px solid #434C5E; padding: 6px 16px; border-radius: 4px; }
        QPushButton:hover { background-color: #5E81AC; border: 1px solid #81A1C1; }
        QPushButton:pressed { background-color: #81A1C1; }

        /* 彻底统一系统的提示框风格 */
        QMessageBox { background-color: #2E3440; border: 1px solid #4C566A; }
        QMessageBox QLabel { color: #D8DEE9; font-size: 13px; }
        QDialog { background-color: #2E3440; border: 1px solid #4C566A; }
    )";
}

void generateTestData(int ontologyId) {
    // 保护机制：如果当前图谱已经有超过 50 个节点，说明已经有数据了，直接跳过，防止重复插入
    if (NodeRepository::getAllNodes(ontologyId).size() > 50) {
        qDebug() << "检测到已存在大量数据，跳过测试数据生成。";
        return;
    }

    qDebug() << "开始生成测试数据...";
    QList<int> nodeIds;
    QStringList types = {"核心概念", "实体", "算法模型", "属性", "应用场景"};
    QStringList relTypes = {"包含", "依赖于", "属于", "优化了", "关联"};

    // 1. 生成 200 个随机节点
    for (int i = 0; i < 200; ++i) {
        GraphNode node;
        node.ontologyId = ontologyId;
        node.name = QString("TestNode_%1").arg(i + 1);
        node.nodeType = types[QRandomGenerator::global()->bounded(types.size())];
        node.description = "自动生成的压力测试节点数据";

        // 随机散布在画板中，给力导向算法提供初始分布
        node.posX = QRandomGenerator::global()->bounded(2000) - 1000;
        node.posY = QRandomGenerator::global()->bounded(1600) - 800;

        // 使用数据库层直接插入
        if (NodeRepository::addNode(node)) {
            nodeIds.append(node.id);
        }
    }

    // 2. 生成 400 条随机关系 (平均每个节点两条连线)
    int edgeCount = 0;
    for (int i = 0; i < 400; ++i) {
        if (nodeIds.size() < 2) break;

        int srcIdx = QRandomGenerator::global()->bounded(nodeIds.size());
        int dstIdx = QRandomGenerator::global()->bounded(nodeIds.size());

        // 避免自环（自己连自己）
        if (srcIdx == dstIdx) continue;

        GraphEdge edge;
        edge.ontologyId = ontologyId;
        edge.sourceId = nodeIds[srcIdx];
        edge.targetId = nodeIds[dstIdx];
        edge.relationType = relTypes[QRandomGenerator::global()->bounded(relTypes.size())];
        edge.weight = 1.0;

        if (RelationshipRepository::addRelationship(edge)) {
            edgeCount++;
        }
    }

    qDebug() << "========================================";
    qDebug() << "测试数据生成完毕！";
    qDebug() << "成功生成节点数：" << nodeIds.size();
    qDebug() << "成功生成关系数：" << edgeCount;
    qDebug() << "========================================";
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("Fusion"));
    a.setStyleSheet(loadStyleSheet());

    // 1. 连接数据库
    DatabaseConfig config;
    config.hostname = "localhost";
    config.username = "admin";
    config.password = "123456";
    config.database = "DatabaseKnowledgeGraph";

    if (!DatabaseConnection::connect(config)) {
        QMessageBox::critical(nullptr, "Error", "无法连接到数据库！");
        return -1;
    }
    OntologyRepository::initDatabase();
    LoginDialog loginDialog;
    if (loginDialog.exec() != QDialog::Accepted) {
        return 0; // 用户未登录，退出程序
    }

    User currentUser = loginDialog.getCurrentUser();
    if (currentUser.isAdmin) {
        // 【路线 A：管理员后台】
        UserManagementDialog adminPanel;
        adminPanel.show();       // 改为普通显示，不阻塞局部循环
        return a.exec();
    } else {
        // 【路线 B：普通用户工作台】
        ProjectSelectionDialog selectDialog;
        if (selectDialog.exec() == QDialog::Accepted) {
            // 权限校验：如果没有查看权限，不准进入工作台
            if (!currentUser.isAdmin && !currentUser.canView) {
                QMessageBox::critical(nullptr, "拒绝访问", "您没有查看图谱的权限，请联系管理员！");
                return 0;
            }

            int ontoId = selectDialog.getSelectedOntologyId();
            QString ontoName = selectDialog.getSelectedOntologyName();

            //generateTestData(ontoId);//用于测试
            
            // 把 currentUser 传给 MainWindow
            MainWindow w(ontoId, ontoName, currentUser);
            w.show();
            return a.exec();
        }
    }

    return 0;
}