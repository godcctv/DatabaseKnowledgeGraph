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
            int ontoId = selectDialog.getSelectedOntologyId();
            QString ontoName = selectDialog.getSelectedOntologyName();

            MainWindow w(ontoId, ontoName);
            w.show();
            return a.exec();
        }
    }

    return 0;
}