#ifndef ADDEDGEDIALOG_H
#define ADDEDGEDIALOG_H

#include <QDialog>
#include "../model/GraphEdge.h"

namespace Ui {
class AddEdgeDialog;
}

class AddEdgeDialog : public QDialog {
    Q_OBJECT

public:
    explicit AddEdgeDialog(QWidget *parent = nullptr);
    ~AddEdgeDialog();

    // 设置显示的源节点和目标节点名称（只读，为了让用户确认）
    void setNodes(const QString& sourceName, const QString& targetName);

    // 获取用户输入的数据
    GraphEdge getEdgeData() const;

private slots:
    void on_btnOk_clicked();
    void on_btnCancel_clicked();

private:
    Ui::AddEdgeDialog *ui;
};

#endif // ADDEDGEDIALOG_H
