#include "ProjectSelectionDialog.h"
#include "../database/OntologyRepository.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QDebug>
#include <QRandomGenerator>
#include <QGraphicsDropShadowEffect>

ProjectSelectionDialog::ProjectSelectionDialog(QWidget *parent)
    : QDialog(parent), m_selectedId(-1), m_shakeSteps(0)
{
    // 1. 基础窗口设置
    setWindowTitle("选择知识库项目");
    resize(800, 500);

    // 2. 初始化 UI 和 样式
    setupUI();

    // 3. 初始化动画特效
    setupAnimations();

    // 4. 加载数据
    loadProjects();
}

void ProjectSelectionDialog::setupUI() {

    setStyleSheet(R"(
        QDialog {
            background-color: #0B0B0B;
            color: #E8DCCA;
            font-family: "FangSong", "SimSun", "Times New Roman", serif;
        }

        /* 标题 */
        QLabel#TitleLabel {
            font-size: 32px;
            font-weight: bold;
            color: #C8A56E;
            margin-bottom: 15px;
            border-bottom: 2px solid #555;
            letter-spacing: 5px; /* 增加字间距，更显庄重 */
        }

        /* 列表容器：模拟石板或重金属边框 */
        QListWidget {
            background-color: #121212;
            border: 2px solid #3E3E3E;
            border-radius: 2px;
            color: #E8DCCA;
            font-size: 18px; /* 中文可以稍微大一点 */
            padding: 5px;
            outline: none; /* 去掉虚线框 */
        }

        /* 列表项：模拟羊皮纸条 */
        QListWidget::item {
            height: 55px;
            margin-bottom: 4px;
            background-color: #1E1E1E;
            border: 1px solid #333;
            padding-left: 10px;
        }

        /* 列表项选中：血红高亮 */
        QListWidget::item:selected {
            background-color: #2A0505;
            border: 1px solid #AF1E1E;
            color: #FF4444;
        }
        QListWidget::item:hover {
            background-color: #252525;
            border-color: #C8A56E;
        }

        /* 按钮：模拟金属铭牌 */
        QPushButton {
            background-color: #1A1A1A;
            color: #C8A56E;
            border: 1px solid #444;
            padding: 10px 24px;
            font-size: 16px;
            font-weight: bold;
            border-radius: 1px;
            font-family: "Microsoft YaHei", "SimHei"; /* 按钮用黑体更清晰 */
        }
        QPushButton:hover {
            background-color: #2A2A2A;
            border-color: #C8A56E;
            color: #FFEEDD;
        }
        QPushButton:pressed {
            background-color: #000000;
            border-color: #886633;
        }

        /* 主要按钮（打开）：暗红底色 */
        QPushButton#BtnPrimary {
            background-color: #3E0E0E;
            border: 1px solid #AF1E1E;
            color: #FFaaaa;
        }
        QPushButton#BtnPrimary:hover {
            background-color: #5E1515;
            color: #FF4444;
            border-color: #FF0000;
        }

        /* 危险按钮（删除）：幽灵灰 */
        QPushButton#BtnDanger {
            color: #888;
            background-color: transparent;
            border: 1px dashed #444;
        }
        QPushButton#BtnDanger:hover {
            color: #FF6B6B;
            border-color: #FF6B6B;
            background-color: rgba(255, 0, 0, 0.05);
        }
    )");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(40, 40, 40, 40);

    // 标题：暗黑知识库
    QLabel* title = new QLabel("暗 黑 知 识 库", this);
    title->setObjectName("TitleLabel");
    title->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(title);

    // 项目列表
    m_projectList = new QListWidget(this);
    // 给列表加一个阴影，增加立体感
    QGraphicsDropShadowEffect* listShadow = new QGraphicsDropShadowEffect();
    listShadow->setBlurRadius(15);
    listShadow->setColor(QColor(0, 0, 0, 150));
    listShadow->setOffset(0, 4);
    m_projectList->setGraphicsEffect(listShadow);
    mainLayout->addWidget(m_projectList);

    // 按钮区域
    QHBoxLayout* btnLayout = new QHBoxLayout();

    m_btnCreate = new QPushButton("新 建 征 途", this);
    m_btnCreate->setToolTip("创建一个新的知识图谱项目");

    m_btnDelete = new QPushButton("遗 弃", this);
    m_btnDelete->setObjectName("BtnDanger");

    m_btnOpen = new QPushButton("启 程", this);
    m_btnOpen->setObjectName("BtnPrimary");
    m_btnOpen->setDefault(true);

    // 给主按钮加一个“血色呼吸”光晕
    QGraphicsDropShadowEffect* btnShadow = new QGraphicsDropShadowEffect(m_btnOpen);
    btnShadow->setBlurRadius(20);
    btnShadow->setColor(QColor(175, 30, 30, 100)); // 半透明红光
    btnShadow->setOffset(0, 0);
    m_btnOpen->setGraphicsEffect(btnShadow);

    // 呼吸动画
    QPropertyAnimation* pulseAnim = new QPropertyAnimation(btnShadow, "blurRadius", this);
    pulseAnim->setDuration(2000);
    pulseAnim->setStartValue(10);
    pulseAnim->setEndValue(30);
    pulseAnim->setEasingCurve(QEasingCurve::SineCurve);
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

void ProjectSelectionDialog::setupAnimations() {
    // --- 1. 烛光闪烁特效 (Global Flickering) ---
    m_overlay = new QWidget(this);
    m_overlay->setStyleSheet("background-color: black;");
    m_overlay->setAttribute(Qt::WA_TransparentForMouseEvents); // 鼠标穿透
    m_overlay->resize(this->size());
    m_overlay->show();
    m_overlay->raise(); // 放在最顶层

    // 透明度特效
    QGraphicsOpacityEffect* opacityEffect = new QGraphicsOpacityEffect(m_overlay);
    opacityEffect->setOpacity(0.15); // 初始暗度
    m_overlay->setGraphicsEffect(opacityEffect);

    // 定时器控制闪烁
    m_flickerTimer = new QTimer(this);
    connect(m_flickerTimer, &QTimer::timeout, this, &ProjectSelectionDialog::onFlickerTimeout);
    m_flickerTimer->start(100); // 10fps

    // --- 2. 震动特效初始化 ---
    m_shakeTimer = new QTimer(this);
    connect(m_shakeTimer, &QTimer::timeout, this, &ProjectSelectionDialog::onShakeTimeout);
}

void ProjectSelectionDialog::onFlickerTimeout() {
    // 获取特效对象
    QGraphicsOpacityEffect* effect = qobject_cast<QGraphicsOpacityEffect*>(m_overlay->graphicsEffect());
    if (!effect) return;

    // 模拟火把光照的不稳定性
    double baseOpacity = 0.10; // 基础遮罩浓度 (越小越亮)

    // 随机抖动: -0.02 ~ +0.05
    double flicker = (QRandomGenerator::global()->bounded(70) - 20) / 1000.0;

    // 偶尔剧烈变暗 (模拟阴影掠过)
    if (QRandomGenerator::global()->bounded(100) > 95) {
        flicker += 0.15;
    }

    effect->setOpacity(baseOpacity + flicker);
}

void ProjectSelectionDialog::triggerShake() {
    m_originalPos = this->pos(); // 记录当前位置
    m_shakeSteps = 10;           // 震动 10 次
    m_shakeTimer->start(30);     // 30ms 一次
}

void ProjectSelectionDialog::onShakeTimeout() {
    if (m_shakeSteps > 0) {
        // 随机偏移 (-5 ~ 5)
        int dx = (QRandomGenerator::global()->bounded(10)) - 5;
        int dy = (QRandomGenerator::global()->bounded(10)) - 5;

        // 随时间衰减幅度
        dx = dx * (m_shakeSteps / 10.0);
        dy = dy * (m_shakeSteps / 10.0);

        this->move(m_originalPos.x() + dx, m_originalPos.y() + dy);
        m_shakeSteps--;
    } else {
        this->move(m_originalPos); // 归位
        m_shakeTimer->stop();
    }
}

// 确保遮罩层跟随窗口大小变化
void ProjectSelectionDialog::resizeEvent(QResizeEvent *event) {
    if (m_overlay) {
        m_overlay->resize(this->size());
    }
    QDialog::resizeEvent(event);
}

// --- 业务逻辑 ---

void ProjectSelectionDialog::loadProjects() {
    m_projectList->clear();
    QList<Ontology> ontologies = OntologyRepository::getAllOntologies();

    for (const auto& onto : ontologies) {
        QListWidgetItem* item = new QListWidgetItem(m_projectList);
        // 使用 Unicode 符号增强风格
        item->setText(QString("❖ %1  [v%2]").arg(onto.name).arg(onto.version));
        item->setData(Qt::UserRole, onto.id);
        item->setData(Qt::UserRole + 1, onto.name);
    }
}

void ProjectSelectionDialog::onCreateProject() {
    QString name = QInputDialog::getText(this, "新建征途", "请输入项目名称：");
    if (name.trimmed().isEmpty()) return;

    QString desc = QInputDialog::getText(this, "描述", "项目描述（可选）：");

    if (OntologyRepository::addOntology(name, desc)) {
        loadProjects();
    } else {
        triggerShake(); // 失败时震动！
        QMessageBox::warning(this, "创建失败", "无法建立新的征途。\n该名称可能已存在于古老的记录中。");
    }
}

void ProjectSelectionDialog::onDeleteProject() {
    QListWidgetItem* item = m_projectList->currentItem();
    if (!item) return;

    int id = item->data(Qt::UserRole).toInt();
    QString name = item->data(Qt::UserRole + 1).toString();

    auto reply = QMessageBox::question(this, "放弃希望？",
        QString("你确定要永久遗弃 [%1] 吗？\n此操作产生的后果无法挽回。").arg(name),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        triggerShake(); // 删除时剧烈震动！
        OntologyRepository::deleteOntology(id);
        loadProjects();
    }
}

void ProjectSelectionDialog::onOpenProject() {
    QListWidgetItem* item = m_projectList->currentItem();
    if (!item) {
        // 未选中时轻微震动提示
        triggerShake();
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