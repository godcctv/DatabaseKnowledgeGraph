#ifndef ADDNODEDIALOG_H
#define ADDNODEDIALOG_H

#include <QDialog>
#include "../model/GraphNode.h"

namespace Ui {
class AddNodeDialog;
}

class AddNodeDialog : public QDialog {
    Q_OBJECT

public:
    explicit AddNodeDialog(QWidget *parent = nullptr);
    ~AddNodeDialog();
    GraphNode getNodeData() const;

private slots:
    void on_btnOk_clicked();
    void on_btnCancel_clicked();

private:
    Ui::AddNodeDialog *ui;
};

#endif
