// src/ui/PathQueryDialog.h
#ifndef PATHQUERYDIALOG_H
#define PATHQUERYDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>

class PathQueryDialog : public QDialog {
    Q_OBJECT
public:
    explicit PathQueryDialog(QWidget* parent = nullptr) : QDialog(parent) {
        setWindowTitle("多跳路径探索");
        resize(300, 400);
        QVBoxLayout* layout = new QVBoxLayout(this);

        layout->addWidget(new QLabel("发现的路径 (点击高亮查看):", this));
        pathListWidget = new QListWidget(this);
        layout->addWidget(pathListWidget);

        // Nord 主题样式
        this->setStyleSheet(R"(
            QDialog { background-color: #2E3440; border: 1px solid #4C566A; }
            QLabel { color: #D8DEE9; font-weight: bold; font-size: 13px; }
            QListWidget { background-color: #3B4252; border: 1px solid #4C566A; color: #ECEFF4; outline: none; padding: 5px; }
            QListWidget::item { padding: 8px; border-bottom: 1px solid #434C5E; }
            QListWidget::item:selected { background-color: #81A1C1; color: #2E3440; font-weight: bold; }
        )");

        connect(pathListWidget, &QListWidget::currentRowChanged, this, &PathQueryDialog::onPathSelected);
    }

    void setPaths(const QList<QList<int>>& paths) {
        m_paths = paths;
        pathListWidget->clear();
        if (paths.isEmpty()) {
            pathListWidget->addItem("未找到连通路径 (当前限制最大5跳)");
        } else {
            for (int i = 0; i < paths.size(); ++i) {
                pathListWidget->addItem(QString("路径 %1 (%2 跳)").arg(i + 1).arg(paths[i].size() - 1));
            }
            pathListWidget->setCurrentRow(0); // 默认选中第一条并触发高亮
        }
    }

signals:
    void pathSelected(const QList<int>& pathNodeIds);
    void dialogClosed();

protected:
    void closeEvent(QCloseEvent *event) override {
        emit dialogClosed();
        QDialog::closeEvent(event);
    }

private slots:
    void onPathSelected(int row) {
        if (row >= 0 && row < m_paths.size()) {
            emit pathSelected(m_paths[row]);
        }
    }

private:
    QListWidget* pathListWidget;
    QList<QList<int>> m_paths;
};

#endif // PATHQUERYDIALOG_H