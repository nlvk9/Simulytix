#include "AiTipsPanel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QByteArray>
#include <QProcessEnvironment>
#include <QFont>
#include <QKeyEvent>
#include <QScrollBar>

static const char *GROQ_URL = "https://api.groq.com/openai/v1/chat/completions";

// ---------------------------------------------------------------------------
AiTipsPanel::AiTipsPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    // ── Header row ───────────────────────────────────────────────────────────
    auto *headerRow = new QWidget;
    auto *headerLayout = new QHBoxLayout(headerRow);
    headerLayout->setContentsMargins(0, 0, 0, 0);

    auto *title = new QLabel("💬  <b>AI Assistant</b>");
    title->setStyleSheet("font-size: 13px;");

    m_clearButton = new QPushButton("Clear");
    m_clearButton->setFixedWidth(50);
    m_clearButton->setStyleSheet(
        "QPushButton { background: #444; color: #ccc; border-radius: 4px;"
        "padding: 3px 8px; font-size: 11px; }"
        "QPushButton:hover { background: #555; }");

    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(m_clearButton);

    // ── Chat display ─────────────────────────────────────────────────────────
    m_chatDisplay = new QTextEdit;
    m_chatDisplay->setReadOnly(true);
    m_chatDisplay->setFont(QFont("Segoe UI", 11));
    m_chatDisplay->setStyleSheet(
        "QTextEdit {"
        "  background: #1e1e2e;"
        "  color: #cdd6f4;"
        "  border: 1px solid #333;"
        "  border-radius: 6px;"
        "  padding: 8px;"
        "  font-size: 12px;"
        "}");

    // ── "Get Tips" shortcut button ────────────────────────────────────────────
    m_tipsButton = new QPushButton("⚡  Get tips for my current circuit");
    m_tipsButton->setCursor(Qt::PointingHandCursor);
    m_tipsButton->setStyleSheet(
        "QPushButton {"
        "  background: #2d2d3f;"
        "  color: #89b4fa;"
        "  border: 1px solid #444;"
        "  border-radius: 6px;"
        "  padding: 6px 12px;"
        "  font-size: 11px;"
        "  text-align: left;"
        "}"
        "QPushButton:hover { background: #3d3d5f; }"
        "QPushButton:disabled { color: #666; }");

    // ── Status label ─────────────────────────────────────────────────────────
    m_statusLabel = new QLabel("");
    m_statusLabel->setStyleSheet("color: #666; font-size: 11px;");

    // ── Input row ────────────────────────────────────────────────────────────
    auto *inputRow = new QWidget;
    auto *inputLayout = new QHBoxLayout(inputRow);
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setSpacing(4);

    m_inputBox = new QLineEdit;
    m_inputBox->setPlaceholderText("Ask anything about your circuit...");
    m_inputBox->setStyleSheet(
        "QLineEdit {"
        "  background: #2d2d3f;"
        "  color: #cdd6f4;"
        "  border: 1px solid #444;"
        "  border-radius: 6px;"
        "  padding: 6px 10px;"
        "  font-size: 12px;"
        "}"
        "QLineEdit:focus { border: 1px solid #89b4fa; }");

    m_sendButton = new QPushButton("Send");
    m_sendButton->setFixedWidth(60);
    m_sendButton->setCursor(Qt::PointingHandCursor);
    m_sendButton->setStyleSheet(
        "QPushButton {"
        "  background: #1a73e8;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 6px 12px;"
        "  font-weight: bold;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover { background: #1558b0; }"
        "QPushButton:disabled { background: #555; color: #888; }");

    inputLayout->addWidget(m_inputBox, 1);
    inputLayout->addWidget(m_sendButton);

    layout->addWidget(headerRow);
    layout->addWidget(m_chatDisplay, 1);
    layout->addWidget(m_tipsButton);
    layout->addWidget(m_statusLabel);
    layout->addWidget(inputRow);

    // ── Network ──────────────────────────────────────────────────────────────
    m_nam = new QNetworkAccessManager(this);
    connect(m_nam, &QNetworkAccessManager::finished,
            this, &AiTipsPanel::onReplyFinished);
    connect(m_sendButton, &QPushButton::clicked,
            this, &AiTipsPanel::onSendClicked);
    connect(m_tipsButton, &QPushButton::clicked,
            this, &AiTipsPanel::onGetTipsClicked);
    connect(m_inputBox, &QLineEdit::returnPressed,
            this, &AiTipsPanel::onSendClicked);
    connect(m_clearButton, &QPushButton::clicked, this, [this]()
            {
        m_history = QJsonArray();
        m_chatDisplay->clear();
        appendToChat("system-info", "Chat cleared. Ask me anything about your circuit!"); });

    // Welcome message
    appendToChat("system-info",
                 "Hi! I'm your Arduino and circuit assistant.\n\n"
                 "Click \"Get tips for my current circuit\" for a walkthrough of your "
                 "current setup, or just ask me anything — wiring help, code questions, "
                 "or ideas for what to build next.");
}

// ---------------------------------------------------------------------------
void AiTipsPanel::setContext(const QString &sketchCode,
                             const QStringList &componentNames)
{
    m_sketchCode = sketchCode;
    m_components = componentNames;
}

// ---------------------------------------------------------------------------
QString AiTipsPanel::systemPrompt() const
{
    QString componentList = m_components.isEmpty()
                                ? "(no components placed yet)"
                                : m_components.join(", ");

    return QString(
               "You are a friendly Arduino and embedded hardware tutor inside a circuit "
               "simulator called Simulytix. Help beginners understand their circuits and code.\n\n"
               "Current components on the canvas: %1\n\n"
               "Current sketch code:\n```cpp\n%2\n```\n\n"
               "Keep responses concise and beginner-friendly. "
               "Use plain text only — no markdown, no asterisks, no hashes. "
               "If the user asks about their circuit or code, refer to the context above.")
        .arg(componentList, m_sketchCode.trimmed());
}

// ---------------------------------------------------------------------------
void AiTipsPanel::onGetTipsClicked()
{
    QString question = QString(
        "Please give me:\n"
        "1. A one-sentence summary of what my project does\n"
        "2. Step-by-step wiring instructions for my components\n"
        "3. A plain-English walkthrough of my sketch code\n"
        "4. Two ideas for what to try next");
    sendMessage(question);
}

// ---------------------------------------------------------------------------
void AiTipsPanel::onSendClicked()
{
    QString text = m_inputBox->text().trimmed();
    if (text.isEmpty())
        return;
    m_inputBox->clear();
    sendMessage(text);
}

// ---------------------------------------------------------------------------
void AiTipsPanel::sendMessage(const QString &userText)
{
    QString apiKey =
        QProcessEnvironment::systemEnvironment().value("GROQ_API_KEY");

    if (apiKey.isEmpty())
    {
        appendToChat("system-info",
                     "No API key found.\n\n"
                     "1. Go to https://console.groq.com and create a free key\n"
                     "2. Run: export GROQ_API_KEY=gsk_...\n"
                     "3. Restart Simulytix");
        return;
    }

    // Show user message in chat
    appendToChat("user", userText);

    // Add to history
    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = userText;
    m_history.append(userMsg);

    // Build full messages array: system prompt + history
    QJsonArray messages;
    QJsonObject sysMsg;
    sysMsg["role"] = "system";
    sysMsg["content"] = systemPrompt();
    messages.append(sysMsg);
    for (const auto &m : m_history)
        messages.append(m);

    QJsonObject body;
    body["model"] = "llama-3.1-8b-instant";
    body["max_tokens"] = 1024;
    body["temperature"] = 0.7;
    body["messages"] = messages;

    QUrl qurl(GROQ_URL);
    QNetworkRequest req(qurl);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", ("Bearer " + apiKey).toUtf8());

    m_sendButton->setEnabled(false);
    m_tipsButton->setEnabled(false);
    m_statusLabel->setText("Thinking...");

    m_nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
}

// ---------------------------------------------------------------------------
void AiTipsPanel::onReplyFinished(QNetworkReply *reply)
{
    m_sendButton->setEnabled(true);
    m_tipsButton->setEnabled(true);
    m_statusLabel->setText("");

    QByteArray data = reply->readAll();

    if (reply->error() != QNetworkReply::NoError)
    {
        QJsonDocument errDoc = QJsonDocument::fromJson(data);
        QString errMsg = errDoc.object()["error"].toObject()["message"].toString();
        if (errMsg.isEmpty())
            errMsg = reply->errorString();
        appendToChat("system-info", "Error: " + errMsg);
        reply->deleteLater();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();

    QString text;
    QJsonArray choices = root["choices"].toArray();
    if (!choices.isEmpty())
    {
        text = choices[0].toObject()["message"].toObject()["content"].toString();
    }

    if (text.isEmpty())
    {
        appendToChat("system-info", "Empty response received. Please try again.");
        reply->deleteLater();
        return;
    }

    // Add assistant reply to history so future messages have context
    QJsonObject assistantMsg;
    assistantMsg["role"] = "assistant";
    assistantMsg["content"] = text;
    m_history.append(assistantMsg);

    appendToChat("assistant", text);
    reply->deleteLater();
}

// ---------------------------------------------------------------------------
void AiTipsPanel::appendToChat(const QString &role, const QString &text)
{
    QString html;

    if (role == "user")
    {
        html = QString(
                   "<div style='margin: 8px 0; text-align: right;'>"
                   "<span style='background: #1a73e8; color: white; padding: 6px 10px;"
                   "border-radius: 12px 12px 2px 12px; display: inline-block;"
                   "max-width: 85%; font-size: 12px;'>%1</span></div>")
                   .arg(text.toHtmlEscaped().replace("\n", "<br>"));
    }
    else if (role == "assistant")
    {
        html = QString(
                   "<div style='margin: 8px 0;'>"
                   "<span style='background: #2d2d3f; color: #cdd6f4; padding: 6px 10px;"
                   "border-radius: 12px 12px 12px 2px; display: inline-block;"
                   "max-width: 85%; font-size: 12px;'>%1</span></div>")
                   .arg(text.toHtmlEscaped().replace("\n", "<br>"));
    }
    else
    {
        // system-info: centered grey text
        html = QString(
                   "<div style='margin: 6px 0; text-align: center;'>"
                   "<span style='color: #888; font-size: 11px; font-style: italic;'>"
                   "%1</span></div>")
                   .arg(text.toHtmlEscaped().replace("\n", "<br>"));
    }

    m_chatDisplay->append(html);

    // Auto-scroll to bottom
    QScrollBar *sb = m_chatDisplay->verticalScrollBar();
    sb->setValue(sb->maximum());
}