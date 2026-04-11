#include "logindialog.h"
#include "../database/UserRepository.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QGraphicsDropShadowEffect>
#include <QFormLayout>

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("系统登录");
    resize(360, 260);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 30, 40, 30);
    mainLayout->setSpacing(20);

    // 标题
    QLabel *titleLabel = new QLabel("知 识 图 谱 系 统", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    // 大约在第 21 行
    titleLabel->setStyleSheet("color: #88C0D0; font-size: 22px; font-weight: bold; letter-spacing: 2px;");
    // 账号输入框
    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText("请输入账号");
    
    // 密码输入框
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText("请输入密码");
    m_passwordEdit->setEchoMode(QLineEdit::Password); // 密码模式，显示星号

    // 按钮布局
    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_btnRegister = new QPushButton("注 册", this);
    m_btnLogin = new QPushButton("登 录", this);
    m_btnExit = new QPushButton("退 出", this);

    m_btnRegister->setCursor(Qt::PointingHandCursor);
    m_btnLogin->setCursor(Qt::PointingHandCursor);
    m_btnExit->setCursor(Qt::PointingHandCursor);
    
    btnLayout->addWidget(m_btnExit);
    btnLayout->addWidget(m_btnRegister);
    btnLayout->addWidget(m_btnLogin);

    mainLayout->addWidget(titleLabel);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(m_usernameEdit);
    mainLayout->addWidget(m_passwordEdit);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(btnLayout);

    this->setStyleSheet(R"(
    QDialog { background-color: #2E3440; border: 1px solid #4C566A; }
    QLineEdit {
        background-color: #3B4252; border: 1px solid #4C566A;
        border-radius: 4px; padding: 8px 12px; color: #ECEFF4; font-size: 14px;
    }
    QLineEdit:focus { border: 1px solid #88C0D0; }
    QPushButton {
        background-color: #4C566A; color: #ECEFF4; border: 1px solid #434C5E;
        padding: 8px; border-radius: 4px; font-size: 14px;
    }
    QPushButton:hover { background-color: #5E81AC; }
    QPushButton:pressed { background-color: #81A1C1; }
    QPushButton#LoginBtn { background-color: #5E81AC; border: 1px solid #81A1C1; }
    QPushButton#LoginBtn:hover { background-color: #81A1C1; }
)");

    // 默认回车触发登录
    m_btnLogin->setDefault(true);

    connect(m_btnLogin, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(m_btnExit, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_btnRegister, &QPushButton::clicked, this, &LoginDialog::onRegisterClicked);
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
        if (m_currentUser.status == "PENDING") {
            QMessageBox::warning(this, "审核中", "您的账号正在审核中，请耐心等待管理员通过！");
            m_currentUser = User();
            return;
        } else if (m_currentUser.status == "REJECTED") {
            QMessageBox::critical(this, "审核拒绝", "您的注册请求已被拒绝！");
            m_currentUser = User();
            return;
        }
        accept();
    } else {
        QMessageBox::critical(this, "登录失败", "账号或密码错误！");
    }
}

void LoginDialog::onRegisterClicked() {
    QDialog regDialog(this);
    regDialog.setWindowTitle("用户注册");
    regDialog.setStyleSheet(this->styleSheet());

    QFormLayout *form = new QFormLayout(&regDialog);
    QLineEdit *nameEdit = new QLineEdit(&regDialog);
    nameEdit->setPlaceholderText("设置您的账号");
    QLineEdit *pwdEdit = new QLineEdit(&regDialog);
    pwdEdit->setPlaceholderText("设置您的密码");
    pwdEdit->setEchoMode(QLineEdit::Password);

    form->addRow("账号:", nameEdit);
    form->addRow("密码:", pwdEdit);

    QPushButton *btnOk = new QPushButton("确认注册", &regDialog);
    btnOk->setObjectName("LoginBtn");
    form->addRow("", btnOk);

    connect(btnOk, &QPushButton::clicked, [&]() {
        QString name = nameEdit->text().trimmed();
        QString pwd = pwdEdit->text().trimmed();
        if (name.isEmpty() || pwd.isEmpty()) {
            QMessageBox::warning(&regDialog, "错误", "账号和密码不能为空！");
            return;
        }
        if (UserRepository::registerUser(name, pwd)) {
            QMessageBox::information(&regDialog, "提交成功", "注册申请已提交！请等待管理员审核。");
            m_usernameEdit->setText(name);
            m_passwordEdit->clear();
            regDialog.accept();
        } else {
            QMessageBox::warning(&regDialog, "错误", "注册失败，该账号可能已存在！");
        }
    });

    regDialog.exec();
}