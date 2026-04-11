#include "aitextimportdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QMessageBox>
#include <QUrl>
#include <QSslConfiguration>
#include <QSslSocket>

AITextImportDialog::AITextImportDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("AI 智能文本知识抽取");
    resize(600, 500);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 1. API Key 输入框
    QHBoxLayout *keyLayout = new QHBoxLayout();
    keyLayout->addWidget(new QLabel("API Key: ", this));
    m_apiKeyInput = new QLineEdit(this);
    m_apiKeyInput->setPlaceholderText("填入你的API Key (形如 sk-...)");
    keyLayout->addWidget(m_apiKeyInput);
    mainLayout->addLayout(keyLayout);

    // 2. 文本输入区
    mainLayout->addWidget(new QLabel("请输入文本 :", this));
    m_textInput = new QTextEdit(this);
    m_textInput->setPlaceholderText("例如：关系型数据库包含MySQL和Oracle。MySQL是一种流行的开源数据库，它使用SQL语言进行操作...");
    mainLayout->addWidget(m_textInput);

    // 3. 状态与按钮
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    m_statusLabel = new QLabel("就绪", this);
    m_statusLabel->setStyleSheet("color: #EBCB8B;");
    m_btnStart = new QPushButton("智能抽取并导入", this);
    m_btnStart->setMinimumHeight(35);
    
    bottomLayout->addWidget(m_statusLabel);
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_btnStart);
    mainLayout->addLayout(bottomLayout);

    // 网络管理器
    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &AITextImportDialog::onNetworkReply);
    connect(m_btnStart, &QPushButton::clicked, this, &AITextImportDialog::onStartExtraction);

    // 沿用 Nord 风格
    setStyleSheet(R"(
        QDialog { background-color: #2E3440; color: #D8DEE9; }
        QLabel { color: #88C0D0; font-weight: bold; }
        QLineEdit, QTextEdit { background-color: #3B4252; border: 1px solid #4C566A; border-radius: 4px; padding: 8px; color: #ECEFF4; font-size: 14px;}
        QPushButton { background-color: #5E81AC; color: #ECEFF4; border: none; padding: 8px 20px; border-radius: 4px; font-weight: bold;}
        QPushButton:hover { background-color: #81A1C1; }
        QPushButton:disabled { background-color: #4C566A; color: #D8DEE9; }
    )");
}

void AITextImportDialog::onStartExtraction() {
    QString apiKey = m_apiKeyInput->text().trimmed();
    QString text = m_textInput->toPlainText().trimmed();

    if (apiKey.isEmpty() || text.isEmpty()) {
        QMessageBox::warning(this, "提示", "API Key 和文本内容不能为空！");
        return;
    }

    m_btnStart->setEnabled(false);
    m_btnStart->setText("正在思考与提取中...");
    m_statusLabel->setText("正在请求大模型，请稍候...");

    // 1. 组装发送的请求 JSON
    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = "你是一个知识图谱专家。请从用户提供的文本中提取实体作为节点，提取实体间的关系作为连线。"
                           "你必须严格返回合法的JSON格式，且绝对不要包含Markdown代码块符号(如```json)。"
                           "JSON的格式必须为: {\"nodes\": [{\"name\":\"实体名\", \"nodeType\":\"类型(如概念/实体/方法)\", \"description\":\"一句话描述\"}], "
                           "\"edges\": [{\"sourceName\":\"起点实体名\", \"targetName\":\"终点实体名\", \"relationType\":\"关系词\"}]}";

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = text;

    QJsonArray messages;
    messages.append(systemMsg);
    messages.append(userMsg);

    QJsonObject requestBody;
    requestBody["model"] = "Qwen/Qwen2.5-7B-Instruct";
    requestBody["messages"] = messages;
    requestBody["response_format"] = QJsonObject{{"type", "json_object"}};

    // 2. 核心修复：直接创建 Request 并设置 URL，去掉那个会误报的 isValid() 检查
    QNetworkRequest request;
    request.setUrl(QUrl("https://api.siliconflow.cn/v1/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + apiKey).toUtf8());

    // 3. 增加 SSL 兼容配置，跳过 Linux 下可能存在的本地证书校验问题
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    sslConfig.setProtocol(QSsl::AnyProtocol);
    request.setSslConfiguration(sslConfig);

    // 4. 发送请求
    m_networkManager->post(request, QJsonDocument(requestBody).toJson());
}
void AITextImportDialog::onNetworkReply(QNetworkReply* reply) {
    m_btnStart->setEnabled(true);
    m_btnStart->setText("智能抽取并导入");

    if (reply->error() != QNetworkReply::NoError) {
        m_statusLabel->setText("请求失败！");
        QMessageBox::critical(this, "网络错误", reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    QJsonObject jsonObj = jsonDoc.object();

    // 提取大模型的回答内容
    QString aiContent = jsonObj["choices"].toArray()[0].toObject()["message"].toObject()["content"].toString();
    
    // 清理可能误带的 Markdown 代码块标签
    if (aiContent.startsWith("```json")) aiContent.remove(0, 7);
    if (aiContent.startsWith("```")) aiContent.remove(0, 3);
    if (aiContent.endsWith("```")) aiContent.chop(3);

    QJsonDocument resultDoc = QJsonDocument::fromJson(aiContent.toUtf8());
    if (resultDoc.isNull() || !resultDoc.isObject()) {
        m_statusLabel->setText("解析失败！");
        QMessageBox::warning(this, "解析错误", "大模型返回的格式不符合要求，请重试！");
        reply->deleteLater();
        return;
    }

    QJsonObject resultObj = resultDoc.object();
    QJsonArray nodes = resultObj["nodes"].toArray();
    QJsonArray edges = resultObj["edges"].toArray();

    m_statusLabel->setText(QString("提取成功: 发现 %1 个节点, %2 条关系").arg(nodes.size()).arg(edges.size()));
    
    // 将数据发送给主窗口处理入库
    emit dataExtracted(nodes, edges);
    
    QMessageBox::information(this, "成功", "提取完毕，即将导入画板！");
    accept(); // 关闭窗口
    reply->deleteLater();
}