#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include "../business/GraphEditor.h" // 引入业务层
#include "VisualNode.h"
#include "VisualEdge.h"

class GraphEditor;
class ForceDirectedLayout;  // 必须加这行
class QTimer;               // 必须加这行

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void onActionDeleteRelationshipTriggered();

private slots:
    void updateStatusBar();
    void onActionAddNodeTriggered();
    void onNodeAdded(const GraphNode& node);

    void onActionDeleteTriggered();
    void onNodeDeleted(int nodeId);

    void onGraphChanged();

    void onActionAddRelationshipTriggered();
    void onRelationshipAdded(const GraphEdge& edge);
    // onActionDeleteRelationshipTriggered 已经移到上面 public 了
    void onRelationshipDeleted(int edgeId);

private:
    Ui::MainWindow *ui;
    GraphEditor *m_graphEditor;
    QGraphicsScene *m_scene;
    ForceDirectedLayout* m_layout;
    QTimer* m_timer;

    QGraphicsItem* findItemById(int nodeId);

    void loadInitialData();
    void setupConnections();
};
#endif // MAINWINDOW_H
