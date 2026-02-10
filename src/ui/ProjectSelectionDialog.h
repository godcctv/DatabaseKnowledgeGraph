#ifndef PROJECTSELECTIONDIALOG_H
#define PROJECTSELECTIONDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QResizeEvent>

class ProjectSelectionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ProjectSelectionDialog(QWidget *parent = nullptr);

    // 获取用户最终选择的项目ID和名称
    int getSelectedOntologyId() const;
    QString getSelectedOntologyName() const;
protected:
    void resizeEvent(QResizeEvent *event) override;
private slots:
    void loadProjects(); // 加载项目列表
    void onCreateProject();
    void onDeleteProject();
    void onOpenProject();
    void onItemDoubleClicked(QListWidgetItem* item);

    // 动画槽函数
    void onShakeTimeout();
    void onFlickerTimeout();

private:
    void setupUI();
    void setupAnimations();
    void triggerShake(); // 屏幕震动

    QListWidget* m_projectList;
    QPushButton* m_btnOpen;
    QPushButton* m_btnCreate;
    QPushButton* m_btnDelete;

    // 动画相关组件
    QWidget* m_overlay;          // 黑色遮罩（烛光效果）
    QTimer* m_flickerTimer;      // 烛光定时器
    QTimer* m_shakeTimer;        // 震动定时器
    int m_shakeSteps;            // 震动剩余帧数
    QPoint m_originalPos;        // 震动前的位置

    int m_selectedId;
    QString m_selectedName;
};

#endif // PROJECTSELECTIONDIALOG_H