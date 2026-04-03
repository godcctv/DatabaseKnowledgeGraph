#include "usermanagementdialog.h"
#include "../database/UserRepository.h"
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

UserManagementDialog::UserManagementDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("知识图谱系统 - 后台管理中心 (Admin)");
    resize(700, 500);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(20);

    // 标题
    QLabel* titleLabel = new QLabel("系 统 用 户 管 理", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #88C0D0; font-size: 24px; font-weight: bold; letter-spacing: 2px; border-bottom: 1px solid #4C566A; padding-bottom: 15px;");
    mainLayout->addWidget(titleLabel);

    // 1. 创建表格
    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({"用户 ID", "用户名", "角色权限", "密码"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers); // 禁止双击编辑表格
    mainLayout->addWidget(m_table);

    // 2. 创建底部按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_btnAdd = new QPushButton("新增管理员/用户", this);
    m_btnReset = new QPushButton("强制重置密码", this);
    m_btnDelete = new QPushButton("永久删除用户", this);
    m_btnDelete->setObjectName("BtnDanger"); // 绑定危险操作样式

    btnLayout->addWidget(m_btnAdd);
    btnLayout->addWidget(m_btnReset);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnDelete);

    mainLayout->addLayout(btnLayout);

    this->setStyleSheet(R"(
    QDialog { background-color: #2E3440; border: 1px solid #4C566A; }
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

    connect(m_btnAdd, &QPushButton::clicked, this, &UserManagementDialog::onAddUserClicked);
    connect(m_btnReset, &QPushButton::clicked, this, &UserManagementDialog::onResetPasswordClicked);
    connect(m_btnDelete, &QPushButton::clicked, this, &UserManagementDialog::onDeleteUserClicked);

    loadUsers();
}

void UserManagementDialog::loadUsers() {
    m_table->setRowCount(0);
    QList<User> users = UserRepository::getAllUsers();
    
    for (int i = 0; i < users.size(); ++i) {
        m_table->insertRow(i);
        
        QTableWidgetItem* idItem = new QTableWidgetItem(QString::number(users[i].id));
        idItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 0, idItem);
        
        m_table->setItem(i, 1, new QTableWidgetItem(users[i].username));
        
        QTableWidgetItem *roleItem = new QTableWidgetItem(users[i].isAdmin ? "管理员" : "普通用户");
        if (users[i].isAdmin) {
            roleItem->setForeground(QColor("#88C0D0")); // 统一为 Nord 冰蓝色
            roleItem->setFont(QFont("Microsoft YaHei", 10, QFont::Bold));
        }
        m_table->setItem(i, 2, roleItem);
        
        m_table->setItem(i, 3, new QTableWidgetItem(users[i].password));
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