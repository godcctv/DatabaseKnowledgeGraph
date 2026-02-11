#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QEvent>
#include <QPointF>
#include "../business/GraphEditor.h" // 引入业务层


class GraphEditor;
class ForceDirectedLayout;
class QTimer;
class VisualNode;
class VisualEdge;
class QGraphicsItem;
class QueryEngine;
class OntologyDock;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(int ontologyId, QString ontologyName, QWidget *parent = nullptr);
    ~MainWindow();
    // 提供给外部添加图元的接口
    void addNodeToScene(VisualNode* node);
    void addEdgeToScene(VisualEdge* edge);

    void onActionDeleteRelationshipTriggered();
    void onActionDeleteTriggered();
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void updateStatusBar();
    void onActionAddNodeTriggered();
    void onNodeAdded(const GraphNode& node);


    void onNodeDeleted(int nodeId);

    void onGraphChanged();

    void onActionAddRelationshipTriggered();
    void onRelationshipAdded(const GraphEdge& edge);
    // onActionDeleteRelationshipTriggered 已经移到上面 public 了
    void onRelationshipDeleted(int edgeId);

    //查询功能
    void onQueryFullGraph();  // 全图查询
    void onQuerySingleNode(); // 单节点查询 (右键或工具栏触发)
    void onQueryAttribute();  // 属性查询
    void onQueryPath();       // 路径查询

    void onTogglePropertyPanel();
    void onSwitchOntology(int ontologyId, QString name);
    void onGraphicsViewContextMenu(const QPoint &pos);
private:
    Ui::MainWindow *ui;
    GraphEditor *m_graphEditor;
    QGraphicsScene *m_scene;
    ForceDirectedLayout* m_layout;
    QTimer* m_timer;
    QueryEngine* m_queryEngine;
    bool m_hasClickPos = false;
    QPointF m_clickPos;
    QGraphicsItem* findItemById(int nodeId);
    int m_currentOntologyId;

    void loadInitialData();
    void setupConnections();
    void setupToolbar();
    void drawNode(int id, QString name, QString type, double x, double y);
    void drawEdge(const GraphEdge& edge);
};
#endif // MAINWINDOW_H
