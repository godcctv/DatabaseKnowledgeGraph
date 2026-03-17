#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include "../model/User.h"

class QLineEdit;
class QPushButton;

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget *parent = nullptr);
    User getCurrentUser() const { return m_currentUser; }

private slots:
    void onLoginClicked();
    void onRegisterClicked();
private:
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QPushButton *m_btnLogin;
    QPushButton *m_btnExit;
    QPushButton *m_btnRegister;
    User m_currentUser;
};

#endif // LOGINDIALOG_H