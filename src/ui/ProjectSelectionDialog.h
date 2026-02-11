#ifndef PROJECTSELECTIONDIALOG_H
#define PROJECTSELECTIONDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
// 移除了不必要的动画相关头文件

class ProjectSelectionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ProjectSelectionDialog(QWidget *parent = nullptr);
    int getSelectedOntologyId() const;
    QString getSelectedOntologyName() const;

private slots:
    void loadProjects();
    void onCreateProject();
    void onDeleteProject();
    void onOpenProject();
    void onItemDoubleClicked(QListWidgetItem* item);

    // 移除了动画槽函数

private:
    void setupUI();
    // 移除了 setupAnimations() 和 triggerShake()

    QListWidget* m_projectList;
    QPushButton* m_btnOpen;
    QPushButton* m_btnCreate;
    QPushButton* m_btnDelete;

    // 移除了动画相关组件 (overlay, timers, etc.)

    int m_selectedId;
    QString m_selectedName;
};

#endif // PROJECTSELECTIONDIALOG_H