// src/main.cpp
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QStyleFactory>
#include "database/DatabaseConnection.h"
#include "ui/mainwindow.h"

// 辅助函数：智能加载样式表
// 优先加载文件，加载失败则使用内置字符串，确保界面永远是深色
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
        QWidget { background-color: #1e1e1e; color: #cccccc; font-family: "Microsoft YaHei"; font-size: 10pt; }
        QMainWindow::separator { background-color: #2d2d2d; width: 1px; }
        QMenuBar { background-color: #1e1e1e; border-bottom: 1px solid #333333; }
        QMenuBar::item { background: transparent; padding: 8px 12px; }
        QMenuBar::item:selected { background-color: #333333; }
        QMenu { background-color: #252526; border: 1px solid #454545; }
        QMenu::item:selected { background-color: #094771; }
        QSplitter::handle { background-color: #2d2d2d; }
        QTreeWidget { background-color: #252526; border: none; border-left: 1px solid #333333; alternate-background-color: #2a2a2d; }
        QTreeWidget::item { height: 32px; padding-left: 5px; }
        QTreeWidget::item:selected { background-color: #37373d; color: #ffffff; border-left: 3px solid #007acc; }
        QHeaderView::section { background-color: #252526; color: #858585; padding: 8px; border: none; border-bottom: 1px solid #333333; font-weight: bold; }
        QScrollBar:vertical { border: none; background: #252526; width: 10px; margin: 0px; }
        QScrollBar::handle:vertical { background: #424242; min-height: 20px; border-radius: 5px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
    )";
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    // 1. 关键：强制使用 Fusion 风格，解决 Linux/Windows 默认白色底色问题
    a.setStyle(QStyleFactory::create("Fusion"));

    // 2. 加载样式表
    a.setStyleSheet(loadStyleSheet());

    // 3. 连接数据库
    DatabaseConfig config;
    config.hostname = "localhost";
    config.username = "admin";
    config.password = "123456";
    config.database = "DatabaseKnowledgeGraph";

    if (DatabaseConnection::connect(config)) {
        MainWindow w;
        w.show();
        return a.exec();
    }

    return -1;
}