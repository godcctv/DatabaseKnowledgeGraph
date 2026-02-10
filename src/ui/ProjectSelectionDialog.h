#ifndef PROJECTSELECTIONDIALOG_H
#define PROJECTSELECTIONDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>

class ProjectSelectionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ProjectSelectionDialog(QWidget *parent = nullptr);

    // 获取用户最终选择的项目ID和名称
    int getSelectedOntologyId() const;
    QString getSelectedOntologyName() const;

private slots:
    void loadProjects(); // 加载项目列表
    void onCreateProject();
    void onDeleteProject();
    void onOpenProject();
    void onItemDoubleClicked(QListWidgetItem* item);

private:
    QListWidget* m_projectList;
    QPushButton* m_btnOpen;
    QPushButton* m_btnCreate;
    QPushButton* m_btnDelete;

    int m_selectedId;
    QString m_selectedName;
};

#endif // PROJECTSELECTIONDIALOG_H