#ifndef QUERYDIALOG_H
#define QUERYDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

class QueryDialog : public QDialog {
    Q_OBJECT
public:
    explicit QueryDialog(QWidget* parent = nullptr) : QDialog(parent) {
        setWindowTitle("属性查询");
        resize(300, 150);

        QVBoxLayout* layout = new QVBoxLayout(this);

        // 属性名选择
        QHBoxLayout* h1 = new QHBoxLayout();
        h1->addWidget(new QLabel("属性名:", this));
        attrNameCombo = new QComboBox(this);
        attrNameCombo->addItem("name"); // 目前支持按名称
        attrNameCombo->addItem("type"); // 和类型查询
        h1->addWidget(attrNameCombo);
        layout->addLayout(h1);

        // 属性值输入
        QHBoxLayout* h2 = new QHBoxLayout();
        h2->addWidget(new QLabel("属性值:", this));
        attrValueEdit = new QLineEdit(this);
        h2->addWidget(attrValueEdit);
        layout->addLayout(h2);

        // 按钮
        QHBoxLayout* btnLayout = new QHBoxLayout();
        okButton = new QPushButton("查询", this);
        connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
        cancelButton = new QPushButton("取消", this);
        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

        btnLayout->addWidget(okButton);
        btnLayout->addWidget(cancelButton);
        layout->addLayout(btnLayout);
        // 赋予 Nord 样式
        this->setStyleSheet(R"(
            QDialog { background-color: #2E3440; border: 1px solid #4C566A; }
            QLabel { color: #D8DEE9; font-weight: bold; font-size: 13px; }
            QComboBox, QLineEdit {
                background-color: #3B4252; border: 1px solid #4C566A; border-radius: 4px;
                padding: 6px; color: #ECEFF4; font-size: 13px;
            }
            QComboBox:focus, QLineEdit:focus { border: 1px solid #88C0D0; }
            QComboBox::drop-down { border: none; }
            QPushButton {
                background-color: #4C566A; color: #ECEFF4; border: 1px solid #434C5E;
                padding: 6px 20px; border-radius: 4px;
            }
            QPushButton:hover { background-color: #5E81AC; }
            QPushButton:pressed { background-color: #81A1C1; }
        )");
    }

    QString getAttrName() const { return attrNameCombo->currentText(); }
    QString getAttrValue() const { return attrValueEdit->text(); }

private:
    QComboBox* attrNameCombo;
    QLineEdit* attrValueEdit;
    QPushButton* okButton;
    QPushButton* cancelButton;
};

#endif // QUERYDIALOG_H