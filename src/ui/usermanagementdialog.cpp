#include "usermanagementdialog.h"
#include "../database/UserRepository.h"
#include "ProjectSelectionDialog.h" // 引入项目选择框
#include "mainwindow.h"             // 引入主图形界面
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QInputDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QStackedWidget>
#include <QFrame>
#include <QCheckBox>

UserManagementDialog::UserManagementDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("知识图谱系统 - 管理员仪表盘 (Admin Dashboard)");
    resize(900, 600); // 调大窗口以适应侧边栏

    // 主布局 (水平)
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ================= 1. 左侧导航侧边栏 =================
    QFrame *sidebar = new QFrame(this);
    sidebar->setObjectName("Sidebar");
    sidebar->setFixedWidth(200);
    QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(10, 30, 10, 20);
    sidebarLayout->setSpacing(10);

    QLabel *logoLabel = new QLabel("🚀 管理员控制台", sidebar);
    logoLabel->setStyleSheet("color: #88C0D0; font-size: 18px; font-weight: bold; margin-bottom: 20px;");
    logoLabel->setAlignment(Qt::AlignCenter);
    sidebarLayout->addWidget(logoLabel);

    QPushButton *btnUserMgmt = new QPushButton("👥 后台用户管理", sidebar);
    btnUserMgmt->setObjectName("SidebarBtnActive"); // 默认处于选中状态

    QPushButton *btnWorkspace = new QPushButton("🌌 进入图谱工作台", sidebar);
    btnWorkspace->setObjectName("SidebarBtn");

    QPushButton *btnExit = new QPushButton("🚪 退出系统", sidebar);
    btnExit->setObjectName("SidebarBtnExit");

    sidebarLayout->addWidget(btnUserMgmt);
    sidebarLayout->addWidget(btnWorkspace);
    sidebarLayout->addStretch();
    sidebarLayout->addWidget(btnExit);

    // ================= 2. 右侧内容区 =================
    // 使用 StackedWidget 方便你以后扩展更多后台页面（如系统日志、权限分配）
    QStackedWidget *stackedWidget = new QStackedWidget(this);

    // --- 页面 A: 用户管理面板 ---
    QWidget *userMgmtPage = new QWidget();
    QVBoxLayout *pageLayout = new QVBoxLayout(userMgmtPage);
    pageLayout->setContentsMargins(30, 30, 30, 30);
    pageLayout->setSpacing(20);

    QLabel* titleLabel = new QLabel("系 统 用 户 管 理", userMgmtPage);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #88C0D0; font-size: 24px; font-weight: bold; letter-spacing: 2px; border-bottom: 1px solid #4C566A; padding-bottom: 15px;");
    pageLayout->addWidget(titleLabel);

    m_table = new QTableWidget(userMgmtPage);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({"用户 ID", "用户名", "角色权限", "密码","当前权限"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    pageLayout->addWidget(m_table);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_btnAdd = new QPushButton("新增管理员/用户", userMgmtPage);
    m_btnReset = new QPushButton("强制重置密码", userMgmtPage);
    m_btnDelete = new QPushButton("永久删除用户", userMgmtPage);
    m_btnDelete->setObjectName("BtnDanger");
    m_btnPermissions = new QPushButton("分配权限", userMgmtPage);
    btnLayout->insertWidget(2, m_btnPermissions); // 插入到重置密码旁边
    connect(m_btnPermissions, &QPushButton::clicked, this, &UserManagementDialog::onAssignPermissionsClicked);

    btnLayout->addWidget(m_btnAdd);
    btnLayout->addWidget(m_btnReset);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnDelete);

    pageLayout->addLayout(btnLayout);
    stackedWidget->addWidget(userMgmtPage);

    // 组装主布局
    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(stackedWidget);

    // ================= 3. 仪表盘全局样式 (Nord风) =================
    this->setStyleSheet(R"(
        QDialog { background-color: #2E3440; border: 1px solid #4C566A; }
        QFrame#Sidebar { background-color: #3B4252; border-right: 1px solid #4C566A; }
        QPushButton#SidebarBtn, QPushButton#SidebarBtnActive, QPushButton#SidebarBtnExit {
            background-color: transparent; color: #D8DEE9; border: none;
            padding: 12px; text-align: left; padding-left: 20px; font-size: 15px; border-radius: 4px;
        }
        QPushButton#SidebarBtn:hover { background-color: #434C5E; }
        /* 左侧边栏选中时的冰蓝色高亮指示器 */
        QPushButton#SidebarBtnActive {
            background-color: #4C566A; color: #88C0D0; font-weight: bold;
            border-left: 4px solid #88C0D0; border-top-left-radius: 0; border-bottom-left-radius: 0;
        }
        QPushButton#SidebarBtnExit:hover { background-color: #BF616A; color: #ECEFF4; }

        QTableWidget {
            background-color: #3B4252; color: #D8DEE9;
            border: 1px solid #4C566A; gridline-color: #434C5E;
            border-radius: 4px; font-size: 14px; outline: none;
        }
        QHeaderView::section {
            background-color: #2E3440; color: #88C0D0; padding: 10px;
            border: none; border-bottom: 1px solid #4C566A;
        }
        QTableWidget::item:selected { background-color: #4C566A; color: #ECEFF4; }
        QPushButton {
            background-color: #4C566A; color: #ECEFF4; border: 1px solid #434C5E;
            padding: 8px 18px; border-radius: 4px; font-size: 14px;
        }
        QPushButton:hover { background-color: #5E81AC; }
        QPushButton#BtnDanger { color: #BF616A; border-color: #BF616A; background-color: transparent; }
        QPushButton#BtnDanger:hover { background-color: #BF616A; color: #ECEFF4; }
    )");

    // ================= 4. 信号槽连接 =================
    connect(m_btnAdd, &QPushButton::clicked, this, &UserManagementDialog::onAddUserClicked);
    connect(m_btnReset, &QPushButton::clicked, this, &UserManagementDialog::onResetPasswordClicked);
    connect(m_btnDelete, &QPushButton::clicked, this, &UserManagementDialog::onDeleteUserClicked);

    // 仪表盘侧边栏按钮交互
    connect(btnWorkspace, &QPushButton::clicked, this, &UserManagementDialog::onEnterGraphWorkspace);
    connect(btnExit, &QPushButton::clicked, this, &QDialog::accept);

    loadUsers();
}

// ================= 进入业务图谱前台的逻辑 =================
// src/ui/usermanagementdialog.cpp
void UserManagementDialog::onEnterGraphWorkspace() {
    // 1. 弹出项目选择框
    ProjectSelectionDialog selectDialog(this);
    if (selectDialog.exec() == QDialog::Accepted) {
        int ontoId = selectDialog.getSelectedOntologyId();
        QString ontoName = selectDialog.getSelectedOntologyName();

        QApplication::setQuitOnLastWindowClosed(false);

        // 2. 隐藏当前的后台管理仪表盘
        this->hide();

        // ================= 这里是修改的重点 =================
        // 构造一个具有最高权限的 Admin 用户
        User adminUser;
        adminUser.isAdmin = true;
        adminUser.canView = true;
        adminUser.canEdit = true;
        adminUser.canDelete = true;

        // 传入 adminUser 作为第三个参数！
        MainWindow* w = new MainWindow(ontoId, ontoName, adminUser);
        // =================================================

        w->setAttribute(Qt::WA_DeleteOnClose);

        connect(w, &MainWindow::destroyed, this, [this]() {
            this->show();
            QApplication::setQuitOnLastWindowClosed(true);
        });

        w->show();
    }
}
// ---------------- 以下为原有的后台用户 CRUD 操作 ----------------

void UserManagementDialog::loadUsers() {
    m_table->setRowCount(0);
    QList<User> users = UserRepository::getAllUsers();

    for (int i = 0; i < users.size(); ++i) {
        m_table->insertRow(i);
        m_table->setItem(i, 0, new QTableWidgetItem(QString::number(users[i].id)));
        m_table->setItem(i, 1, new QTableWidgetItem(users[i].username));
        m_table->setItem(i, 2, new QTableWidgetItem(users[i].isAdmin ? "管理员" : "普通用户"));
        m_table->setItem(i, 3, new QTableWidgetItem(users[i].password));

        // 渲染权限展示
        QString perms = QString("%1 / %2 / %3")
                            .arg(users[i].canView ? "查" : "-")
                            .arg(users[i].canEdit ? "改" : "-")
                            .arg(users[i].canDelete ? "删" : "-");
        m_table->setItem(i, 4, new QTableWidgetItem(perms));
    }
}

void UserManagementDialog::onAddUserClicked() {
    QDialog addDialog(this);
    addDialog.setWindowTitle("新增用户");
    addDialog.setStyleSheet(this->styleSheet());

    QFormLayout *form = new QFormLayout(&addDialog);
    form->setContentsMargins(20, 20, 20, 20);
    form->setSpacing(15);

    QLineEdit *nameEdit = new QLineEdit(&addDialog);
    QLineEdit *pwdEdit = new QLineEdit(&addDialog);
    QComboBox *roleCombo = new QComboBox(&addDialog);

    roleCombo->addItems({"普通用户", "管理员"});
    roleCombo->setStyleSheet("background-color: #3B4252; color: #ECEFF4; border: 1px solid #4C566A; padding: 5px;");
    nameEdit->setStyleSheet(roleCombo->styleSheet());
    pwdEdit->setStyleSheet(roleCombo->styleSheet());

    form->addRow("用户名:", nameEdit);
    form->addRow("初始密码:", pwdEdit);
    form->addRow("角色权限:", roleCombo);

    QPushButton *btnOk = new QPushButton("确定添加", &addDialog);
    form->addRow("", btnOk);

    connect(btnOk, &QPushButton::clicked, [&]() {
        QString name = nameEdit->text().trimmed();
        QString pwd = pwdEdit->text().trimmed();
        if (name.isEmpty() || pwd.isEmpty()) {
            QMessageBox::warning(&addDialog, "错误", "用户名和密码不能为空！");
            return;
        }
        if (UserRepository::addUser(name, pwd, roleCombo->currentIndex() == 1)) {
            addDialog.accept();
        } else {
            QMessageBox::warning(&addDialog, "错误", "添加失败，该用户名可能已存在！");
        }
    });

    if (addDialog.exec() == QDialog::Accepted) {
        loadUsers();
    }
}

void UserManagementDialog::onResetPasswordClicked() {
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先在表格中选中一个用户！");
        return;
    }

    int userId = m_table->item(row, 0)->text().toInt();
    QString userName = m_table->item(row, 1)->text();

    bool ok;
    QString newPwd = QInputDialog::getText(this, "重置密码",
        QString("请输入用户 [%1] 的新密码:").arg(userName),
        QLineEdit::Normal, "", &ok);

    if (ok && !newPwd.trimmed().isEmpty()) {
        if (UserRepository::resetPassword(userId, newPwd)) {
            QMessageBox::information(this, "成功", "密码重置成功！");
            loadUsers();
        } else {
            QMessageBox::warning(this, "失败", "密码重置失败！");
        }
    }
}

void UserManagementDialog::onDeleteUserClicked() {
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先在表格中选中一个用户！");
        return;
    }

    int userId = m_table->item(row, 0)->text().toInt();
    QString userName = m_table->item(row, 1)->text();

    if (userName == "admin") {
        QMessageBox::critical(this, "警告", "无法删除系统的超级管理员(admin)账号！");
        return;
    }

    if (QMessageBox::question(this, "危险操作确认", QString("确定要永久删除用户 [%1] 吗？").arg(userName)) == QMessageBox::Yes) {
        if (UserRepository::deleteUser(userId)) {
            loadUsers();
        } else {
            QMessageBox::warning(this, "失败", "删除用户失败！");
        }
    }
}

void UserManagementDialog::onAssignPermissionsClicked() {
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先在表格中选中一个用户！");
        return;
    }

    int userId = m_table->item(row, 0)->text().toInt();
    QString userName = m_table->item(row, 1)->text();
    bool isAdmin = m_table->item(row, 2)->text() == "管理员";

    if (isAdmin) {
        QMessageBox::information(this, "提示", "管理员默认拥有所有权限，无需分配！");
        return;
    }

    QDialog permDialog(this);
    permDialog.setWindowTitle("分配权限 - " + userName);
    permDialog.setStyleSheet(this->styleSheet());
    QVBoxLayout *layout = new QVBoxLayout(&permDialog);

    QCheckBox *chkView = new QCheckBox("允许查看 (View)", &permDialog);
    QCheckBox *chkEdit = new QCheckBox("允许修改 (Edit - 添加/编辑节点和关系)", &permDialog);
    QCheckBox *chkDelete = new QCheckBox("允许删除 (Delete - 删除节点和关系)", &permDialog);

    // 回显当前权限
    QString currentPerms = m_table->item(row, 4)->text();
    chkView->setChecked(currentPerms.contains("查"));
    chkEdit->setChecked(currentPerms.contains("改"));
    chkDelete->setChecked(currentPerms.contains("删"));

    layout->addWidget(chkView);
    layout->addWidget(chkEdit);
    layout->addWidget(chkDelete);

    QPushButton *btnOk = new QPushButton("保存权限", &permDialog);
    layout->addWidget(btnOk);

    connect(btnOk, &QPushButton::clicked, [&]() {
        if (UserRepository::updatePermissions(userId, chkView->isChecked(), chkEdit->isChecked(), chkDelete->isChecked())) {
            QMessageBox::information(&permDialog, "成功", "权限分配成功！");
            loadUsers();
            permDialog.accept();
        } else {
            QMessageBox::warning(&permDialog, "失败", "权限分配失败！");
        }
    });

    permDialog.exec();
}