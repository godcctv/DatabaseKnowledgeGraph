#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
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
    void onActionAddNodeTriggered(); // 响应添加
    void onNodeAdded(const GraphNode& node);

    void onActionDeleteTriggered();       // 响应删除
    void onNodeDeleted(int nodeId);

    void onGraphChanged(); // 响应图数据变化

private:
    Ui::MainWindow *ui;
    GraphEditor *m_graphEditor;
    QGraphicsScene *m_scene;

    void setupConnections();
};
#endif
