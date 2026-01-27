#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "../database/DatabaseConnection.h" // 检查连接状态

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_graphEditor = new GraphEditor(this);

    // 初始化显示数据库状态
    updateStatusBar();
}

void MainWindow::updateStatusBar() {
    if (DatabaseConnection::isConnected()) {
        ui->statusbar->showMessage("数据库已连接");
    } else {
        ui->statusbar->showMessage("数据库未连接", 0);
    }
}

MainWindow::~MainWindow() {
    delete ui;
}
