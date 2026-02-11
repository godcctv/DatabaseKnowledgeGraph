#include "ProjectSelectionDialog.h"
#include "../database/OntologyRepository.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QDebug>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation> // 保留用于按钮呼吸效果

ProjectSelectionDialog::ProjectSelectionDialog(QWidget *parent)
    : QDialog(parent), m_selectedId(-1)
{
    // 1. 基础窗口设置
    setWindowTitle("选择知识库项目");
    resize(800, 500);

    // 2. 初始化 UI 和 样式
    setupUI();

    // 3. 加载数据
    loadProjects();
}

void ProjectSelectionDialog::setupUI() {
    // 核心配色：深空蓝背景，亮蓝/青色高亮，白色/浅蓝文本
    setStyleSheet(R"(
        QDialog {
            background-color: #050a14; /* 深空蓝底色 */
            /* 如果有星空背景图，可以取消下面注释并替换路径 */
            /* background-image: url(:/images/star_bg.jpg); */
            background-position: center;
            color: #d0e6ff; /* 浅蓝文本 */
            font-family: "Microsoft YaHei", "Segoe UI", sans-serif; /* 现代无衬线字体 */
        }

        /* 标题 */
        QLabel#TitleLabel {
            font-size: 26px;
            font-weight: bold;
            color: #58a6ff; /* 亮蓝色标题 */
            margin-bottom: 25px;
            /* 渐变分割线，营造科幻感 */
            border-bottom: 2px solid qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0, stop:0 rgba(88, 166, 255, 0), stop:0.5 rgba(88, 166, 255, 255), stop:1 rgba(88, 166, 255, 0));
            letter-spacing: 2px;
            padding-bottom: 10px;
        }

        /* 列表容器 */
        QListWidget {
            background-color: rgba(16, 24, 40, 0.8); /* 半透明深蓝 */
            border: 1px solid #1e3a5a; /* 深蓝边框 */
            border-radius: 8px;
            color: #d0e6ff;
            font-size: 15px;
            padding: 8px;
            outline: none; /* 去掉虚线框 */
        }

        /* 列表项 */
        QListWidget::item {
            height: 50px;
            margin: 3px 0;
            background-color: rgba(255, 255, 255, 0.03); /* 极淡的背景 */
            border: 1px solid transparent;
            border-radius: 6px;
            padding-left: 15px;
        }

        /* 列表项悬浮 */
        QListWidget::item:hover {
            background-color: rgba(88, 166, 255, 0.1);
            border-color: rgba(88, 166, 255, 0.5);
        }

        /* 列表项选中：亮蓝高亮 */
        QListWidget::item:selected {
            background-color: rgba(88, 166, 255, 0.2);
            border: 1px solid #58a6ff;
            color: #ffffff;
        }

        /* 按钮通用样式 */
        QPushButton {
            background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1e3a5a, stop:1 #101828); /* 深蓝渐变 */
            color: #d0e6ff;
            border: 1px solid #3a6ea5;
            padding: 10px 24px;
            font-size: 15px;
            font-weight: 600;
            border-radius: 6px;
        }
        QPushButton:hover {
            background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #3a6ea5, stop:1 #1e3a5a);
            border-color: #58a6ff;
            color: #ffffff;
        }
        QPushButton:pressed {
            background-color: #0f172a;
            border-color: #2a4e75;
        }

        /* 主要按钮（打开）：亮蓝底色 */
        QPushButton#BtnPrimary {
            background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #0066cc, stop:1 #003366);
            border: 1px solid #0077ff;
            color: #ffffff;
        }
        QPushButton#BtnPrimary:hover {
            background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #0077ff, stop:1 #004488);
        }

        /* 危险按钮（删除）：警告红 */
        QPushButton#BtnDanger {
            color: #ff6b6b;
            background-color: transparent;
            border: 1px solid #ff6b6b;
        }
        QPushButton#BtnDanger:hover {
            color: #ff9999;
            border-color: #ff9999;
            background-color: rgba(255, 107, 107, 0.1);
        }
    )");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(25);
    mainLayout->setContentsMargins(50, 50, 50, 50);

    // 标题：知识图谱项目
    QLabel* title = new QLabel("知 识 图 谱 项 目", this);
    title->setObjectName("TitleLabel");
    title->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(title);

    // 项目列表
    m_projectList = new QListWidget(this);
    // 给列表加一个蓝色的发光阴影
    QGraphicsDropShadowEffect* listShadow = new QGraphicsDropShadowEffect();
    listShadow->setBlurRadius(20);
    listShadow->setColor(QColor(88, 166, 255, 50)); // 半透明亮蓝
    listShadow->setOffset(0, 4);
    m_projectList->setGraphicsEffect(listShadow);
    mainLayout->addWidget(m_projectList);

    // 按钮区域
    QHBoxLayout* btnLayout = new QHBoxLayout();

    m_btnCreate = new QPushButton("新建项目", this);
    m_btnCreate->setToolTip("创建一个新的知识图谱项目");

    m_btnDelete = new QPushButton("删除项目", this);
    m_btnDelete->setObjectName("BtnDanger");

    m_btnOpen = new QPushButton("打开项目", this);
    m_btnOpen->setObjectName("BtnPrimary");
    m_btnOpen->setDefault(true);

    // 给主按钮加一个蓝色的呼吸光晕
    QGraphicsDropShadowEffect* btnShadow = new QGraphicsDropShadowEffect(m_btnOpen);
    btnShadow->setBlurRadius(20);
    btnShadow->setColor(QColor(0, 119, 255, 150)); // 亮蓝光
    btnShadow->setOffset(0, 0);
    m_btnOpen->setGraphicsEffect(btnShadow);

    // 呼吸动画 (保留一个简单的呼吸效果，增加科技感)
    QPropertyAnimation* pulseAnim = new QPropertyAnimation(btnShadow, "blurRadius", this);
    pulseAnim->setDuration(1500);
    pulseAnim->setStartValue(10);
    pulseAnim->setEndValue(30);
    pulseAnim->setEasingCurve(QEasingCurve::InOutSine); // 更平滑的曲线
    pulseAnim->setLoopCount(-1);
    pulseAnim->start();

    btnLayout->addWidget(m_btnCreate);
    btnLayout->addWidget(m_btnDelete);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnOpen);

    mainLayout->addLayout(btnLayout);

    // 连接信号槽
    connect(m_btnCreate, &QPushButton::clicked, this, &ProjectSelectionDialog::onCreateProject);
    connect(m_btnDelete, &QPushButton::clicked, this, &ProjectSelectionDialog::onDeleteProject);
    connect(m_btnOpen, &QPushButton::clicked, this, &ProjectSelectionDialog::onOpenProject);
    connect(m_projectList, &QListWidget::itemDoubleClicked, this, &ProjectSelectionDialog::onItemDoubleClicked);
}

// --- 业务逻辑 ---

void ProjectSelectionDialog::loadProjects() {
    m_projectList->clear();
    QList<Ontology> ontologies = OntologyRepository::getAllOntologies();

    for (const auto& onto : ontologies) {
        QListWidgetItem* item = new QListWidgetItem(m_projectList);
        // 使用更符合科幻主题的 Unicode 图标
        item->setText(QString("🌌 %1  [v%2]").arg(onto.name).arg(onto.version));
        item->setData(Qt::UserRole, onto.id);
        item->setData(Qt::UserRole + 1, onto.name);
    }
}

void ProjectSelectionDialog::onCreateProject() {
    // 文案更新为更专业的说法
    QString name = QInputDialog::getText(this, "新建项目", "请输入项目名称：");
    if (name.trimmed().isEmpty()) return;

    QString desc = QInputDialog::getText(this, "描述", "项目描述（可选）：");

    if (OntologyRepository::addOntology(name, desc)) {
        loadProjects();
    } else {
        // 移除了 triggerShake()
        QMessageBox::warning(this, "创建失败", "无法创建项目。\n该名称可能已存在。");
    }
}

void ProjectSelectionDialog::onDeleteProject() {
    QListWidgetItem* item = m_projectList->currentItem();
    if (!item) return;

    int id = item->data(Qt::UserRole).toInt();
    QString name = item->data(Qt::UserRole + 1).toString();

    // 文案更新
    auto reply = QMessageBox::question(this, "确认删除",
        QString("确定要删除项目 [%1] 吗？\n此操作无法撤销。").arg(name),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // 移除了 triggerShake()
        OntologyRepository::deleteOntology(id);
        loadProjects();
    }
}

void ProjectSelectionDialog::onOpenProject() {
    QListWidgetItem* item = m_projectList->currentItem();
    if (!item) {
        // 移除了 triggerShake()
        QMessageBox::warning(this, "提示", "请先选择一个项目。");
        return;
    }

    m_selectedId = item->data(Qt::UserRole).toInt();
    m_selectedName = item->data(Qt::UserRole + 1).toString();
    accept();
}

void ProjectSelectionDialog::onItemDoubleClicked(QListWidgetItem* item) {
    Q_UNUSED(item);
    onOpenProject();
}

int ProjectSelectionDialog::getSelectedOntologyId() const {
    return m_selectedId;
}

QString ProjectSelectionDialog::getSelectedOntologyName() const {
    return m_selectedName;
}