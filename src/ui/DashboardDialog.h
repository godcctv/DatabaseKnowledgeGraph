#ifndef DASHBOARDDIALOG_H
#define DASHBOARDDIALOG_H

#include <QDialog>

class QueryEngine;
class QTableWidget;
class QLabel;

class DashboardDialog : public QDialog {
    Q_OBJECT

public:
    explicit DashboardDialog(int ontologyId, QueryEngine* engine, QWidget *parent = nullptr);
    ~DashboardDialog();

private:
    void setupUI();
    void loadData();

    int m_ontologyId;
    QueryEngine* m_engine;

    // UI 组件
    QLabel* m_lblTotalNodes;
    QLabel* m_lblTotalEdges;
    QTableWidget* m_nodeTable;
    QTableWidget* m_edgeTable;
};

#endif // DASHBOARDDIALOG_H