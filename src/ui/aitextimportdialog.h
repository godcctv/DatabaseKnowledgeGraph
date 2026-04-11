#ifndef AITEXTIMPORTDIALOG_H
#define AITEXTIMPORTDIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonArray>
#include <QLabel>

class AITextImportDialog : public QDialog {
    Q_OBJECT
public:
    explicit AITextImportDialog(QWidget *parent = nullptr);

    signals:
        // 当 AI 成功解析并提取出数据后，触发此信号给主窗口
        void dataExtracted(QJsonArray nodes, QJsonArray edges);

private slots:
    void onStartExtraction();
    void onNetworkReply(QNetworkReply* reply);

private:
    QTextEdit* m_textInput;
    QLineEdit* m_apiKeyInput;
    QPushButton* m_btnStart;
    QLabel* m_statusLabel;
    QNetworkAccessManager* m_networkManager;
};

#endif // AITEXTIMPORTDIALOG_H