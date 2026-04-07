#ifndef DASHBOARDDIALOG_H
#define DASHBOARDDIALOG_H

#include <QDialog>
#include <QMap>
#include <QWidget>

class QueryEngine;
class QTableWidget;
class QLabel;

// --- 自定义轻量级原生环形图控件 ---
class SimpleRingChart : public QWidget {
    Q_OBJECT
public:
    explicit SimpleRingChart(QWidget *parent = nullptr);
    void setData(const QMap<QString, int>& data);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QMap<QString, int> m_data;
    QStringList m_colors; // Nord 主题色板
};

// --- 仪表盘主窗口 ---
class DashboardDialog : public QDialog {
    Q_OBJECT

public:
    explicit DashboardDialog(int ontologyId, QueryEngine* engine, QWidget *parent = nullptr);
    ~DashboardDialog();

private:
    void setupUI();
    void loadData();
    QWidget* createStatCard(const QString& title, QLabel*& valueLabel, const QString& icon);

    int m_ontologyId;
    QueryEngine* m_engine;

    // UI 组件
    QLabel* m_lblTotalNodes;
    QLabel* m_lblTotalEdges;
    QLabel* m_lblCoreNode;   // 新增：核心节点展示

    SimpleRingChart* m_nodeChart; // 节点环形图
    SimpleRingChart* m_edgeChart; // 关系环形图

    QTableWidget* m_nodeTable;
    QTableWidget* m_edgeTable;
};

#endif // DASHBOARDDIALOG_H