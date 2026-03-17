#include "logindialog.h"
#include "../database/UserRepository.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QGraphicsDropShadowEffect>

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("系统登录");
    resize(360, 260);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 30, 40, 30);
    mainLayout->setSpacing(20);

    // 标题
    QLabel *titleLabel = new QLabel("知 识 图 谱 系 统", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #58a6ff; font-size: 22px; font-weight: bold; letter-spacing: 2px;");
    
    // 账号输入框
    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText("请输入账号 (admin/user)");
    
    // 密码输入框
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText("请输入密码 (123456)");
    m_passwordEdit->setEchoMode(QLineEdit::Password); // 密码模式，显示星号

    // 按钮布局
    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_btnLogin = new QPushButton("登 录", this);
    m_btnExit = new QPushButton("退 出", this);
    m_btnLogin->setCursor(Qt::PointingHandCursor);
    m_btnExit->setCursor(Qt::PointingHandCursor);
    
    btnLayout->addWidget(m_btnExit);
    btnLayout->addSpacing(20);
    btnLayout->addWidget(m_btnLogin);

    mainLayout->addWidget(titleLabel);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(m_usernameEdit);
    mainLayout->addWidget(m_passwordEdit);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(btnLayout);

    // 注入科幻 QSS
    this->setStyleSheet(R"(
        QDialog { background-color: #0B0D17; border: 1px solid #2A2F45; }
        QLineEdit {
            background-color: #08090F;
            border: 1px solid #2A2F45;
            border-radius: 6px;
            padding: 8px 12px;
            color: #00E5FF;
            font-size: 14px;
        }
        QLineEdit:focus { border: 1px solid #00E5FF; }
        QPushButton {
            background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1e3a5a, stop:1 #101828);
            color: #d0e6ff;
            border: 1px solid #3a6ea5;
            padding: 8px;
            border-radius: 6px;
            font-size: 14px;
            font-weight: bold;
        }
        QPushButton:hover { background-color: #3a6ea5; border-color: #00E5FF; color: #ffffff; }
        QPushButton:pressed { background-color: #0f172a; }
    )");

    // 默认回车触发登录
    m_btnLogin->setDefault(true);

    connect(m_btnLogin, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(m_btnExit, &QPushButton::clicked, this, &QDialog::reject);
}

void LoginDialog::onLoginClicked() {
    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text().trimmed();

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "提示", "账号或密码不能为空！");
        return;
    }

    // 调用数据库验证
    m_currentUser = UserRepository::login(username, password);

    if (m_currentUser.isValid()) {
        accept(); // 登录成功，关闭窗口并返回 Accepted
    } else {
        QMessageBox::critical(this, "登录失败", "账号或密码错误！");
    }
}