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