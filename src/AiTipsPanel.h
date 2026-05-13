#pragma once
#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QStringList>
#include <QJsonArray>

// ---------------------------------------------------------------------------
//  AiTipsPanel — chat interface powered by Groq (free tier)
//
//  Setup:
//    1. Get a free key at https://console.groq.com
//    2. export GROQ_API_KEY=gsk_...
//    3. Run Simulytix — chat panel is ready
// ---------------------------------------------------------------------------

class AiTipsPanel : public QWidget
{
    Q_OBJECT
public:
    explicit AiTipsPanel(QWidget *parent = nullptr);

    // Called on Upload — updates the AI's context about the current circuit
    void setContext(const QString &sketchCode, const QStringList &componentNames);

private slots:
    void onSendClicked();
    void onReplyFinished(QNetworkReply *reply);
    void onGetTipsClicked();

private:
    void sendMessage(const QString &userText);
    void appendToChat(const QString &role, const QString &text);
    QString systemPrompt() const;

    // UI
    QTextEdit *m_chatDisplay; // scrolling chat history
    QLineEdit *m_inputBox;    // user types here
    QPushButton *m_sendButton;
    QPushButton *m_tipsButton; // "Get Tips" shortcut
    QPushButton *m_clearButton;
    QLabel *m_statusLabel;

    // Network
    QNetworkAccessManager *m_nam;

    // State
    QString m_sketchCode;
    QStringList m_components;
    QJsonArray m_history; // full conversation sent to Groq each time
};