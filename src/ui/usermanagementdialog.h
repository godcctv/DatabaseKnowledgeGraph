#ifndef USERMANAGEMENTDIALOG_H
#define USERMANAGEMENTDIALOG_H

#include <QDialog>

class QTableWidget;
class QPushButton;

class UserManagementDialog : public QDialog {
    Q_OBJECT
public:
    explicit UserManagementDialog(QWidget *parent = nullptr);

private slots:
    void loadUsers();
    void onAddUserClicked();
    void onResetPasswordClicked();
    void onDeleteUserClicked();

private:
    QTableWidget *m_table;
    QPushButton *m_btnAdd;
    QPushButton *m_btnReset;
    QPushButton *m_btnDelete;
};

#endif // USERMANAGEMENTDIALOG_H