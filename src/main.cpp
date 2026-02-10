// src/main.cpp
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QMessageBox>
#include <QStyleFactory>
#include "database/DatabaseConnection.h"
#include "database/OntologyRepository.h"
#include "ui/mainwindow.h"
#include "ui/ProjectSelectionDialog.h"


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

    // 内置保底深色样式 (与 style.qss 内容一致)
    return R"(
        QWidget { background-color: #0E1019; color: #E0E6ED; font-family: "Microsoft YaHei"; font-size: 10pt; selection-background-color: #00E5FF; outline: none; }
        QMenuBar { background-color: #0B0D17; border-bottom: 1px solid #2A2F45; }
        QMenuBar::item { background: transparent; padding: 8px 16px; color: #A0AAB5; }
        QMenuBar::item:selected { background-color: rgba(255, 255, 255, 0.05); color: #FFFFFF; }
        QMenu { background-color: #161925; border: 1px solid #2A2F45; padding: 5px; }
        QMenu::item:selected { background-color: #00E5FF; color: #000000; border-radius: 2px; }
        QToolBar { background-color: #0E1019; border-bottom: 1px solid #2A2F45; spacing: 10px; padding: 5px; }
        QToolButton { background-color: transparent; border-radius: 4px; padding: 6px 12px; color: #E0E6ED; font-weight: bold; }
        QToolButton:hover { background-color: rgba(0, 229, 255, 0.1); border: 1px solid rgba(0, 229, 255, 0.3); color: #00E5FF; }
        QLineEdit { background-color: #08090F; border: 1px solid #2A2F45; border-radius: 14px; padding: 4px 12px; color: #00E5FF; }
        QTreeWidget { background-color: rgba(22, 25, 37, 0.9); border: none; border-left: 1px solid #2A2F45; }
        QHeaderView::section { background-color: #0E1019; color: #00E5FF; padding: 10px; border: none; border-bottom: 2px solid #2A2F45; font-weight: bold; }
        QTreeWidget::item { height: 36px; border-bottom: 1px solid rgba(42, 47, 69, 0.5); }
        QTreeWidget::item:selected { background-color: rgba(0, 229, 255, 0.15); color: #FFFFFF; border-left: 3px solid #00E5FF; }
        QScrollBar:vertical { border: none; background: #0E1019; width: 8px; margin: 0px; }
        QScrollBar::handle:vertical { background: #2A2F45; min-height: 20px; border-radius: 4px; }
        QScrollBar::handle:vertical:hover { background: #00E5FF; }
    )";
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
    // 2. 显示项目选择对话框 (这是新的启动流程)
    ProjectSelectionDialog selectDialog;
    if (selectDialog.exec() == QDialog::Accepted) {
        // 用户选好项目了，启动主界面
        int ontoId = selectDialog.getSelectedOntologyId();
        QString ontoName = selectDialog.getSelectedOntologyName();

        // 3. 启动主窗口，传入选中的项目ID
        MainWindow w(ontoId, ontoName);
        w.show();

        return a.exec();
    }

    // 如果用户取消或关闭了选择框，程序直接退出
    return 0;
}