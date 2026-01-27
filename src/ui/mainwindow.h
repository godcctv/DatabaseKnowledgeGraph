#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "../business/GraphEditor.h" // 引入业务层

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateStatusBar(); // 用于显示数据库状态

private:
    Ui::MainWindow *ui;
    GraphEditor *m_graphEditor; // 关联业务层
};
#endif
